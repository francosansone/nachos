/// Routines to manage threads.
///
/// There are four main operations:
///
/// * `Fork` -- create a thread to run a procedure concurrently with the
///   caller (this is done in two steps -- first allocate the Thread object,
///   then call `Fork` on it).
/// * `Finish` -- called when the forked procedure finishes, to clean up.
/// * `Yield` -- relinquish control over the CPU to another ready thread.
/// * `Sleep` -- relinquish control over the CPU, but thread is now blocked.
///   In other words, it will not run again, until explicitly put back on the
///   ready queue.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread.hh"
#include "switch.h"
#include "synch.hh"
#include "system.hh"

class TranslationEntry;
class Puerto; //Le indico que existe una clase Puerto.
static int ContPid = 0;


//Lista global de pares (thread, pid) creados
List<ThreadTable> *threads = new List<ThreadTable>;


//Funciones sobre la lista threads

void
addThread(ThreadTable t)
{
    threads -> Append(t);
}

void
removeThread(SpaceId pid)
{
    List <ThreadTable> *newthreads = new List <ThreadTable>;
    while(!threads -> IsEmpty()) {
        ThreadTable temp = threads -> Remove();
        if(temp.Pid != pid)
            newthreads -> Append(temp);
    }
    threads = newthreads;
}

Thread *
getThread(SpaceId pid)
{
    List <ThreadTable> *newthreads = new List <ThreadTable>;
    Thread *ret = NULL;
    while(!threads-> IsEmpty()) {
        ThreadTable temp = threads -> Remove();
        if(temp.Pid == pid)
            ret = temp.thread;
        newthreads -> Append(temp);
    }
    threads = newthreads;
    return ret;
}


/// This is put at the top of the execution stack, for detecting stack
/// overflows.
const unsigned STACK_FENCEPOST = 0xdeadbeef;

/// Initialize a thread control block, so that we can then call
/// `Thread::Fork`.
///
/// * `threadName` is an arbitrary string, useful for debugging.
Thread::Thread(const char* threadName, bool join, int pr)
{
    name     = threadName;
    stackTop = NULL;
    stack    = NULL;
    status   = JUST_CREATED;
    port = new Puerto("port"); //Asigno memoria a port
    ToJoin = join;       //Por defecto, ToJoin es false
    Priority = pr;       //Por defecto, el hilo tiene la menor prioridad
    files = new List <struct stOpen>;
    cont = 2;           //contador de archivos abiertos
    Pid = ContPid;
    ContPid++;
    ThreadTable tt = {Pid, this};
    addThread(tt);
    #ifdef USE_TLB
        TranslationEntry tempSavedTlb[TLB_SIZE];
        TranslationEntry t;
        t.valid = false;
        for(unsigned i = 0; i < TLB_SIZE; i++)
            tempSavedTlb[i] = t;
        savedTlb = tempSavedTlb;
    #endif
    DEBUG('t',"Creado el hilo %d\n", Pid);
    //threads = new List <ThreadTable>;
#ifdef USER_PROGRAM
    space    = NULL;
#endif
}

/// De-allocate a thread.
///
/// NOTE: the current thread *cannot* delete itself directly, since it is
/// still running on the stack that we need to delete.
///
/// NOTE: if this is the main thread, we cannot delete the stack because we
/// did not allocate it -- we got it automatically as part of starting up
/// Nachos.
Thread::~Thread()
{
    DEBUG('t', "Deleting thread \"%s\"\n",name );

    ASSERT(this != currentThread);
    ///delete port; //no se realiza
    if (stack != NULL)
        DeallocBoundedArray((char *) stack, STACK_SIZE * sizeof *stack);
    #ifdef USE_TLB
        TranslationEntry tempTlbCopy[TLB_SIZE];
        flushTlb(tempTlbCopy);
    #endif
    delete files;
}

/// Invoke `(*func)(arg)`, allowing caller and callee to execute
/// concurrently.
///
/// NOTE: although our definition allows only a single integer argument to be
/// passed to the procedure, it is possible to pass multiple arguments by
/// by making them fields of a structure, and passing a pointer to the
/// structure as "arg".
///
/// Implemented as the following steps:
/// 1. Allocate a stack.
/// 2. Initialize the stack so that a call to SWITCH will cause it to run the
///    procedure.
/// 3. Put the thread on the ready queue.
///
/// * `func` is the procedure to run concurrently.
/// * `arg` is a single argument to be passed to the procedure.
void
Thread::Fork(VoidFunctionPtr func, void* arg, int p)
{
#ifdef HOST_x86_64
    DEBUG('t', "Forking thread \"%s\" with func = 0x%lX, arg = %ld\n",
          name, (HostMemoryAddress) func, arg);
#else
    DEBUG('t', "Forking thread \"%s\" with func = 0x%X, arg = %d\n",
          name, (HostMemoryAddress) func, arg);
#endif

    Priority = p;
    StackAllocate(func, arg);

    IntStatus oldLevel = interrupt->SetLevel(INT_OFF);
    scheduler->ReadyToRun(this);  // `ReadyToRun` assumes that interrupts
                                  // are disabled!
    interrupt->SetLevel(oldLevel);


}


/// Check a thread's stack to see if it has overrun the space that has been
/// allocated for it.  If we had a smarter compiler, we would not need to
/// worry about this, but we do not.
///
/// NOTE: Nachos will not catch all stack overflow conditions.  In other
/// words, your program may still crash because of an overflow.
///
/// If you get bizarre results (such as seg faults where there is no code)
/// then you *may* need to increase the stack size.  You can avoid stack
/// overflows by not putting large data structures on the stack.  Do not do
/// this:
///         void foo() { int bigArray[10000]; ... }
void
Thread::CheckOverflow()
{
    if (stack != NULL) {
        ASSERT(*stack == STACK_FENCEPOST);
    }
}

/// Called by `ThreadRoot` when a thread is done executing the forked
/// procedure.
///
/// NOTE: we do not immediately de-allocate the thread data structure or the
/// execution stack, because we are still running in the thread and we are
/// still on the stack!  Instead, we set `threadToBeDestroyed`, so that
/// `Scheduler::Run` will call the destructor, once we are running in the
/// context of a different thread.
///
/// NOTE: we disable interrupts, so that we do not get a time slice between
/// setting `threadToBeDestroyed`, and going to sleep.
void
Thread::Finish(int r)
{
    interrupt->SetLevel(INT_OFF);
    ASSERT(this == currentThread);

    DEBUG('t', "Finishing thread \"%s\"\n", getName());
    if(ToJoin){
        DEBUG('s', "Envia el valor de retorno (por defecto 0)\n");
        port -> Send(r); //desbloquea al join
    }
    threadToBeDestroyed = currentThread;
    Sleep();  // Invokes `SWITCH`.
    // Not reached.
}

/// Relinquish the CPU if any other thread is ready to run.
///
/// If so, put the thread on the end of the ready list, so that it will
/// eventually be re-scheduled.
///
/// NOTE: returns immediately if no other thread on the ready queue.
/// Otherwise returns when the thread eventually works its way to the front
/// of the ready list and gets re-scheduled.
///
/// NOTE: we disable interrupts, so that looking at the thread on the front
/// of the ready list, and switching to it, can be done atomically.  On
/// return, we re-set the interrupt level to its original state, in case we
/// are called with interrupts disabled.
///
/// Similar to `Thread::Sleep`, but a little different.
void
Thread::Yield()
{
    Thread *nextThread;
    IntStatus oldLevel = interrupt->SetLevel(INT_OFF);

    ASSERT(this == currentThread);

    DEBUG('t', "Yielding thread \"%s\"\n", getName());

    nextThread = scheduler->FindNextToRun();
    if (nextThread != NULL) {
        scheduler->ReadyToRun(this);
        scheduler->Run(nextThread);
    }
    interrupt->SetLevel(oldLevel);
}

/// Relinquish the CPU, because the current thread is blocked waiting on a
/// synchronization variable (`Semaphore`, `Lock`, or `Condition`).
/// Eventually, some thread will wake this thread up, and put it back on the
/// ready queue, so that it can be re-scheduled.
///
/// NOTE: if there are no threads on the ready queue, that means we have no
/// thread to run.  `Interrupt::Idle` is called to signify that we should
/// idle the CPU until the next I/O interrupt occurs (the only thing that
/// could cause a thread to become ready to run).
///
/// NOTE: we assume interrupts are already disabled, because it is called
/// from the synchronization routines which must disable interrupts for
/// atomicity.  We need interrupts off so that there cannot be a time slice
/// between pulling the first thread off the ready list, and switching to it.
void
Thread::Sleep()
{
    Thread *nextThread;

    ASSERT(this == currentThread);
    ASSERT(interrupt->getLevel() == INT_OFF);

    DEBUG('t', "Sleeping thread \"%s\"\n", getName());

    status = BLOCKED;
    while ((nextThread = scheduler->FindNextToRun()) == NULL) {
        interrupt->Idle();  // No one to run, wait for an interrupt.
    }

    scheduler->Run(nextThread);  // Returns when we have been signalled.
}

/// ThreadFinish, InterruptEnable
///
/// Dummy functions because C++ does not allow a pointer to a member
/// function.  So in order to do this, we create a dummy C function (which we
/// can pass a pointer to), that then simply calls the member function.
static void
ThreadFinish()
{
    currentThread->Finish();
}

static void
InterruptEnable()
{
    interrupt->Enable();
}

/// Allocate and initialize an execution stack.
///
/// The stack is initialized with an initial stack frame for `ThreadRoot`,
/// which:
/// 1. enables interrupts;
/// 2. calls `(*func)(arg)`;
/// 3. calls `Thread::Finish`.
///
/// * `func` is the procedure to be forked.
/// * `arg` is the parameter to be passed to the procedure.
void
Thread::StackAllocate(VoidFunctionPtr func, void *arg)
{
    stack = (HostMemoryAddress *) AllocBoundedArray(STACK_SIZE
                                                    * sizeof *stack);

    // i386 & MIPS & SPARC stack works from high addresses to low addresses.
    stackTop = stack + STACK_SIZE - 4;  // -4 to be on the safe side!

    // the 80386 passes the return address on the stack.  In order for
    // `SWITCH` to go to `ThreadRoot` when we switch to this thread, the
    // return addres used in `SWITCH` must be the starting address of
    // `ThreadRoot`.
    *--stackTop = (HostMemoryAddress) ThreadRoot;

    *stack = STACK_FENCEPOST;

    machineState[PCState]         = (HostMemoryAddress) ThreadRoot;
    machineState[StartupPCState]  = (HostMemoryAddress) InterruptEnable;
    machineState[InitialPCState]  = (HostMemoryAddress) func;
    machineState[InitialArgState] = (HostMemoryAddress) arg;
    machineState[WhenDonePCState] = (HostMemoryAddress) ThreadFinish;
}


//defino el join
//----------------------------------------------------------------------------
// Asigno true a ToJoin, puesto que el hilo padre se bloqueará y esperara a
// que el hijo termine. Esto evita que hilos que no realizan ningun fork no se
// bloqueen indefinidamente esperando a un hijo que no existe.
void
Thread::Join(int *ret)
{
    ToJoin = true;
    if(!ret) {
        int Buff;
        port -> Receive(&Buff);
     }
     else
        port -> Receive(ret);
}

//------------------------------------------------------------------------


//funciones definidas para excepcion.cc
//----------------------------------------------------------------------------
OpenFileId
Thread::addFile(OpenFile *f)
{
    struct stOpen nf;
    nf.file = f;
    nf.number = cont;
    cont++;
    files -> Append(nf);
    return (cont - 1);
}

void
Thread::removeFile (OpenFileId f)
{
    List <struct stOpen> *newfiles = new List <struct stOpen>;
    while(!files -> IsEmpty()){
        struct stOpen temp = files -> Remove();
        if(temp.number != f)
            newfiles -> Append(temp);
    }
    files = newfiles;
}

OpenFile *
Thread::getFile(OpenFileId f)
{
    List <struct stOpen> *newfiles = new List <struct stOpen>;
    OpenFile *ret = NULL;
    while(!files -> IsEmpty()){
        struct stOpen temp = files -> Remove();
        if(temp.number == f)
            ret = temp.file;
        newfiles -> Append(temp);
    }
    files = newfiles;
    return ret;
}

void
Thread::PrintList()
{
    List <struct stOpen> *newfiles = new List <struct stOpen>;
    while(!files -> IsEmpty()){
        struct stOpen temp = files -> Remove();
        newfiles -> Append(temp);
    }
    files = newfiles;
}


//Esta funcion cierra todos los archivos abiertos
void
Thread::FileClose()
{
    while(!files -> IsEmpty()) {
        struct stOpen temp = files -> Remove();
        DEBUG('f', "Cerrado el archivo %d\n", temp.number);
    }
}

#ifdef USER_PROGRAM
#include "machine.hh"

/// Save the CPU state of a user program on a context switch.
///
/// Note that a user program thread has *two* sets of CPU registers -- one
/// for its state while executing user code, one for its state while
/// executing kernel code.  This routine saves the former.

void
Thread::flushTlb(TranslationEntry *tempSavedTlb){
    #ifdef USE_TLB
        TranslationEntry t;
        for(unsigned i = 0; i < TLB_SIZE; i++){
            // printf("SaveUserState %u\n", i);
            tempSavedTlb[i] = machine->tlb[i];
            machine->tlb[i] = t;
        }
    #endif

};

void
Thread::SaveUserState()
{
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++)
        userRegisters[i] = machine->ReadRegister(i);

    #ifdef USE_TLB
        // Flush TLB
        TranslationEntry tempSavedTlb[TLB_SIZE];
        flushTlb(tempSavedTlb);
        savedTlb = tempSavedTlb;
    #endif
}

/// Restore the CPU state of a user program on a context switch.
///
/// Note that a user program thread has *two* sets of CPU registers -- one
/// for its state while executing user code, one for its state while
/// executing kernel code.  This routine restores the former.
void
Thread::RestoreUserState()
{
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++)
        machine->WriteRegister(i, userRegisters[i]);

        DEBUG('a',"RestoreUserState\n");
    #ifdef USE_TLB
        for(unsigned i = 0; i < TLB_SIZE; i++){
            // printf("RestoreUserState %u\n", i);
            machine->tlb[i] = savedTlb[i];
        }
    #endif
}

#endif

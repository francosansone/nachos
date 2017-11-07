/// Routines to choose the next thread to run, and to dispatch to that
/// thread.
///
/// These routines assume that interrupts are already disabled.  If
/// interrupts are disabled, we can assume mutual exclusion (since we are on
/// a uniprocessor).
///
/// NOTE: we cannot use `Lock`s to provide mutual exclusion here, since if we
/// needed to wait for a lock, and the lock was busy, we would end up calling
/// `FindNextToRun`, and that would put us in an infinite loop.
///
/// Very simple implementation -- no priorities, straight FIFO.  Might need
/// to be improved in later assignments.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "scheduler.hh"
#include "system.hh"

typedef List <Thread *> ListThread;

/// Initialize the list of ready but not running threads to empty.
Scheduler::Scheduler()
{
    for(int i = 0; i <= MAX_PRIORITY; i++)
        readyList[i] = new ListThread;
}

/// De-allocate the list of ready threads.
Scheduler::~Scheduler()
{
    int i;
    for(i = 0; i <= MAX_PRIORITY; i++)
    {
        DEBUG('t', "Se borra la cola de prioridad %d\n", i);
        delete readyList[i];
    }
}

/// Mark a thread as ready, but not running.
/// Put it on the ready list, for later scheduling onto the CPU.
///
/// * `thread` is the thread to be put on the ready list.
void
Scheduler::ReadyToRun(Thread *thread)
{
    int i;
    DEBUG('t', "Putting thread %s on ready list, priority %d .\n",
                thread->getName(), thread -> getPriority());
                
    i = thread -> getPriority();
    thread->setStatus(READY);
    (readyList[i])->Append(thread);
}

/// Return the next thread to be scheduled onto the CPU.
///
/// If there are no ready threads, return `NULL`.
///
/// Side effect: thread is removed from the ready list.

//Recorro la lista de listos, comenzando por la mayor prioridad.
//Verifica si hay hilos en la lista, si hay lo elimina y retorna,
//sino sigue buscando. Si no encuentra retorna NULL.
Thread *
Scheduler::FindNextToRun()
{
    int i;

    for(i=MAX_PRIORITY; i>=0; i--)
        if(!readyList[i] -> IsEmpty())
            return readyList[i] -> Remove();

    return NULL;
}

/// Defino esta funcion para eliminar al hilo Thread de su cola.
/// Luego, sera reubicado en su cola correspondiente con su
/// nueva prioridad.
void
Scheduler::moveThread(Thread *thread, int Priority)
{
    ListThread *newList = new ListThread;
    while(!readyList[Priority] -> IsEmpty())
    {
        Thread *temp = readyList[Priority] -> Remove();
        if(temp != thread)
            newList -> Prepend(temp);
    }
    readyList[Priority] = newList; //obtengo la misma lista, sin thread

    //si el proceso esta en modo READY, lo agrego a la correspondiente 
    readyList[thread -> getPriority()] -> Append(thread);

    // No libero la memoria guardada por newList, pues de ser asi 
    // readyList[Priority] apunta a un sector de memoria vacio
}

/// Dispatch the CPU to `nextThread`.
///
/// Save the state of the old thread, and load the state of the new thread,
/// by calling the machine dependent context switch routine, `SWITCH`.
///
/// Note: we assume the state of the previously running thread has already
/// been changed from running to blocked or ready (depending).
///
/// Side effect: the global variable `currentThread` becomes `nextThread`.
///
/// * `nextThread` is the thread to be put into the CPU.
void
Scheduler::Run(Thread *nextThread)
{
    Thread *oldThread = currentThread;

#ifdef USER_PROGRAM  // Ignore until running user programs.
    if (currentThread->space != NULL) {
        // If this thread is a user program, save the user's CPU registers.
        currentThread->SaveUserState();
        currentThread->space->SaveState();
    }
#endif

    oldThread->CheckOverflow();  // Check if the old thread had an undetected
                                 // stack overflow.

    currentThread = nextThread;  // Switch to the next thread.
    currentThread->setStatus(RUNNING);  // `nextThread` is now running.

    DEBUG('t', "Switching from thread \"%s\" to thread \"%s\"\n",
          oldThread->getName(), nextThread->getName());

    // This is a machine-dependent assembly language routine defined in
    // `switch.s`.  You may have to think a bit to figure out what happens
    // after this, both from the point of view of the thread and from the
    // perspective of the “outside world”.

    SWITCH(oldThread, nextThread);

    DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());

    // If the old thread gave up the processor because it was finishing, we
    // need to delete its carcass.  Note we cannot delete the thread before
    // now (for example, in `Thread::Finish`), because up to this point, we
    // were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }

#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {
        // If there is an address space to restore, do it.
        currentThread->RestoreUserState();
        currentThread->space->RestoreState();
    }
#endif
}

/// Print the scheduler state -- in other words, the contents of the ready
/// list.
///
/// For debugging.
static void
ThreadPrint(Thread* t)
{
    t->Print();
}

void
Scheduler::Print()
{
    int i;
    printf("Ready list contents:\n");
    for(i = 5; i >= 0; i--)
        readyList[i]->Apply(ThreadPrint);
}

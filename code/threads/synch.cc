/// Routines for synchronizing threads.
///
/// Three kinds of synchronization routines are defined here: semaphores,
/// locks and condition variables (the implementation of the last two are
/// left to the reader).
///
/// Any implementation of a synchronization routine needs some primitive
/// atomic operation.  We assume Nachos is running on a uniprocessor, and
/// thus atomicity can be provided by turning off interrupts.  While
/// interrupts are disabled, no context switch can occur, and thus the
/// current thread is guaranteed to hold the CPU throughout, until interrupts
/// are reenabled.
///
/// Because some of these routines might be called with interrupts already
/// disabled (`Semaphore::V` for one), instead of turning on interrupts at
/// the end of the atomic operation, we always simply re-set the interrupt
/// state back to its original value (whether that be disabled or enabled).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "synch.hh"
#include "system.hh"
#include "scheduler.hh"

class Thread;

/// Initialize a semaphore, so that it can be used for synchronization.
///
/// * `debugName` is an arbitrary name, useful for debugging.
/// * `initialValue` is the initial value of the semaphore.
Semaphore::Semaphore(const char *debugName, int initialValue)
{
    name  = debugName;
    value = initialValue;
    queue = new List<Thread *>;
}

/// De-allocate semaphore, when no longer needed.
///
/// Assume no one is still waiting on the semaphore!
Semaphore::~Semaphore()
{
    delete queue;
}

/// Wait until semaphore `value > 0`, then decrement.
///
/// Checking the value and decrementing must be done atomically, so we need
/// to disable interrupts before checking the value.
///
/// Note that `Thread::Sleep` assumes that interrupts are disabled when it is
/// called.
void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(INT_OFF);
      // Disable interrupts.

    while (value == 0) {  // Semaphore not available.
        queue->Append(currentThread);  // So go to sleep.
        currentThread->Sleep();
    }
    value--;  // Semaphore available, consume its value.

    interrupt->SetLevel(oldLevel);  // Re-enable interrupts.
}

/// Increment semaphore value, waking up a waiter if necessary.
///
/// As with `P`, this operation must be atomic, so we need to disable
/// interrupts.  `Scheduler::ReadyToRun` assumes that threads are disabled
/// when it is called.
void
Semaphore::V()
{
    Thread   *thread;
    IntStatus oldLevel = interrupt->SetLevel(INT_OFF);

    thread = queue->Remove();
    if (thread != NULL)  // Make thread ready, consuming the `V` immediately.
        scheduler->ReadyToRun(thread);
    value++;
    interrupt->SetLevel(oldLevel);
}

/// Dummy functions -- so we can compile our later assignments.
///
/// Note -- without a correct implementation of `Condition::Wait`, the test
/// case in the network assignment will not work!

//------------------------------------------------------------------------------    
//modificaciones correspondientes al ejercicio 1


//Asigno memoria y defino las variables necesarias
Lock::Lock(const char *debugName)
{
    name = debugName;
    sem = new Semaphore("sem",1);
    ThreadHasLock = NULL;
    changePr = false;
}

//Libero la memoria asignada
Lock::~Lock()
{
    delete(sem);
}

//Decremento el lock para bloquear y asigno ThreadHasLock
void
Lock::Acquire()
{
    //si el hilo posee el semaforo, pum!
    ASSERT(!IsHeldByCurrentThread());
    
    // Inversion de prioridades
    // El hilo verifica si algun hilo de menor prioridad tiene el lock,
    // de tenerlo, le asigna provisoriamente su priodad y lo inserta
    // en la cola correspondiente.
    //#ifdef SCHED_PRIORITYINHERITANCE
    if(ThreadHasLock != NULL &&
        ThreadHasLock -> getPriority() < currentThread -> getPriority())
    {
        changePr = true;
        oldPr = ThreadHasLock -> getPriority();
        ThreadHasLock -> setPriority(currentThread -> getPriority());
        if(ThreadHasLock -> getStatus() == 2)
            scheduler -> moveThread(ThreadHasLock, oldPr);
       
    }
    //#endif
    
    sem->P();
    ThreadHasLock = currentThread;
}

//Libero el lock para que un proceso pueda avanzar y pongo NULL a 
//ThreadHasLock, lo cual tiene sentido, nadie tiene el lock
void
Lock::Release()
{
    //si el hilo NO posee el semaforo, pum!
    ASSERT(IsHeldByCurrentThread());  
    // Inversion de prioridades. No es necesario evaluar si 
    // SCHED_PRIORITYINHERITANCE esta definida, puesto que de
    // no estarle, changePr es false
    if(changePr)
    {
        changePr = false;
        currentThread -> setPriority(oldPr); //retorna a su prioridad anterior
    }
    ThreadHasLock = NULL;
    sem->V();
}

//obvia
bool
Lock::IsHeldByCurrentThread()
{
    return currentThread == ThreadHasLock;
}
    
    

//Asigno memoria y defino las variables necesarias
Condition::Condition(const char *debugName, Lock *conditionLock)
{
    name = debugName;
    queueSem = new List<Semaphore *>;
    CopyLock = conditionLock;
}

//Libero la memoria asignada
Condition::~Condition()
{
    delete(queueSem);
}

//verifico que tenga el lock de no tenerlo, podria haber señales perdidas
//defino el semaforo y lo agrego a la cola
//una vez dormido el hilo por sem, no volvera a ejecutar hasta que ocurra un
//signal. Cuando sea despertado y retome la ejecucion, vuelve a tomar el lock
void
Condition::Wait()
{
    ASSERT(CopyLock -> IsHeldByCurrentThread());
    Semaphore *sem = new Semaphore("sem",0); 
    queueSem -> Append(sem);       
    CopyLock -> Release();         
    sem -> P(); 
    CopyLock -> Acquire();
    //delete sem;

}


//verifico que el hilo que hace el signal tenga el lock
//si la cola esta vacia, la señal no tiene nada que hacer, se pierde
void
Condition::Signal()
{
    ASSERT(CopyLock -> IsHeldByCurrentThread());
    if(!queueSem -> IsEmpty())
    {
        Semaphore *s = queueSem -> Remove();
        s -> V(); //el hilo que hizo el wait se desbloquea
    }
}


//verifico que el hilo que hace el signal tenga el lock
//envia señales hasta que la cola este vacia
void
Condition::Broadcast()
{

    ASSERT(CopyLock -> IsHeldByCurrentThread());
    while(!queueSem -> IsEmpty())
        Signal();

}

//------------------------------------------------------------------------------    

//------------------------------------------------------------------------------    
//modificaciones correspondientes al ejercicio 2

//Asigno memoria y defino las variables necesarias
Puerto::Puerto(const char *debugName)
{
    name = debugName;
    lock = new Lock("lock");
    HayMensaje = false;
    Buff_lleno = new Condition("Buff lleno", lock);
    Buff_vacio = new Condition("Buff vacio", lock);
}

//Libero la memoria asignada
Puerto::~Puerto()
{
    delete(lock);
    delete(Buff_lleno);
    delete(Buff_vacio);
}


//Tomo el lock, verifico si hay mensajes. Cuando no los haya, copio el mensaje
//en el buffer, envio la señal correspondiente y libero el lock.
void
Puerto::Send(int mensaje)
{
    lock -> Acquire();
    while(HayMensaje)         //Espera mientras haya mensajes
        Buff_lleno -> Wait(); //Wait se encarga de soltar el lock
    HayMensaje = true;
    Buff = mensaje;
    DEBUG('s',"Se envio el mensaje %d por el puerto \"%s\"\n\n", Buff, name);
    Buff_vacio -> Signal();
    lock -> Release();
}


//Tomo el lock, verifico si hay mensajes. Cuando los haya, lo copio, envio la 
//señal correspondiente y libero el lock.
void
Puerto::Receive(int *mensaje)
{
    lock -> Acquire();
    while(!HayMensaje)        //Espera mientras no haya mensajes
        Buff_vacio -> Wait(); 
    *mensaje = Buff;
    DEBUG('s',"Se recibio el mensaje %d por el puerto \"%s\"\n", *mensaje, name);
    HayMensaje = false;
    Buff_lleno -> Signal();
    lock -> Release();
}


//------------------------------------------------------------------------------    







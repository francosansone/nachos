/// Simple test case for the threads assignment.
///
/// Create several threads, and have them context switch back and forth
/// between themselves by calling `Thread::Yield`, to illustrate the inner
/// workings of the thread system.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "system.hh"
#include "synch.hh"

//defino en este sector para que este definida en el alcance de SimpleThread
Semaphore *sem; 
Lock *lock;
Semaphore *blisto;
Semaphore *blisto2;
Lock *lock_p;
Puerto *port;

/// Loop 10 times, yielding the CPU to another ready thread each iteration.
///
/// * `name` points to a string with a thread name, just for debugging
///   purposes.


void
SimpleThread(void *name_)
{
    // Reinterpret arg `name` as a string.
    char *name = (char *) name_;

    // If the lines dealing with interrupts are commented, the code will
    // behave incorrectly, because printf execution may cause race
    // conditions.
    
    #ifdef SEMAPHORE_LOCK_TEST
    //sem -> P();
    lock -> Acquire();
    #endif

    for (unsigned num = 0; num < 10; num++) {
        //IntStatus oldLevel = interrupt->SetLevel(IntOff);
        printf("*** Thread `%s` is running: iteration %d\n", name, num);
        currentThread->Yield();
    }

    #ifdef SEMAPHORE_LOCK_TEST
    //sem -> V();
    lock -> Release();
    #endif
        
    //IntStatus oldLevel = interrupt->SetLevel(IntOff);
    printf("!!! Thread `%s` has finished\n", name);
    //interrupt->SetLevel(oldLevel);
}

void A(void *name_) {
    blisto->P();
    lock_p->Acquire();
    DEBUG('s',"A:tengo el lock\n");
    printf("A: Pudiste ejecutarme!\n");
    lock_p ->Release();
}

void M(void *name_) {
    blisto2 -> P();
    while(1);
}

void B(void *name_) {
    lock_p -> Acquire();
    blisto->V();
    blisto2->V();
    currentThread -> Yield () ;
    DEBUG('s', "B: tengo el lock\n");
    printf("B: Pudiste ejecutarme!\n");
    lock_p -> Release();
}


void
emisor(void *name){
    port -> Send(5);
    port -> Send(6);
}

void
receptor(void *name){
    int buf;
    port -> Receive(&buf);
    printf("\nel buff es: %d\n", buf);
}



/// Set up a ping-pong between several threads.
///
/// Do it by launching ten threads which call `SimpleThread`, and finally
/// calling `SimpleThread` ourselves.
void
ThreadTest()
{

    DEBUG('t', "Entering SimpleTest");
    #ifdef SEMAPHORE_LOCK_TEST 
    sem = new Semaphore("Sem",3);
    lock = new Lock("lock");
    #endif
    
    #ifdef SIMPLETHREAD_TEST
    for(int i=0; i<5; i++){ 
        char *name = new char[64];
        sprintf(name, "Thread %d", i);
        Thread *newThread = new Thread(name, true);
        newThread->Fork(SimpleThread, (void *) name, 4);
        #ifdef JOIN_TEST
        newThread->Join();
        #endif
    }
    #endif
           
    char *name = new char[64];
    
    #ifdef PORT_TEST
    port = new Puerto("8080");
    sprintf(name, "hijo");    
    Thread *Thread1 = new Thread("Thread 1"), *Thread2 = new Thread("Thread 2"),  
    *Thread3 = new Thread("Thread 3");
    Thread1->Fork(emisor,  name, MAX_PRIORITY);
    Thread2->Fork(receptor,  name, MAX_PRIORITY);
    Thread3->Fork(receptor, name, MAX_PRIORITY);
    #endif
    
    #ifdef PRIORITY_TEST
    blisto = new Semaphore("blisto", 0);
    blisto2 = new Semaphore("blisto2",0);
    lock_p = new Lock("lock");
    sprintf(name, "hijo");    
    Thread *ThreadB = new Thread("Thread B"), *ThreadM = new Thread("Thread M"),  
    *ThreadA = new Thread("Thread A");
    ThreadB->Fork(B,  name,1);
    ThreadA->Fork(A, name,3);
    ThreadM->Fork(M,  name,2);
    #endif

    delete [] name;
}

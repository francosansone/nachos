/// Entry point into the Nachos kernel from user programs.
///
/// There are two kinds of things that can cause control to transfer back to
/// here from user code:
///
/// * System calls: the user code explicitly requests to call a procedure in
///   the Nachos kernel.  Right now, the only function we support is `Halt`.
///
/// * Exceptions: the user code does something that the CPU cannot handle.
///   For instance, accessing memory that does not exist, arithmetic errors,
///   etc.
///
/// Interrupts (which can also cause control to transfer from user code into
/// the Nachos kernel) are handled elsewhere.
///
/// For now, this only handles the `Halt` system call.  Everything else core
/// dumps.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "syscall.h"
#include "threads/system.hh"
#include "threads/thread.hh"
#include "address_space.hh"
#include "args.cc"
#include "read_write.cc"

/// Entry point into the Nachos kernel.  Called when a user program is
/// executing, and either does a syscall, or generates an addressing or
/// arithmetic exception.
///
/// For system calls, the following is the calling convention:
///
/// * system call code in `r2`;
/// * 1st argument in `r4`;
/// * 2nd argument in `r5`;
/// * 3rd argument in `r6`;
/// * 4th argument in `r7`;
/// * the result of the system call, if any, must be put back into `r2`.
///
/// And do not forget to increment the pc before returning. (Or else you will
/// loop making the same system call forever!)
///
/// * `which` is the kind of exception.  The list of possible exceptions is
///   in `machine.hh`.



void StartProc(void *args);

//void WriteArgs(char **args);

//char ** SaveArgs(int address);

void insertTLB(TranslationEntry);

static int penalizedPage = 0;

void
IncPC()
{
    int pc = machine->ReadRegister(PC_REG);
    machine->WriteRegister(PREV_PC_REG, pc);
    pc = machine->ReadRegister(NEXT_PC_REG);
    machine->WriteRegister(PC_REG, pc);
    pc += 4;
    machine->WriteRegister(NEXT_PC_REG, pc);
    return;
}


void
ExceptionHandler(ExceptionType which)
{
    // printf("ExceptionHandler" );
    int type = machine->ReadRegister(2);
    char name[128];
    char buff[128];
    int r4 = machine -> ReadRegister(4);//defino esto por prolijidad
    int r5 = machine -> ReadRegister(5);
    int r6 = machine -> ReadRegister(6);
    if (which == SYSCALL_EXCEPTION) {
        DEBUG('t', "SYSCALL_EXCEPTION\n");
        switch (type) {
            case SC_Halt: {
                DEBUG('a', "Shutdown, initiated by user program.\n");
                #ifdef HIT_RATIO
                    float misses = machine->getMisses();
                    float reads = machine->getReads();
                    float hitRatio =  misses / reads;
                    int intHitRatio = 100 - hitRatio * 100;
                    printf("\nmisses: %f, reads: %f hit ratio: %d \n",
                        misses, reads, intHitRatio);
                #endif
                interrupt->Halt();
                break;
            }

            case SC_Create: {
                DEBUG('f', "Syscall CREATE\n");
                ReadStringFromUser(r4, name, 16);
                DEBUG('f', "Var name: %s\n", name);
                ASSERT(fileSystem -> Create(name, 128));
                IncPC();
                break;
            }

            case SC_Read: {
                DEBUG('f', "Syscall READ\n");
                int read = -1;
                if(r6 == 1){
                    DEBUG('f', "Bad arguments\n");
                    ASSERT(false);
                }
                if(r6 == 0){
                    //char temp = buff[0];
                    for(int i = 0; i < r5; i++){
                        DEBUG('f', "READING!\n");
                        buff[i] = synchConsole -> ReadChar();
                        read = i;
                    }
                        WriteBufferToUser(buff, r4, r5);
                }
                else{
                    currentThread -> PrintList();
                    OpenFile *f1= currentThread -> getFile(r6);
                    if(f1 == NULL){
                        DEBUG('f',"Bad arguments\n");
                        ASSERT(false);
                    }
                    read = f1 -> Read(buff, r5);
                    WriteBufferToUser(buff, r4, read);
                }
                machine -> WriteRegister(2, read);
                IncPC();
                break;
            }

            case SC_Write: {
                DEBUG('f', "Syscall WRITE, (fid == %d)\n", r6);
                int write = -1;
                DEBUG('f', "r5 = %d\n", r5);
                if(r6 == 0){
                    DEBUG('f', "Bad arguments (r6 == 0)\n");
                    ASSERT(false);
                }
                if(r6 == 1){
                    ReadBufferFromUser(r4, buff, r5);
                    DEBUG('f', "Buff: %s\n", buff);
                    for(int i = 0; i < r5; i++){
                        char temp;
                        temp = buff[i];
                        synchConsole -> WriteChar(temp);
                        write = i;
                    }
                }
                else{
                    ReadBufferFromUser(r4, buff, r5);
                    OpenFile *f = currentThread -> getFile(r6);
                    if(f == NULL){
                        DEBUG('f', "Bad Arguments (f == NULL)\n");
                        ASSERT(false);
                    }
                    write = f -> Write(buff, (unsigned)r5); ///////
                }
                machine -> WriteRegister(2, write);
                IncPC();
                break;
            }

            case SC_Open: {
                DEBUG('f', "Syscall Open\n");
                ReadStringFromUser(r4, name, 128);
                OpenFile *f = fileSystem -> Open(name);
                if(f == NULL){
                    printf("Archivo inexistente\n");
                    ASSERT(false);
                }
                OpenFileId fid = currentThread -> addFile(f);
                DEBUG('f', "Abierto el archivo %d\n", fid);
                machine -> WriteRegister(2, fid);
                IncPC();
                break;
            }

            case SC_Close: {
                DEBUG('f', "Syscall Close\nCerrado el archivo %d\n", r4);
                currentThread -> removeFile(r4);
                IncPC();
                break;
            }

            case SC_Exit: {
                currentThread->FileClose(); //cierra los archivos abiertos
                currentThread -> Finish(r4); //termina con estado r4
                break;                      // No incrementa el PC
            }

            case SC_Join: {
                int ret;
                Thread *t = getThread(r4);
                DEBUG('q', "Haciendo join sobre el hilo %d, thread=%p\n", r4, t);
                t -> Join(&ret); //obtengo el hilo y hago el join
                machine -> WriteRegister(2, ret);
                removeThread(r4); //lo elimino de la lista
                IncPC();
                break;
            }

            case SC_Exec: {
                DEBUG('t', "SYSCALL EXEC\n");
                //printf("%d %d\n\n", r4, r5);
                ReadStringFromUser(r4, name, 12);
                OpenFile *f = fileSystem -> Open(name);
                AddressSpace *tas = new AddressSpace(f);
                Thread *t = new Thread(strdup(name), true); //ver como tratar args = NULL
                t -> space = tas;
                SpaceId pid = t -> getPid();
                DEBUG('q', "El hilo %d ha sido creado\n", pid);
                #ifdef VMEM
                    coremap->addAddrSpace(t->getPid(), t->space);
                #endif
                //leer args a kernel
                char **args = SaveArgs(r5); // Leo los argumentos
                t->Fork(StartProc, args);
                machine -> WriteRegister(2, pid); //Retorno el Pid del nuevo thread
                IncPC();
                break;
            }
        }
    } else if(which == PAGE_FAULT_EXCEPTION) {
        // printf("PAGE FAULT EXCEPTION\n");
        unsigned vaddr = machine -> ReadRegister(BAD_VADDR_REG);
        unsigned vpn = vaddr / PAGE_SIZE;
        DEBUG('m', "vpn number: %d\n", vpn);
        if(vaddr < 0 || vaddr >= ((currentThread -> space) ->  getNumPages())
        * PAGE_SIZE){
            printf("No more memory %d %d %d\n", which, type,
                        (currentThread -> space) ->  getNumPages() * PAGE_SIZE);
        }
        // TranslationEntry t = currentThread -> space -> getPageTable(vpn);
        #ifdef DEMAND_LOADING
            if((int)currentThread -> space -> getPageTable(vpn).physicalPage < 0)
                currentThread -> space -> loadVPN(vaddr);
        #endif
        #ifdef USE_TLB
            insertTLB(currentThread -> space -> getPageTable(vpn));
        #endif
    } else if(which == READ_ONLY_EXCEPTION){
        printf("READ ONLY EXCEPTION\n");
        currentThread->Finish(-1);
    } else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(false);
    }
}

void
StartProc(void *arg)
{
    char **args = (char**)arg;
    currentThread-> space -> InitRegisters();  // Set the initial register values.
    currentThread -> space -> RestoreState();   // Load page table register.
    WriteArgs(args);
    machine -> Run();
}

void
insertTLB(TranslationEntry t) { //Write machine -> tlb
    // printf("Writing tlb!\n");
    for(int i = 0; i < (int)TLB_SIZE; i++)
        if(!machine->tlb[i].valid){
            DEBUG('m', machine->tlb[i].valid ? "true\n" : "false\n");
            machine->tlb[i] = t;
            return;
        }
    // What do I do now?
    int vpn = machine -> tlb[penalizedPage].virtualPage;
    currentThread -> space -> getPageTable(vpn) = machine->tlb[penalizedPage]; // save penalized page
    machine->tlb[penalizedPage] = t;
    penalizedPage = penalizedPage == TLB_SIZE - 1 ? 0 : penalizedPage + 1;
}

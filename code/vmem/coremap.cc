#include "coremap.hh"

Coremap::Coremap(int nitems)
{
    ramStatus = new structCoremap[nitems];
    structCoremap memoryCell;
    memoryCell.virtualPage = -1;
    memoryCell.pid = -1;
    for(int i = 0; i < nitems; i++){
        ramStatus[i] = memoryCell;
    }
}

Coremap::~Coremap()
{

}

void
Coremap::set(unsigned phy, unsigned vpn, int pid)
{
    structCoremap memoryCell;
    memoryCell.virtualPage = vpn;
    memoryCell.pid = pid;
    ramStatus[phy] = memoryCell;
    // printf("Coremap::set %u, %d\n", ramStatus[phy].virtualPage, ramStatus[phy].pid);
}

void
Coremap::addAddrSpace(int pid, AddressSpace *space)
{
    if(pid >= MAX_PROCS){
        // machine->RaiseException(ILLEGAL_INSTR_EXCEPTION, -1);
        return;
    }
    threadAddrSpace[pid] = space;
    DEBUG('t', "\n\n***Addres space added from pid %d***\n\n\n", pid);
}

int
Coremap::Find(int Pid)
{
    return 0;
}

int
Coremap::selectVictim()
{
    return 0;
}

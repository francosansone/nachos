#include "coremap.hh"
#define COREMAP_DEBUG 0

Coremap::Coremap(int _nitems)
{
    #if COREMAP_DEBUG
        printf("Coremap::Coremap %d\n", _nitems);
    #endif
    this -> nitems = _nitems;
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
    #if COREMAP_DEBUG
        printf("Coremap::set %u, %d\n", ramStatus[phy].virtualPage, ramStatus[phy].pid);
    #endif
}

void
Coremap::addAddrSpace(int pid, AddressSpace *space)
{
    if(pid >= MAX_PROCS){
        // machine->RaiseException(ILLEGAL_INSTR_EXCEPTION, -1);
        return;
    }
    threadAddrSpace[pid] = space;
    #if COREMAP_DEBUG
        printf("\n\n***Addres space added from pid %d***\n\n\n", pid);
    #endif
}

int
Coremap::FindVictim(int pid, unsigned vpn)
{
    #if COREMAP_DEBUG
        printf("FindVictim %d\n", Random() % nitems);
    #endif
    int victim = Random() % nitems;
    #if COREMAP_DEBUG
        printf("Coremap::FindVictim %u\n",victim);
    #endif

    updateMemoryStatus(pid, victim, vpn);
    return victim;
}

void
Coremap::updateMemoryStatus(int currentPid, int victim, unsigned vpn)
{
    int penalizedPid = ramStatus[victim].pid;
    if(penalizedPid == currentPid){
        #if 1//COREMAP_DEBUG
            printf("update TLB\n");
        #endif
        threadAddrSpace[currentPid]->invalidPageInTlb(victim, vpn);
    }
    if(threadAddrSpace[penalizedPid] != NULL){
        #if COREMAP_DEBUG
            printf("updatePageTable\n");
        #endif
        threadAddrSpace[penalizedPid]->saveInSwap(victim);
        // threadAddrSpace[penalizedPid] -> updatePageTable(victim);
    }
    set(victim, vpn, currentPid);
}

int
Coremap::selectVictim()
{
    return 0;
}

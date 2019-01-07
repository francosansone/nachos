#include "coremap.hh"
#define COREMAP_DEBUG 1

static unsigned penalizedPage = 0;

Coremap::Coremap(int _nitems)
{
    this->nitems = _nitems;
    mainMemoryStatus = new structCoremap[nitems];
    structCoremap memoryCell;
    memoryCell.virtualPage = -1;
    memoryCell.pid = -1;
    for(int i = 0; i < nitems; i++){
        mainMemoryStatus[i] = memoryCell;
    }
}

Coremap::~Coremap()
{

}

void
Coremap::set(unsigned phy, unsigned vpn, int pid)
{
    printf("Coremap::set\n");
    structCoremap memoryCell;
    memoryCell.virtualPage = vpn;
    memoryCell.pid = pid;
    mainMemoryStatus[phy] = memoryCell;
    #if 0 //COREMAP_DEBUG
        printf("Coremap::set %u, %d\n", mainMemoryStatus[phy].virtualPage, mainMemoryStatus[phy].pid);
    #endif
}

void
Coremap::addAddrSpace(int pid, AddressSpace *space)
{
    printf("Coremap::addAddrSpace\n");
    if(pid >= MAX_PROCS){
        // machine->RaiseException(ILLEGAL_INSTR_EXCEPTION, -1);
        return;
    }
    threadAddrSpace[pid] = space;
    DEBUG('t', "\n\n***Addres space added from pid %d***\n\n\n", pid);
}

int
Coremap::FindVictim()
{
    printf("Coremap::FindVictim\n");
    unsigned physicalPage = penalizedPage;
    unsigned virtualPage = mainMemoryStatus[penalizedPage].virtualPage;
    int pid = mainMemoryStatus[penalizedPage].pid;
    #if COREMAP_DEBUG
        printf("FindVictim: %d %d %d\n", physicalPage, virtualPage, pid);
    #endif
    if(threadAddrSpace[pid] != NULL){
        threadAddrSpace[pid]->saveInSwap(virtualPage);
    }
    if((int)penalizedPage == nitems - 1)
        penalizedPage = 0;
    else
        penalizedPage++;
    return physicalPage;
}

int
Coremap::getFromSwap(unsigned vpn, int pid, int phy)
{
    printf("Coremap::getFromSwap\n");
    if(phy == -1){
        phy = FindVictim();
    }
    set(phy, vpn, pid);
    threadAddrSpace[pid]->loadFromSwap(vpn, phy);
    return phy;
}

int
Coremap::selectVictim()
{
    printf("Coremap::selectVictim\n");
    return 0;
}

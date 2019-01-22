#include "lru.hh"

#define LRU_DEBUG 0
typedef List<unsigned> ListPages;

Lru::Lru(int _nitems) : Coremap(_nitems)
{
    #if LRU_DEBUG
        printf("Lru::Lru %d", _nitems);
    #endif
    nitems = _nitems;
    coremap = new Coremap(_nitems);
    listPages = new ListPages;
    for(int i = 0; i < _nitems; i++)
        listPages->Append(i);
}

Lru::~Lru()
{
    delete coremap;
}

void
Lru::set(unsigned phy, unsigned vpn, int pid)
{
    ListPages *newListPages = new ListPages;
    for(int i = 0; i < nitems; i++){
        unsigned pageNum = listPages -> Remove();
        if(pageNum != phy)
            newListPages -> Append(pageNum);
    }
    newListPages->Append(phy);
    #if LRU_DEBUG
        printf("Al fondo! %u\n", phy);
    #endif
    listPages = newListPages;
    coremap->set(phy, vpn, pid);
}

void
Lru::addAddrSpace(int pid, AddressSpace *space)
{
    coremap->addAddrSpace(pid,space);
}

int
Lru::FindVictim(int pid, unsigned vpn)
{
    unsigned victim = listPages->Remove();
    listPages->Append(victim);
    coremap->updateMemoryStatus(pid, victim, vpn);
    #if LRU_DEBUG
        printf("Lru::FindVictim %d\n", victim);
    #endif
    return victim;
}

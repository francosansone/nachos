#ifndef NACHOS_VMEM_LRU__HH
#define NACHOS_VMEM_LRU__HH

#include "coremap.hh"
#include "machine/machine.hh"
#include "userprog/address_space.hh"
#include "threads/list.hh"
#define MAX_PROCS 100

typedef List<unsigned> ListPages;

class Lru : public Coremap {
public:
    Lru(int ntimes);
    ~Lru();
    void set(unsigned phy, unsigned vpn, int pid);
    void addAddrSpace(int pid, AddressSpace *space);
    int FindVictim(int Pid, unsigned vpn);
    int selectVictim();
    //
private:
    int nitems;
    Coremap *coremap;
    List<unsigned> *listPages;
};

#endif

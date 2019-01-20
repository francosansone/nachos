#ifndef NACHOS_VMEM_COREMAP__HH
#define NACHOS_VMEM_COREMAP__HH

#include "machine/machine.hh"
#include "userprog/address_space.hh"
#define MAX_PROCS 100

typedef struct _structCoremap{
    //The page number in virtual memory
    unsigned virtualPage;

    //The Pid that uses this memory page
    int pid;
}structCoremap;

/// The following class defines a coremap

class Coremap {

public:

    Coremap(int nitems);
    ~Coremap();
    void set(unsigned phy, unsigned vpn, int pid);
    void addAddrSpace(int pid, AddressSpace *space);
    int FindVictim(int Pid, unsigned vpn);
    int selectVictim();
    //
private:
    int nitems;
    structCoremap *ramStatus;
    AddressSpace *threadAddrSpace[MAX_PROCS];
};

#endif

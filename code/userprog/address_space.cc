/// Routines to manage address spaces (executing user programs).
///
/// In order to run a user program, you must:
///
/// 1. Link with the `-N -T 0` option.
/// 2. Run `coff2noff` to convert the object file to Nachos format (Nachos
///    object code format is essentially just a simpler version of the UNIX
///    executable object code format).
/// 3. Load the NOFF file into the Nachos file system (if you have not
///    implemented the file system yet, you do not need to do this last
///    step).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "threads/system.hh"
#include "bitmap.hh"
#include "filesys/open_file.hh"

#define BITS_OFFSET 7
#define OR_OFFSET 127 //128 direcciones por pagina
#define ADDRESS_SPACE_DEBUG 1

BitMap *bitmap = new BitMap(NUM_PHYS_PAGES); //cantidad de paginas

/// Do little endian to big endian conversion on the bytes in the object file
/// header, in case the file was generated on a little endian machine, and we
/// are re now running on a big endian machine.
static void
SwapHeader(NoffHeader *noffH)
{
    noffH->noffMagic              = WordToHost(noffH->noffMagic);
    noffH->code.size              = WordToHost(noffH->code.size);
    noffH->code.virtualAddr       = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr        = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size          = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr   = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr    = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size        = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr =
      WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr  = WordToHost(noffH->uninitData.inFileAddr);
}

/// Create an address space to run a user program.
///
/// Load the program from a file `executable`, and set everything up so that
/// we can start executing user instructions.
///
/// Assumes that the object code file is in NOFF format.
///
/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
///
/// * `executable` is the file containing the object code to load into
///   memory.
AddressSpace::AddressSpace(OpenFile *exec)
{
    executable = exec;
    unsigned   size;

    executable->ReadAt((char *) &noffH, sizeof noffH, 0);
    if (noffH.noffMagic != NOFFMAGIC &&
          WordToHost(noffH.noffMagic) == NOFFMAGIC)
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // How big is address space?

    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size
           + USER_STACK_SIZE;
      // We need to increase the size to leave room for the stack.
    numPages = divRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE;

    ASSERT(numPages <= NUM_PHYS_PAGES);
      // Check we are not trying to run anything too big -- at least until we
      // have virtual memory.

    DEBUG('a', "Initializing address space, num pages %u, size %u\n",
          numPages, size);

    // First, set up the translation.

    pageTable = new TranslationEntry[numPages];

    for (unsigned i = 0; i < numPages; i++) {
        pageTable[i].virtualPage  = i;
          // For now, virtual page number = physical page number.
          // Cambiamos para cargar mas de un programa en mainMemory
        #ifdef DEMAND_LOADING
            pageTable[i].physicalPage = -1;
            pageTable[i].valid = false;
        #else
            pageTable[i].physicalPage = bitmap -> Find();
            DEBUG('a', "%d ", (int)pageTable[i].physicalPage);
            ASSERT((int)pageTable[i].physicalPage != -1);
            #ifdef VMEM
            coremap -> set(pageTable[i].physicalPage, i, currentThread->getPid());
            #endif
            pageTable[i].valid        = true;
            pageTable[i].use          = false;
            pageTable[i].dirty        = false;
            pageTable[i].readOnly     = false;
            //memset(&(machine->mainMemory[pageTable[i].physicalPage * PAGE_SIZE]), 0, PAGE_SIZE);
            bzero(&(machine->mainMemory[pageTable[i].physicalPage * PAGE_SIZE]),PAGE_SIZE);
        #endif
    }

    #ifndef DEMAND_LOADING
        // Then, copy in the code and data segments into memory.
        DEBUG('a', "Initializing code segment, size %d\n", noffH.code.size);
        for (int i=0; i<noffH.code.size; i++) {
            char c;
            executable->ReadAt(&c, 1, noffH.code.inFileAddr + i); //leo de 1 byte el seg de codigo
            int virtualAddr = noffH.code.virtualAddr + i; //DIRECCION virtual correspondiente a ese byte de codigo
            //ahora desgloso la direccion virtual
            int virtualPageNum = virtualAddr / PAGE_SIZE;    //numero de pagina de la direccion
            int offset = virtualAddr % PAGE_SIZE;             //offset de la direccion
            int physicalPageNum = (pageTable[virtualPageNum].physicalPage * PAGE_SIZE) + offset;    //pagina fisica de la pagina virtual
            DEBUG('a', "Leo el bloque de codigo %d\n",physicalPageNum);
            machine->mainMemory[physicalPageNum] = c; //escribo en la pagina fisica correspondiente
        }


        DEBUG('a', "Initializing data segment, size %d\n", noffH.initData.size);
        for (int i=0; i<noffH.initData.size; i++) {
            char c;
            executable->ReadAt(&c, 1, noffH.initData.inFileAddr + i); //leo de 1 byte el seg de codigo
            int virtualAddr = noffH.initData.virtualAddr + i; //DIRECCION virtual correspondiente a ese byte de codigo
            //ahora desgloso la direccion virtual
            int virtualPageNum = virtualAddr / PAGE_SIZE;    //numero de pagina de la direccion
            int offset = virtualAddr % PAGE_SIZE;             //offset de la direccion
            int physicalPageNum = (pageTable[virtualPageNum].physicalPage * PAGE_SIZE);    //pagina fisica de la pagina virtual
            int physicalAddrNum = physicalPageNum + offset;  //direccion fisica
            DEBUG('a',"Leo el bloque de datos %d\n", physicalAddrNum);
            machine->mainMemory[physicalAddrNum] = c; //escribo en la pagina fisica correspon
        }
    #endif
}

void
AddressSpace::loadVPNFromBinary(int vaddr)
{
    Segment segment;
    bool uninitData = false;
    int vpn = vaddr / PAGE_SIZE;
    int ppn = bitmap -> Find();
    if(ppn == -1){
        #if ADDRESS_SPACE_DEBUG
            printf("Here we need swap\n");
        #endif
        #ifdef VMEM
            ppn = coremap->FindVictim(currentThread->getPid(), vpn);
        #endif
    }
    else{
        #ifdef VMEM
            coremap -> set(ppn, vpn, currentThread->getPid());
        #endif
    }
    pageTable[vpn].physicalPage = ppn;
    if(vaddr >= noffH.code.virtualAddr){
            segment = noffH.code;
        }
    else if(vaddr >= noffH.initData.virtualAddr){
            segment = noffH.initData;
        }
    else{
        segment = noffH.uninitData;
        uninitData = true;
    }
    int readed = vpn*PAGE_SIZE;
    for (int i = 0;
            (i < (int)PAGE_SIZE)
                && (((int)executable->Length() > (i+ segment.inFileAddr + readed - segment.virtualAddr)));
             i++){
        char c = 0;
        if(!uninitData) // Load data
            executable->ReadAt(&c, 1, i + segment.inFileAddr + readed - segment.virtualAddr);
        int physicalPageOffset = (pageTable[vpn].physicalPage * PAGE_SIZE);
        int physicalAddrNum = physicalPageOffset + i;
        machine->mainMemory[physicalAddrNum] = c;
    }
    pageTable[vpn].valid = true;
    pageTable[vpn].readOnly = false;
    pageTable[vpn].dirty = false;
    pageTable[vpn].use = false;
}

#ifdef VMEM
void
AddressSpace::createSwapFile(int pid)
{
    name[0] = 'S';
    name[1] = 'W';
    name[2] = 'A';
    name[3] = 'P';
    name[4] = '.';
    //support until 99 process
    #if ADDRESS_SPACE_DEBUG
        printf("AddressSpace::createSwapFile\n");
    #endif
    if(pid % 10 > 10){
        char _pid = pid + '0';
        name[5] = _pid;
        name[6] = '\0';
    }
    else{
        int tens = pid / 10;
        printf("%d, %c", tens, tens + '0');
        name[5] =  tens + '0';
        int unit = pid % 10;
        name[6] = unit + '0';
        name[7] = '\0';
    }
    DEBUG('t', "Swap file created %s\n", name);
    fileSystem -> Create(name, NUM_PHYS_PAGES*PAGE_SIZE);
    #if ADDRESS_SPACE_DEBUG
        printf("created %s!!\n", name);
    #endif
}

void
AddressSpace::loadFromSwap(unsigned vpn)
{
    #if ADDRESS_SPACE_DEBUG
        printf("*** loadFromSwap %s ** \n", name);
    #endif
    int readFrom = vpn*PAGE_SIZE;
    OpenFile *f = fileSystem -> Open(name);
    unsigned phy = bitmap -> Find();
    if((int)phy == -1){
        phy = coremap -> FindVictim(currentThread->getPid(), vpn);
    }
    else{
        coremap->set(phy, vpn, currentThread->getPid());
    }
    unsigned physicalPageOffset = phy*PAGE_SIZE;
    for(unsigned i = 0; i < PAGE_SIZE; i++){
        char c = 0;
        f->ReadAt(&c, 1, readFrom + i);
        machine -> mainMemory[physicalPageOffset + i] = c;
    }
    delete f;
    pageTable[vpn].physicalPage = phy;
}

void
AddressSpace::saveInSwap(unsigned phy)
{
    #if ADDRESS_SPACE_DEBUG
        printf("AddressSpace::saveInSwap %u\n", phy);
    #endif
    if(pageTable == NULL)
        return;
    if((int)phy < 0 || (unsigned)phy >= PAGE_SIZE*NUM_PHYS_PAGES){
        #if ADDRESS_SPACE_DEBUG
            printf("notSwap %d\n", phy);
        #endif
        return;
    }
    unsigned physicalPageOffset = phy*PAGE_SIZE;
    unsigned vpn;
    for(unsigned i = 0; i < NUM_PHYS_PAGES; i++){
        if(pageTable[i].physicalPage == phy){
            vpn = i;
            break;
        }
    }
    #if ADDRESS_SPACE_DEBUG
        printf("AddressSpace::saveInSwap %u\n", vpn);
    #endif
    int writeFrom = vpn*PAGE_SIZE;
    OpenFile *f = fileSystem -> Open(name);
    #if ADDRESS_SPACE_DEBUG
        printf("Write in file: %s", name);
    #endif
    if(name == NULL){
        printf("name is null");
        return;
    }
    for(unsigned i = 0; i < PAGE_SIZE; i++){
        char c = 0;
        c = machine->mainMemory[physicalPageOffset + i];
        f->WriteAt(&c, 1, writeFrom + i);
    }
    delete f;
    updatePageTable(phy);
}

void
AddressSpace::invalidPageInTlb(unsigned penalized, unsigned newVpn)
{
    #if ADDRESS_SPACE_DEBUG
        printf("AddressSpace::invalidPageInTlb penalized: %d new: %d\n", penalized, newVpn);
    #endif
    #ifdef USE_TLB
        for(int i = 0; i <  (int)TLB_SIZE; i++){
            if(machine->tlb[i].physicalPage == penalized){
                machine->tlb[i].virtualPage = newVpn;
            }
        }
    #endif
}

void
AddressSpace::updatePageTable(unsigned ppn)
{
    #if ADDRESS_SPACE_DEBUG
        printf("AddressSpace::updatePageTable ppn: %u\n", ppn);
    #endif
    for (unsigned i = 0; i < numPages; i++) {
        if(pageTable[i].physicalPage == ppn)
            pageTable[i].physicalPage = -2;
    }
}

#endif

/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace()
{
    delete [] pageTable;
}

/// Set the initial values for the user-level register set.
///
/// We write these directly into the “machine” registers, so that we can
/// immediately jump to user code.  Note that these will be saved/restored
/// into the `currentThread->userRegisters` when this thread is context
/// switched out.
void
AddressSpace::InitRegisters()
{
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++)
        machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of `Start`.
    machine->WriteRegister(PC_REG, 0);

    // Need to also tell MIPS where next instruction is, because of branch
    // delay possibility.
    machine->WriteRegister(NEXT_PC_REG, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we do not
    // accidentally reference off the end!
    machine->WriteRegister(STACK_REG, numPages * PAGE_SIZE - 16);
    DEBUG('a', "Initializing stack register to %u\n",
          numPages * PAGE_SIZE - 16);
}

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
///
/// For now, nothing!
void AddressSpace::SaveState()
{
    #ifdef USE_TLB
        for(unsigned i = 0; i < TLB_SIZE; i++){
            // printf("SaveUserState %u\n", i);
            if(machine->tlb[i].valid){
                pageTable[machine->tlb[i].virtualPage] = machine->tlb[i];
                // tempSavedTlb[i] = machine->tlb[i];
            }
        }
    #endif
}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void AddressSpace::RestoreState()
{
    machine->pageTable     = pageTable;
    machine->pageTableSize = numPages;
    #ifdef USE_TLB
        for(unsigned i = 0; i < TLB_SIZE; i++){
            // printf("SaveUserState %u\n", i);
            machine->tlb[i].valid = false;
        }
    #endif
}

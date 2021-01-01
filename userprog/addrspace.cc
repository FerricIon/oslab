// addrspace.cc
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "addrspace.h"
#include "copyright.h"
#include "noff.h"
#include "system.h"
#ifdef HOST_SPARC
#include <strings.h>
#endif

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void SwapHeader(NoffHeader *noffH) {
  noffH->noffMagic = WordToHost(noffH->noffMagic);
  noffH->code.size = WordToHost(noffH->code.size);
  noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
  noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
  noffH->initData.size = WordToHost(noffH->initData.size);
  noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
  noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
  noffH->uninitData.size = WordToHost(noffH->uninitData.size);
  noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
  noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

AddrSpace::AddrSpace(AddrSpace *parentSpace) {
  numPages = parentSpace->numPages;
  spaceId = parentSpace->spaceId;
  pageTable = new TranslationEntry[numPages];
  for (int i = 0; i < numPages; ++i)
    pageTable[i] = parentSpace->pageTable[i];
  ReallocateStack();
#ifdef USE_TLB
  tlb = new TranslationEntry[TLBSize];
  tlbCounter = new unsigned char[TLBSize];
  for (int i = 0; i < TLBSize; ++i) {
    tlb[i].valid = FALSE;
    tlbCounter[i] = 0;
  }
#endif
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable) {
  NoffHeader noffH;
  unsigned int i, size;

  executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
  if ((noffH.noffMagic != NOFFMAGIC) &&
      (WordToHost(noffH.noffMagic) == NOFFMAGIC))
    SwapHeader(&noffH);
  ASSERT(noffH.noffMagic == NOFFMAGIC);

  // how big is address space?
  size = noffH.code.size + noffH.initData.size + noffH.uninitData.size +
         UserStackSize; // we need to increase the size
                        // to leave room for the stack
  numPages = divRoundUp(size, PageSize);
  size = numPages * PageSize;

  ASSERT(numPages <= NumPhysPages); // check we're not trying
                                    // to run anything too big --
                                    // at least until we have
                                    // virtual memory
  if (numPages > allocator->Available()) {
    printf("No enough empty pages...\n");
    ASSERT(FALSE);
  }

  DEBUG('a', "Initializing address space, num pages %d, size %d\n", numPages,
        size);
  // first, set up the translation
  pageTable = new TranslationEntry[numPages];
  for (i = 0; i < numPages; i++) {
    pageTable[i].virtualPage = i; // for now, virtual page # = phys page #
    pageTable[i].physicalPage = allocator->AllocPage();
    pageTable[i].valid = TRUE;
    pageTable[i].use = FALSE;
    pageTable[i].dirty = FALSE;
    pageTable[i].readOnly = FALSE; // if the code segment was entirely on
                                   // a separate page, we could set its
                                   // pages to be read-only
    // zero out the entire address space, to zero the unitialized data segment
    // and the stack segment
    bzero(&machine->mainMemory[pageTable[i].physicalPage * PageSize], PageSize);
  }
#ifdef USE_TLB
  tlb = new TranslationEntry[TLBSize];
  tlbCounter = new unsigned char[TLBSize];
  for (i = 0; i < TLBSize; ++i) {
    tlb[i].valid = FALSE;
    tlbCounter[i] = 0;
  }
#endif

  // then, copy in the code and data segments into memory
  if (noffH.code.size > 0) {
    DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
          noffH.code.virtualAddr, noffH.code.size);
    ManualReadTranslation(executable, noffH.code.virtualAddr, noffH.code.size,
                          noffH.code.inFileAddr);
  }
  if (noffH.initData.size > 0) {
    DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",
          noffH.initData.virtualAddr, noffH.initData.size);
    ManualReadTranslation(executable, noffH.initData.virtualAddr,
                          noffH.initData.size, noffH.initData.inFileAddr);
  }
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace() {
  // Free user stack only.
  for (int i = numPages - UserStackSize / PageSize; i < numPages; ++i) {
    allocator->FreePage(pageTable[i].physicalPage);
  }
  delete[] pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void AddrSpace::InitRegisters() {
  int i;

  for (i = 0; i < NumTotalRegs; i++)
    machine->WriteRegister(i, 0);

  // Initial program counter -- must be location of "Start"
  machine->WriteRegister(PCReg, 0);

  // Need to also tell MIPS where next instruction is, because
  // of branch delay possibility
  machine->WriteRegister(NextPCReg, 4);

  // Set the stack register to the end of the address space, where we
  // allocated the stack; but subtract off a bit, to make sure we don't
  // accidentally reference off the end!
  machine->WriteRegister(StackReg, numPages * PageSize - 16);
  DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() {
#ifdef USE_TLB
  memcpy(tlb, machine->tlb, sizeof(TranslationEntry) * TLBSize);
#endif
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() {
#ifdef USE_TLB
  memcpy(machine->tlb, tlb, sizeof(TranslationEntry) * TLBSize);
#else
  machine->pageTable = pageTable;
  machine->pageTableSize = numPages;
#endif
}

//----------------------------------------------------------------------
// AddrSpace::LookupPageTable
//  A thread saves its page table in the addr space if tlb is used.
//  Lookup the entry when tlb missing using vpn. Set the dirty bit
//  when changing out.
//----------------------------------------------------------------------

TranslationEntry *AddrSpace::LookupPageTable(int vpn, TranslationEntry *pte) {
  if (vpn < 0 || vpn >= numPages)
    return NULL;
  if (pte != NULL && pte->valid)
    pageTable[pte->virtualPage].dirty = pte->dirty;
  return pageTable + vpn;
}

//----------------------------------------------------------------------
// AddrSpace::UpdateTlbCounter
//  Used by aging algorithm. Should be updated on timer interrupts.
//----------------------------------------------------------------------

void AddrSpace::UpdateTlbCounter() {
  for (int i = 0; i < TLBSize; ++i) {
    tlbCounter[i] =
        ((unsigned char)machine->tlb[i].use << 7) | (tlbCounter[i] >> 1);
    machine->tlb[i].use = FALSE;
  }
}

//----------------------------------------------------------------------
// AddrSpace::TlbIndex
//  Find the index with the smallest counter whose PTE should be
//  changed out. Set the counter to 0xFF for the newer coming PTE
//  in case of thrashing.
//----------------------------------------------------------------------

int AddrSpace::TlbIndex() {
  int index = 0;
  for (int i = 0; i < TLBSize; ++i)
    if (tlbCounter[i] < tlbCounter[index])
      index = i;
  tlbCounter[index] = 0xFF;
  return index;
}

//----------------------------------------------------------------------
// AddrSpace::ManualReadTranslation
//  Used for copying executable file data into main memory. translate
//  cannot be called here, so manual handling is needed. Copy the data
//  one page by one page to ensure continuity in virtual memory.
//----------------------------------------------------------------------

void AddrSpace::ManualReadTranslation(OpenFile *executable, int virtAddr,
                                      int size, int fileAddr) {
  int addr, lengthToRead;

  for (addr = virtAddr; size != 0;) {
    int vpn = virtAddr / PageSize;
    int ppn = pageTable[vpn].physicalPage;
    int offset = virtAddr - vpn * PageSize;
    lengthToRead = std::min(size, PageSize - offset);
    executable->ReadAt(&(machine->mainMemory[ppn * PageSize + offset]),
                       lengthToRead, fileAddr);
    virtAddr += lengthToRead;
    size -= lengthToRead;
    fileAddr += lengthToRead;
  }
}

void AddrSpace::ReallocateStack() {
  for (int i = numPages - UserStackSize / PageSize; i < numPages; ++i) {
    pageTable[i].physicalPage = allocator->AllocPage();
    pageTable[i].valid = TRUE;
    pageTable[i].use = FALSE;
    pageTable[i].dirty = FALSE;
    pageTable[i].readOnly = FALSE;
    bzero(&machine->mainMemory[pageTable[i].physicalPage * PageSize], PageSize);
  }
}
void AddrSpace::FreeSharedPages() {
  for (int i = 0; i < numPages - UserStackSize / PageSize; ++i)
    allocator->FreePage(pageTable[i].physicalPage);
}

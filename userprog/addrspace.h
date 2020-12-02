// addrspace.h
//	Data structures to keep track of executing user programs
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"

#define UserStackSize 1024 // increase this as necessary!

class AddrSpace
{
public:
  AddrSpace(OpenFile *executable); // Create an address space,
                                   // initializing it with the program
                                   // stored in the file "executable"
  ~AddrSpace();                    // De-allocate an address space

  void InitRegisters(); // Initialize user-level CPU registers,
                        // before jumping to user code

  void SaveState();    // Save/restore address space-specific
  void RestoreState(); // info on a context switch

  TranslationEntry *LookupPageTable(
      int vpn, TranslationEntry *pte = NULL); // Lookup an entry in the
                                              // thread saving page table
  void UpdateTlbCounter();                    // Called when handling a TimerInt
  int TlbIndex();                             // Get the index with minimal
                                              // counter value and set to 0xFF

private:
  TranslationEntry *tlb;
  unsigned char *tlbCounter;
  TranslationEntry *pageTable; // Thread saving when using tlb
  unsigned int numPages;       // Number of pages in the virtual
                               // address space

  void ManualReadTranslation(OpenFile *executable,
                             int virtAddr, int size, int fileAddr);
};

#endif // ADDRSPACE_H

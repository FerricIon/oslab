// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "syscall.h"
#include "system.h"

//----------------------------------------------------------------------
// FetchUserString
//  Fetch a string (segment of data) in user space into a buffer of
//  kernel space. At most n bytes will be fetched.
//
//  "dst" is address of the destination kernal buffer
//  "src" is virtual address of the user space data to be fetched
//  at most "n" bytes of data will be fetched
//  if "delimiter" is set, the fetch stops at the first '\0', and '\0'
//    will be added to the end of the data
//----------------------------------------------------------------------

void FetchUserString(char *dst, const char *src, int n, int delimiter = 1) {
  for (int i = 0; i < n; ++i) {
    while (!machine->ReadMem((int)(src + i), 1, (int *)(dst + i)))
      ;
    if (delimiter && dst[i] == '\0')
      break;
  }
  if (delimiter)
    dst[n - 1] = '\0';
}

//----------------------------------------------------------------------
// PutUserString
//  Put a string (segment of data) in a kernel space buffter into
//  somewhere in the user space. At most n bytes will be put.
//
//  "dst" is virtual address of the destination memory
//  "src" is address of the kernel space buffer
//  at most "n" bytes of data will be put
//  if "delimiter" is set, the put stops at the first '\0', and '\0'
//    will be added to the end of the data
//----------------------------------------------------------------------

void PutUserString(char *dst, const char *src, int n, int delimiter = 1) {
  for (int i = 0; i < n; ++i) {
    while (!machine->WriteMem((int)(dst + i), 1, src[i]))
      ;
    if (delimiter && src[i] == '\0')
      break;
  }
  if (delimiter)
    while (!machine->WriteMem((int)(dst + n - 1), 1, '\0'))
      ;
}

void SyscallIncPC() {
  int pcAfter = machine->registers[NextPCReg] + 4;
  machine->registers[PrevPCReg] = machine->registers[PCReg];
  machine->registers[PCReg] = machine->registers[NextPCReg];
  machine->registers[NextPCReg] = pcAfter;
}

void ExecHandler(int dummy) {
  currentThread->space->InitRegisters();
  currentThread->space->RestoreState();
  machine->Run();
}

void ForkHandler(int function) {
  currentThread->space->InitRegisters();
  currentThread->space->RestoreState();
  machine->WriteRegister(PrevPCReg, 0);
  machine->WriteRegister(PCReg, function);
  machine->WriteRegister(NextPCReg, function + 4);
  machine->Run();
}

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions
//	are in machine.h.
//----------------------------------------------------------------------

void ExceptionHandler(ExceptionType which) {
  int type = machine->ReadRegister(2);

  if (which == SyscallException) {
    switch (type) {
    case SC_Halt: {
      DEBUG('a', "Shutdown, initiated by user program.\n");
      interrupt->Halt();
      break;
    }
    case SC_Exit: {
      printf("%s exit with code %d.\n", currentThread->getName(),
             machine->ReadRegister(4));
      for (auto &entry : spaceTable)
        if (entry.id == currentThread->space->spaceId) {
          --entry.refCount;
          entry.status = machine->ReadRegister(4);
          if (!entry.refCount)
            entry.space->FreeSharedPages();
          break;
        }
      currentThread->Finish();
      break;
    }
    case SC_Create: {
      char name[32];
      FetchUserString(name, (char *)machine->ReadRegister(4), 32);
      fileSystem->Create(name, 0);
      break;
    }
    case SC_Open: {
      char name[32];
      FetchUserString(name, (char *)machine->ReadRegister(4), 32);
      OpenFile *openFile = fileSystem->Open(name);
      bool found = false;
      for (OpenFileId i = 2; i < 128; ++i)
        if (tableOpenFile[i] == NULL) {
          found = true;
          tableOpenFile[i] = openFile;
          machine->WriteRegister(2, i);
          break;
        }
      // Cannot open more files.
      ASSERT(found);
      break;
    }
    case SC_Close: {
      OpenFileId id = machine->ReadRegister(4);
      ASSERT(tableOpenFile[id]);
      delete tableOpenFile[id];
      tableOpenFile[id] = NULL;
      break;
    }
    case SC_Write: {
      int size = machine->ReadRegister(5);
      OpenFileId id = machine->ReadRegister(6);
      char buffer[size];
      FetchUserString(buffer, (char *)machine->ReadRegister(4), size, 0);
      OpenFile *openFile = tableOpenFile[id];
      ASSERT(openFile);
      openFile->Write(buffer, size);
      break;
    }
    case SC_Read: {
      int size = machine->ReadRegister(5);
      OpenFileId id = machine->ReadRegister(6);
      char buffer[size];
      OpenFile *openFile = tableOpenFile[id];
      ASSERT(openFile);
      int read = openFile->Read(buffer, size);
      machine->WriteRegister(2, read);
      PutUserString((char *)machine->ReadRegister(4), buffer, read, 0);
      break;
    }
    case SC_Exec: {
      char name[32];
      FetchUserString(name, (char *)machine->ReadRegister(4), 32);
      OpenFile *executable = fileSystem->Open(name);
      ASSERT(executable);

      Thread *t = new Thread("exec thread");
      t->space = new AddrSpace(executable);
      t->space->spaceId = nextSpaceId++;
      spaceTable.push_back({0, t->space->spaceId, 0, 1, t->space});
      delete executable;
      t->Fork(ExecHandler, 0);
      machine->WriteRegister(2, t->space->spaceId);
      break;
    }
    case SC_Fork: {
      Thread *t = new Thread("fork thread");
      t->space = new AddrSpace(currentThread->space);
      for (auto &entry : spaceTable)
        if (t->space->spaceId == entry.id) {
          ++entry.refCount;
          break;
        }
      t->Fork(ForkHandler, (void *)machine->ReadRegister(4));
      break;
    }
    case SC_Yield: {
      currentThread->Yield();
      printf("%s yields\n", currentThread->getName());
      break;
    }
    case SC_Join: {
      int id = machine->ReadRegister(4);
      bool found = false;
      for (auto &entry : spaceTable)
        if (id == entry.id) {
          ++entry.joinCount;
          found = true;
          break;
        }
      ASSERT(found);
      bool block = true;
      while (block) {
        for (int i = 0; i < spaceTable.size(); ++i)
          if (id == spaceTable[i].id) {
            auto &entry = spaceTable[i];
            if (entry.refCount)
              currentThread->Yield();
            else {
              entry.joinCount--;
              machine->WriteRegister(2, entry.status);
              if (!entry.joinCount) {
                spaceTable.erase(spaceTable.begin() + i);
              }
              block = false;
            }
            break;
          }
      }
      break;
    }
    default:
      printf("Unexpected syscall code %d\n", type);
      ASSERT(FALSE);
      break;
    }
    SyscallIncPC();
  }
#ifdef USE_TLB
  else if (which == PageFaultException) {
    DEBUG('a', "Page fault, badVAddr: %d.\n", machine->registers[BadVAddrReg]);
    static int pagefaultCounter;
    printf("PageFault #%d, bad virtual addr: 0x%x\n", ++pagefaultCounter,
           machine->registers[BadVAddrReg]);

    unsigned int virtAddr = machine->registers[BadVAddrReg];
    unsigned int vpn = (unsigned)virtAddr / PageSize;

    unsigned int index = 0;

#ifdef TLB_FIFO
    static int lastIndex = TLBSize - 1;
    lastIndex = index = (lastIndex + 1) % TLBSize;
#elif defined(TLB_AGING)
    index = currentThread->space->TlbIndex();
#else
    ASSERT(FALSE);
#endif

    // Set the dirty bit as well
    TranslationEntry *entry =
        currentThread->space->LookupPageTable(vpn, &machine->tlb[index]);
    if (entry) {
      machine->tlb[index] = *entry;
    } else {
      printf("Unexpected virtual address 0x%x\n", virtAddr);
      ASSERT(FALSE);
    }
  }
#endif
  else {
    printf("Unexpected user mode exception %d %d\n", which, type);
    ASSERT(FALSE);
  }
}

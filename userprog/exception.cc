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
#include "system.h"
#include "syscall.h"

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

void ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if (which == SyscallException)
    {
        if (type == SC_Halt)
        {
            DEBUG('a', "Shutdown, initiated by user program.\n");
            interrupt->Halt();
        }
        else if (type == SC_Exit)
        {
            printf("%s exit with code %d.\n", currentThread->getName(), machine->ReadRegister(4));
            currentThread->Finish();
        }
        else
        {
            printf("Unexpected syscall %d\n", type);
            ASSERT(FALSE);
        }
    }
#ifdef USE_TLB
    else if (which == PageFaultException)
    {
        DEBUG('a', "Page fault, badVAddr: %d.\n", machine->registers[BadVAddrReg]);
        static int pagefaultCounter;
        printf("PageFault #%d, bad virtual addr: 0x%x\n",
               ++pagefaultCounter, machine->registers[BadVAddrReg]);

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
        TranslationEntry *entry = currentThread->space->LookupPageTable(
            vpn, &machine->tlb[index]);
        if (entry)
        {
            machine->tlb[index] = *entry;
        }
        else
        {
            printf("Unexpected virtual address 0x%x\n", virtAddr);
            ASSERT(FALSE);
        }
    }
#endif
    else
    {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}

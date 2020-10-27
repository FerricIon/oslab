// threadtest.cc
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield,
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "elevatortest.h"

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// ts
//  Print current threads as Linux ps does.
//  Shows tid, uid, state and name for each thread
//----------------------------------------------------------------------

void ts()
{
    puts("TID UID STAT NAME");
    for (int i = 0; i < MAX_THREADS; ++i)
    {
        if (tidTable[i] == NULL)
            continue;
        char *threadStatusStr;
        switch (tidTable[i]->getStatus())
        {
        case READY:
        case RUNNING:
            threadStatusStr = "R";
            break;
        case BLOCKED:
            threadStatusStr = "S";
            break;
        }
        if (tidTable[i] == threadToBeDestroyed)
            threadStatusStr = "X";
        printf("%3d %3d %-4s %s\n", tidTable[i]->getTid(),
               tidTable[i]->getUid(), threadStatusStr, tidTable[i]->getName());
    }
}

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void SimpleThread(int which)
{
    int num;

    for (num = 0; num < 5; num++)
    {
        printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// SimpleSleep
//  Sleep directly.
//
//  "which" is simply a number identifting the thread, for debugging
//  purposes.
//----------------------------------------------------------------------

void SimpleSleep(int which)
{
    interrupt->SetLevel(IntOff);
    printf("*** thread %d sleeped\n", which);
    currentThread->Sleep();
}
void DummyFunction(int which) {}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, (void *)1);
    SimpleThread(0);
}

//----------------------------------------------------------------------
// ThreadTest2
//  Try to fork 129 threads which surely exceed the limit of 128.
//  Should print 2 lines of error message.
//----------------------------------------------------------------------

void ThreadTest2()
{
    DEBUG('t', "Entering ThreadTest2");

    for (int i = 1; i <= MAX_THREADS + 1; ++i)
    {
        Thread *t = new Thread("forked thread");
        try
        {
            t->Fork(DummyFunction, (void *)i);
            printf("forked thread %d\n", i);
        }
        catch (thread_exception e)
        {
            delete t;
            puts(e.what());
        }
    }
}

//----------------------------------------------------------------------
// ThreadTest3
//  Try to fork 2 threads, making one yield 5 times while the other
//  sleeps. Print the ts result. When the first thread finishes,
//  fork another and print ts to check tid allocation works.
//----------------------------------------------------------------------

void ThreadTest3()
{
    DEBUG('t', "Entering ThreadTest3");

    Thread *t1 = new Thread("forked thread 1");
    Thread *t2 = new Thread("forked thread 2");
    Thread *t3 = new Thread("forked thread 3");

    t1->Fork(SimpleThread, (void *)1);
    t2->Fork(SimpleSleep, (void *)2);
    SimpleThread(0);
    ts();
    currentThread->Yield();
    ts();
    t3->Fork(DummyFunction, (void *)0);
    ts();
}

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void ThreadTest()
{
    switch (testnum)
    {
    case 1:
        ThreadTest1();
        break;
    case 2:
        ThreadTest2();
        break;
    case 3:
        ThreadTest3();
        break;
    default:
        printf("No test specified.\n");
        break;
    }
}

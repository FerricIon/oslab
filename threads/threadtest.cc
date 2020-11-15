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
#include "synch.h"
#include "sysdep.h"

// testnum is set in main.cc
int testnum = 1;

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
// ConditionBuffer
//  Shared data structure between producer and consumer threads - using
//  Condition Variables. Buffer size should be 4 in the following test.
//----------------------------------------------------------------------

struct ConditionBuffer
{
    Condition *empty;
    Condition *full;
    Lock *lock;

    unsigned int size;
    unsigned int unconsumed;
};

//----------------------------------------------------------------------
// ConditionConsumer
//  Consumer thread - using Condition Variables. Try to consume an
//  element if the buffer is not empty, for 10 times.
//----------------------------------------------------------------------

void ConditionConsumer(int p)
{
    ConditionBuffer *buffer = (ConditionBuffer *)p;
    for (int i = 0; i < 10; ++i)
    {
        buffer->lock->Acquire();
        while (buffer->unconsumed == 0)
        {
            printf("*** buffer is empty, %s waits...\n", currentThread->getName());
            buffer->empty->Wait(buffer->lock);
            printf("*** %s is waken up...\n", currentThread->getName());
        }

        --buffer->unconsumed;
        printf("### %s consumed an element - %d/%d\n", currentThread->getName(), buffer->unconsumed, buffer->size);

        buffer->full->Signal(buffer->lock);
        buffer->lock->Release();
    }
}

//----------------------------------------------------------------------
// ConditionProducer
//  Producer thread - using Condition Variables. Try to produce an
//  element if the buffer is not full, for 10 times.
//----------------------------------------------------------------------

void ConditionProducer(int p)
{
    ConditionBuffer *buffer = (ConditionBuffer *)p;
    for (int i = 0; i < 10; ++i)
    {
        buffer->lock->Acquire();
        while (buffer->unconsumed == buffer->size)
        {
            printf("*** buffer is full, %s waits...\n", currentThread->getName());
            buffer->full->Wait(buffer->lock);
            printf("*** %s is waken up...\n", currentThread->getName());
        }

        ++buffer->unconsumed;
        printf("### %s produced an element - %d/%d\n", currentThread->getName(), buffer->unconsumed, buffer->size);

        buffer->empty->Signal(buffer->lock);
        buffer->lock->Release();
    }
}

//----------------------------------------------------------------------
// ThreadTest2
//  Create a shared buffer, 2 consumer threads and 2 producer threads.
//  Should be tested with -rs option, yielding random interrupts.
//----------------------------------------------------------------------

void ThreadTest2()
{
    DEBUG('t', "Entering ThreadTest2");

    Thread *consumer_t_1 = new Thread("consumer1");
    Thread *producer_t_1 = new Thread("producer1");
    Thread *consumer_t_2 = new Thread("consumer2");
    Thread *producer_t_2 = new Thread("producer2");

    ConditionBuffer *buffer = new ConditionBuffer;
    buffer->empty = new Condition("empty condition");
    buffer->full = new Condition("full condition");
    buffer->lock = new Lock("lock");
    buffer->size = 4;
    buffer->unconsumed = 0;

    consumer_t_1->Fork(ConditionConsumer, buffer);
    producer_t_1->Fork(ConditionProducer, buffer);
    consumer_t_2->Fork(ConditionConsumer, buffer);
    producer_t_2->Fork(ConditionProducer, buffer);
}

//----------------------------------------------------------------------
// SemaphoreBuffer
//  Shared data structure between producer and consumer threads - using
//  Semaphore. Buffer size should be 4 in the following test.
//----------------------------------------------------------------------

struct SemaphoreBuffer
{
    Semaphore *empty;
    Semaphore *full;
    Lock *lock;

    unsigned int size;
    unsigned int unconsumed;
};

//----------------------------------------------------------------------
// SemaphoreConsumer
//  Consumer thread - using Semaphore. Try to consume an element if the
//  buffer is not empty, for 10 times.
//----------------------------------------------------------------------

void SemaphoreConsumer(int p)
{
    SemaphoreBuffer *buffer = (SemaphoreBuffer *)p;
    for (int i = 0; i < 10; ++i)
    {
        buffer->empty->P();
        buffer->lock->Acquire();
        --buffer->unconsumed;
        printf("### %s consumed an element - %d/%d\n", currentThread->getName(), buffer->unconsumed, buffer->size);
        buffer->lock->Release();
        buffer->full->V();
    }
}

//----------------------------------------------------------------------
// SemaphoreProducer
//  Producer thread - using Semaphore. Try to produce an element if the
//  buffer is not full, for 10 times.
//----------------------------------------------------------------------

void SemaphoreProducer(int p)
{
    SemaphoreBuffer *buffer = (SemaphoreBuffer *)p;
    for (int i = 0; i < 10; ++i)
    {
        buffer->full->P();
        buffer->lock->Acquire();
        ++buffer->unconsumed;
        printf("### %s produced an element - %d/%d\n", currentThread->getName(), buffer->unconsumed, buffer->size);
        buffer->lock->Release();
        buffer->empty->V();
    }
}

//----------------------------------------------------------------------
// ThreadTest3
//  Create a shared buffer, 2 consumer threads and 2 producer threads.
//  Should be tested with -rs option, yielding random interrupts.
//----------------------------------------------------------------------

void ThreadTest3()
{
    DEBUG('t', "Entering ThreadTest3");

    Thread *consumer_t_1 = new Thread("consumer1");
    Thread *producer_t_1 = new Thread("producer1");
    Thread *consumer_t_2 = new Thread("consumer2");
    Thread *producer_t_2 = new Thread("producer2");

    SemaphoreBuffer *buffer = new SemaphoreBuffer;
    buffer->empty = new Semaphore("empty semaphore", 0);
    buffer->full = new Semaphore("full semaphore", 4);
    buffer->lock = new Lock("buffer lock");
    buffer->size = 4;
    buffer->unconsumed = 0;

    consumer_t_1->Fork(SemaphoreConsumer, buffer);
    producer_t_1->Fork(SemaphoreProducer, buffer);
    consumer_t_2->Fork(SemaphoreConsumer, buffer);
    producer_t_2->Fork(SemaphoreProducer, buffer);
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

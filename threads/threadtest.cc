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
// ConditionBarrier
//  Shared data structure between threads - using Condition Variable.
//  Thread size should be 4 in the following test.
//----------------------------------------------------------------------

struct ConditionBarrier
{
    Condition *condition;
    Lock *lock;

    unsigned size;
    unsigned reached;
};

//----------------------------------------------------------------------
// SimpleThread2
//  Loop 5 times, while pusing forward the system clock by manually enabling
//  and disabling interrupts. A thread should wait at the barrier until
//  all threads have reached the barrier.
//----------------------------------------------------------------------

void SimpleThread2(int p)
{
    ConditionBarrier *barrier = (ConditionBarrier *)p;
    for (int i = 0; i < 5; ++i)
    {
        IntStatus oldLevel;
        oldLevel = interrupt->SetLevel(IntOff);
        interrupt->SetLevel(oldLevel);
        oldLevel = interrupt->SetLevel(IntOff);
        interrupt->SetLevel(oldLevel);
        oldLevel = interrupt->SetLevel(IntOff);
        printf("### %s loop %d\n", currentThread->getName(), i);
        interrupt->SetLevel(oldLevel);
        oldLevel = interrupt->SetLevel(IntOff);
        interrupt->SetLevel(oldLevel);
        oldLevel = interrupt->SetLevel(IntOff);
        interrupt->SetLevel(oldLevel);
    }
    barrier->lock->Acquire();
    ++barrier->reached;
    barrier->condition->Broadcast(barrier->lock);

    while (barrier->reached < barrier->size)
    {
        printf("  * %s is blocked by the barrier...\n", currentThread->getName());
        barrier->condition->Wait(barrier->lock);
    }
    barrier->lock->Release();
    printf("*** %s passed the barrier\n", currentThread->getName());
}

//----------------------------------------------------------------------
// ThreadTest4
//  Create a shared buffer and 4 threads running SimpleThread2.
//  Should be tested with -rs option, yielding random interrupts.
//----------------------------------------------------------------------

void ThreadTest4()
{
    DEBUG('t', "Entering ThreadTest4");

    Thread *thread_1 = new Thread("thread1");
    Thread *thread_2 = new Thread("thread2");
    Thread *thread_3 = new Thread("thread3");
    Thread *thread_4 = new Thread("thread4");

    ConditionBarrier *barrier = new ConditionBarrier;
    barrier->condition = new Condition("condition");
    barrier->lock = new Lock("lock");
    barrier->size = 4;
    barrier->reached = 0;

    thread_1->Fork(SimpleThread2, barrier);
    thread_2->Fork(SimpleThread2, barrier);
    thread_3->Fork(SimpleThread2, barrier);
    thread_4->Fork(SimpleThread2, barrier);
}

//----------------------------------------------------------------------
// ReadWriteLock
//  A lock based on Lock, providing interfaces for reading and writing
//  processes. There could only be one process writing or some
//  processes reading at a single time point.
//----------------------------------------------------------------------

struct ReadWriteLock
{
    unsigned reading;
    Semaphore *semaphore;
    Lock *mutex;

    ReadWriteLock()
    {
        reading = 0;
        semaphore = new Semaphore("read write lock semaphore", 1);
        mutex = new Lock("read write lock mutex");
    }
    ~ReadWriteLock()
    {
        delete semaphore;
        delete mutex;
    }
    void AcquireRead()
    {
        mutex->Acquire();
        if (reading == 0)
        {
            semaphore->P();
        }
        ++reading;
        printf("### %s starts reading... %d reading\n", currentThread->getName(), reading);
        mutex->Release();
    }
    void ReleaseRead()
    {
        mutex->Acquire();
        --reading;
        printf("### %s stops reading... %d reading\n", currentThread->getName(), reading);
        if (reading == 0)
        {
            semaphore->V();
        }
        mutex->Release();
    }
    void AcquireWrite()
    {
        semaphore->P();
        printf("*** %s starts writing... %d reading\n", currentThread->getName(), reading);
    }
    void ReleaseWrite()
    {
        printf("*** %s stops writing... %d reading\n", currentThread->getName(), reading);
        semaphore->V();
    }
};

//----------------------------------------------------------------------
// Reader
//  Reader processes for testing ReadWriteLock
//----------------------------------------------------------------------

void Reader(int p)
{
    ReadWriteLock *lock = (ReadWriteLock *)p;
    for (int i = 0; i < 5; ++i)
    {
        lock->AcquireRead();

        IntStatus oldLevel;
        oldLevel = interrupt->SetLevel(IntOff);
        interrupt->SetLevel(oldLevel);
        oldLevel = interrupt->SetLevel(IntOff);
        interrupt->SetLevel(oldLevel);
        oldLevel = interrupt->SetLevel(IntOff);
        interrupt->SetLevel(oldLevel);
        oldLevel = interrupt->SetLevel(IntOff);
        interrupt->SetLevel(oldLevel);

        lock->ReleaseRead();
    }
}

//----------------------------------------------------------------------
// Writer
//  Writer processes for testing ReadWriteLock
//----------------------------------------------------------------------

void Writer(int p)
{
    ReadWriteLock *lock = (ReadWriteLock *)p;
    for (int i = 0; i < 5; ++i)
    {
        lock->AcquireWrite();

        IntStatus oldLevel;
        oldLevel = interrupt->SetLevel(IntOff);
        interrupt->SetLevel(oldLevel);
        oldLevel = interrupt->SetLevel(IntOff);
        interrupt->SetLevel(oldLevel);
        oldLevel = interrupt->SetLevel(IntOff);
        interrupt->SetLevel(oldLevel);
        oldLevel = interrupt->SetLevel(IntOff);
        interrupt->SetLevel(oldLevel);

        lock->ReleaseWrite();
    }
}

//----------------------------------------------------------------------
// ThreadTest5
//  Create 4 reading processes and 2 reading ones which run
//  simultaneously. Reading processes could be able to run
//  concurrently, while a writing process should monopolize the CPU.
//----------------------------------------------------------------------

void ThreadTest5()
{
    DEBUG('t', "Entering ThreadTest4");

    Thread *reader_t_1 = new Thread("reader1");
    Thread *reader_t_2 = new Thread("reader2");
    Thread *reader_t_3 = new Thread("reader3");
    Thread *reader_t_4 = new Thread("reader4");
    Thread *writer_t_1 = new Thread("writer1");
    Thread *writer_t_2 = new Thread("writer2");

    ReadWriteLock *lock = new ReadWriteLock;

    reader_t_1->Fork(Reader, lock);
    reader_t_2->Fork(Reader, lock);
    reader_t_3->Fork(Reader, lock);
    reader_t_4->Fork(Reader, lock);
    writer_t_1->Fork(Writer, lock);
    writer_t_2->Fork(Writer, lock);
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
    case 4:
        ThreadTest4();
        break;
    case 5:
        ThreadTest5();
        break;
    default:
        printf("No test specified.\n");
        break;
    }
}

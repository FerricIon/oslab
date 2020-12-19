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

void ThreadTest2()
{
    DEBUG('t', "Entering ThreadTest2");

    printf("  List fileSystem: \n");
    fileSystem->List();
    printf("\n");
    printf("*** Creating Long_Filename_Test\n");
    fileSystem->Create("Long_Filename_Test", 20);
    printf("*** stat Long_Filename_Test\n");
    fileSystem->Stat("Long_Filename_Test");
    printf("  List fileSystem: \n");
    fileSystem->List();
    printf("\n");

    printf("*** Delay 2 seconds\n");
    Delay(2);
    printf("*** Opening Long_Filename_Test\n");
    OpenFile *openFile = fileSystem->Open("Long_Filename_Test");
    printf("*** Writing hello world! into Long_Filename_Test\n");
    openFile->WriteAt("hello world!", 13, 0);
    char buffer[20];
    printf("*** Reading from Long_Filename_Test at 6 for 6 chars\n");
    openFile->ReadAt(buffer, 6, 6);
    buffer[6] = '\0';
    printf("  content: %s\n", buffer);
    printf("*** Closing Long_Filename_Test\n");
    delete openFile;
    printf("*** stat Long_Filename_Test\n");
    fileSystem->Stat("Long_Filename_Test");

    printf("*** Removing Long_Filename_Test\n");
    fileSystem->Remove("Long_Filename_Test");
    printf("  List filesystem: \n");
    fileSystem->List();
    printf("\n");
}

void ThreadTest3()
{
    printf("*** Generating 5120 random integers\n");
    int dataArray[2 << 10];
    for (int i = 0; i < 2 << 10; ++i)
        dataArray[i] = Random();

    printf("*** Creating Big_File_Test\n");
    fileSystem->Create("Big_File_Test", (2 << 10) * sizeof(int));
    printf("*** Opening Big_File_Test\n");
    OpenFile *openFile = fileSystem->Open("Big_File_Test");
    printf("*** Writing integers into Big_File_Test\n");
    openFile->WriteAt((char *)dataArray, sizeof(dataArray), 0);

    printf("### Checking data integrity\n");
    bool flag = TRUE;
    for (int i = 0; i < (2 << 10); ++i)
    {
        int dataFromDisk;
        openFile->ReadAt((char *)&dataFromDisk, sizeof(int), i * sizeof(int));
        flag &= (dataFromDisk == dataArray[i]);
    }
    if (flag)
        printf("*** OK!\n");
    else
        printf("xxx Data corrupted\n");

    printf("*** Removing Big_File_Test\n");
    fileSystem->Remove("Big_File_Test");
}

void ThreadTest5()
{
    printf("*** Generating 2048 random integers\n");
    int dataArray[2 << 10];
    for (int i = 0; i < 2 << 10; ++i)
        dataArray[i] = Random();

    printf("*** Creating Dynamic_File_Test\n");
    fileSystem->Create("Dynamic_File_Test");
    printf("*** Opening Dynamic_File_Test\n");
    OpenFile *openFile = fileSystem->Open("Dynamic_File_Test");
    printf("### file size: %d\n", openFile->Length());
    printf("*** Writing 616 integers into Dynamic_File_Test\n");
    openFile->WriteAt((char *)dataArray, sizeof(int) * 616, 0);
    printf("### file size: %d\n", openFile->Length());
    printf("*** Writing 2048 integers into Dynamic_File_Test\n");
    openFile->WriteAt((char *)dataArray, sizeof(dataArray), openFile->Length());
    printf("### file size: %d\n", openFile->Length());

    printf("### Checking data integrity\n");
    bool flag = TRUE;
    for (int i = 0; i < 616; ++i)
    {
        int dataFromDisk;
        openFile->ReadAt((char *)&dataFromDisk, sizeof(int), i * sizeof(int));
        flag &= (dataFromDisk == dataArray[i]);
    }
    for (int i = 0; i < (2 << 10); ++i)
    {
        int dataFromDisk;
        openFile->ReadAt((char *)&dataFromDisk, sizeof(int),
                         (i + 616) * sizeof(int));
        flag &= (dataFromDisk == dataArray[i]);
    }
    if (flag)
        printf("*** OK!\n");
    else
        printf("xxx Data corrupted\n");

    printf("*** Removing Dynamic_File_Test\n");
    fileSystem->Remove("Dynamic_File_Test");
}

void SimpleThread2(int)
{
    OpenFile *openFile = fileSystem->Open("pipe");
    char s[100];
    openFile->Read(s, 12);
    printf("%s\n", s);
    currentThread->Yield();
    s[1] = '\0';
    for (int i = 0; i < 4; i++)
    {
        openFile->Read(s, 1);
        printf("%s\n", s);
    }
}
void ThreadTest7()
{
    fileSystem->Mkfifo("pipe");
    OpenFile *openFile = fileSystem->Open("pipe");
    openFile->Write("Hello World", 12);

    Thread *t = new Thread("forked thread");
    t->Fork(SimpleThread2, 0);

    currentThread->Yield();
    openFile->Write("Lab5", 4);
    currentThread->Yield();
}

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
    // case 4:
    //     ThreadTest4();
    //     break;
    case 5:
        ThreadTest5();
        break;
    case 7:
        ThreadTest7();
        break;
    default:
        printf("No test specified.\n");
        break;
    }
}

#include "copyright.h"

#ifndef SYNCHCONSOLE_H
#define SYNCHCONSOLE_H

#include "console.h"
#include "synch.h"

class SynchConsole
{
public:
    SynchConsole(char *readFile, char *writeFile);
    ~SynchConsole();

    void PutChar(char ch);
    char GetChar();

    void ReadAvail();
    void WriteDone();

private:
    Console *console;
    Semaphore *semaphoreRead, *semaphoreWrite;
    Lock *lockRead, *lockWrite;
};

#endif // SYNCHCONSOLE_H
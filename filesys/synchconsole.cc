#include "copyright.h"
#include "synchconsole.h"

static void ConsoleReadAvail(int arg)
{
    SynchConsole *console = (SynchConsole *)arg;

    console->ReadAvail();
}
static void ConsoleWriteDone(int arg)
{
    SynchConsole *console = (SynchConsole *)arg;

    console->WriteDone();
}

SynchConsole::SynchConsole(char *readFile, char *writeFile)
{
    semaphoreRead = new Semaphore("synch console read", 0);
    semaphoreWrite = new Semaphore("synch console write", 0);
    lockRead = new Lock("synch console read lock");
    lockWrite = new Lock("synch console write lock");
    console = new Console(readFile, writeFile, ConsoleReadAvail, ConsoleWriteDone, (int)this);
}

SynchConsole::~SynchConsole()
{
    delete console;
    delete lockWrite;
    delete lockRead;
    delete semaphoreWrite;
    delete semaphoreRead;
}

void SynchConsole::PutChar(char ch)
{
    lockWrite->Acquire();
    console->PutChar(ch);
    semaphoreWrite->P();
    lockWrite->Release();
}

char SynchConsole::GetChar()
{
    lockRead->Acquire();
    semaphoreRead->P();
    char ch = console->GetChar();
    lockRead->Release();

    return ch;
}

void SynchConsole::WriteDone()
{
    semaphoreWrite->V();
}

void SynchConsole::ReadAvail()
{
    semaphoreRead->V();
}
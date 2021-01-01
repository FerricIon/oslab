#include "syscall.h"

int main()
{
    Yield();
    Yield();

    Exit(1);
}
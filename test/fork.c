#include "syscall.h"

int main()
{
    int i;

    // Assume running from folder code
    Exec("test/exec");
    Yield();
    Yield();

    Exit(0);
}
#include "syscall.h"

#define PageSize 32
#define Pages 5
#define SeqLength 12

int mem[Pages * PageSize];
int seq[SeqLength] = {1, 2, 1, 0, 4, 1, 3, 4, 2, 1, 4, 1};

int main()
{
    int i, a = 0;

    for (i = 0; i < SeqLength; ++i)
    {
        a += mem[seq[i] * PageSize];
    }

    Exit(a);
}
// allocator.h

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "copyright.h"
#include "machine.h"

class PageAllocator
{
public:
    PageAllocator();
    ~PageAllocator();

    int AllocPage();                // Allocate an empty page
    void FreePage(int virtualPage); // Free a page
    int Available();                // How many empty pages

private:
    bool *pages;
};

#endif
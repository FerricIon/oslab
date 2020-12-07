// allocator.h

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "copyright.h"
#include "machine.h"
#include "addrspace.h"

struct PTEProps
{
    AddrSpace *space;
    TranslationEntry *entry;
    char count;
};

class PageAllocator
{
public:
    PageAllocator();
    ~PageAllocator();

    void AllocPage(TranslationEntry *entry); // Allocate an empty page
    void FreePage(TranslationEntry *entry);  // Free a page
    void UpdateCount();                      // Update count

private:
    PTEProps *pages;
    int available;
};

#endif
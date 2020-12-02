// allocator.cc
//  Allocate or recycle one page at a time.

#include "allocator.h"

//----------------------------------------------------------------------
// PageAllocator::PageAllocator
//  Create an allocator. Maintaining all physical page state in pages.
//  All pages should be free at first.
//----------------------------------------------------------------------

PageAllocator::PageAllocator()
{
    pages = new bool[NumPhysPages];
    memset(pages, 0, sizeof(bool) * NumPhysPages);
}

//----------------------------------------------------------------------
// PageAllocator::~PageAllocator
//  Free the array.
//----------------------------------------------------------------------

PageAllocator::~PageAllocator()
{
    delete[] pages;
}

//----------------------------------------------------------------------
// PageAllocator::AllocPage
//  Find the first free page and allocate it. If none, return -1.
//----------------------------------------------------------------------

int PageAllocator::AllocPage()
{
    for (int i = 0; i < NumPhysPages; ++i)
        if (!pages[i])
        {
            pages[i] = 1;
            return i;
        }
    return -1;
}

//----------------------------------------------------------------------
// PageAllocator::FreePage
//  Free an allocated page.

void PageAllocator::FreePage(int page)
{
    ASSERT(pages[page]);

    pages[page] = 0;
}

//----------------------------------------------------------------------
// PageAllocator::Available
//  Count how many free pages are there.
//----------------------------------------------------------------------

int PageAllocator::Available()
{
    int count = 0;
    for (int i = 0; i < NumPhysPages; ++i)
        count += (pages[i] == 0);
    return count;
}

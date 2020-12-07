// allocator.cc
//  Allocate or recycle one page at a time.

#include "allocator.h"
#include "system.h"

//----------------------------------------------------------------------
// PageAllocator::PageAllocator
//  Create an allocator. Maintaining all physical page state in pages.
//  All pages should be free at first.
//----------------------------------------------------------------------

PageAllocator::PageAllocator()
{
    pages = new PTEProps[NumPhysPages];
    available = NumPhysPages;
    for (int i = 0; i < NumPhysPages; ++i)
    {
        pages[i].entry = NULL;
    }
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

void PageAllocator::AllocPage(TranslationEntry *entry)
{
    if (available)
    {
        for (int i = 0; i < NumPhysPages; ++i)
            if (pages[i].entry == NULL)
            {
                pages[i].space = currentThread->space;
                pages[i].entry = entry;
                pages[i].count = 0xFF;

                entry->physicalPage = i;
                --available;
                break;
            }
    }
    else
    {
        int index = 0;
        for (int i = 0; i < NumPhysPages; ++i)
            if (pages[i].count < pages[index].count)
                index = i;
        int count = 0, *indexes = new int[NumPhysPages];
        for (int i = 0; i < NumPhysPages; ++i)
        {
            indexes[count++] = i;
        }
        index = indexes[Random() % count];
        printf("SWAP OUT %d LOAD %d\n", pages[index].entry->virtualPage, entry->virtualPage);
        pages[index].space->SwapOut(pages[index].entry->virtualPage);
        pages[index].space = currentThread->space;
        pages[index].entry = entry;
        pages[index].count = 0xFF;

        entry->physicalPage = index;
    }
}

//----------------------------------------------------------------------
// PageAllocator::FreePage
//  Free an allocated page.
//----------------------------------------------------------------------

void PageAllocator::FreePage(TranslationEntry *entry)
{
    int page = entry->physicalPage;
    ASSERT(pages[page].entry == entry);

    pages[page].entry = NULL;
    ++available;
}

//----------------------------------------------------------------------
// PageAllocator::UpdateCount
//  Update count on timer interrupt with Aging policy.
//----------------------------------------------------------------------

void PageAllocator::UpdateCount()
{
    for (int i = 0; i < NumPhysPages; ++i)
        if (pages[i].entry != NULL)
        {
            pages[i].count =
                (pages[i].entry->use << 7) | pages[i].count >> 1;
            pages[i].entry->use = FALSE;
        }
}

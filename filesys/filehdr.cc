// filehdr.cc
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector,
//
//      Unlike in a real system, we do not keep track of file permissions,
//	ownership, last modification date, etc., in the file header.
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"

#include <ctime>

#define min(a, b) (((a) < (b)) ? (a) : (b))

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool FileHeader::Allocate(BitMap *freeMap, int fileSize)
{
    numBytes = fileSize;
    numSectors = divRoundUp(fileSize, SectorSize);
    numSecondaryIndexes = divRoundUp(numSectors, SectorsPerSecondaryIndex);
    if (freeMap->NumClear() < numSectors + numSecondaryIndexes)
        return FALSE; // not enough space

    int dataSectorsToAllocate = numSectors;
    for (int i = 0; i < numSecondaryIndexes; i++)
    {
        secondaryIndexSectors[i] = freeMap->Find();
        int dataSectors[SectorsPerSecondaryIndex];
        int sectors = min(dataSectorsToAllocate, SectorsPerSecondaryIndex);
        for (int j = 0; j < sectors; j++)
            dataSectors[j] = freeMap->Find();

        synchDisk->WriteSector(secondaryIndexSectors[i], (char *)dataSectors);
        dataSectorsToAllocate -= sectors;
    }
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void FileHeader::Deallocate(BitMap *freeMap)
{
    int dataSectorsToDeallocate = numSectors;
    for (int i = 0; i < numSecondaryIndexes; i++)
    {
        // ought to be marked!
        ASSERT(freeMap->Test((int)secondaryIndexSectors[i]));

        int dataSectors[SectorsPerSecondaryIndex];
        int sectors = min(dataSectorsToDeallocate, SectorsPerSecondaryIndex);
        synchDisk->ReadSector(secondaryIndexSectors[i], (char *)dataSectors);

        for (int j = 0; j < sectors; j++)
        {
            // ought to be marked!
            ASSERT(freeMap->Test((int)dataSectors[j]));
            freeMap->Clear(dataSectors[j]);
        }
        freeMap->Clear(secondaryIndexSectors[i]);
        dataSectorsToDeallocate -= sectors;
    }
}

//----------------------------------------------------------------------
// FileHeader::Append
//  Allocate new blocks for appended content
//----------------------------------------------------------------------

bool FileHeader::Append(BitMap *freeMap, int size)
{
    int newBytes = numBytes + size;
    int newSectors = divRoundUp(newBytes, SectorSize);
    int newSecondaryIndexes = divRoundUp(newSectors, SectorsPerSecondaryIndex);

    if (freeMap->NumClear() < (newSectors + newSecondaryIndexes) -
                                  (numSectors + numSecondaryIndexes))
        return FALSE;

    for (int i = 0; i < newSecondaryIndexes - numSecondaryIndexes; ++i)
        secondaryIndexSectors[i + numSecondaryIndexes] = freeMap->Find();
    int dataSectorsToAllocate = newSectors - numSectors;
    int start = numSectors % SectorsPerSecondaryIndex;
    for (int i = numSecondaryIndexes - !!start; i < newSecondaryIndexes; ++i)
    {
        int dataSectors[SectorsPerSecondaryIndex];
        synchDisk->ReadSector(secondaryIndexSectors[i], (char *)dataSectors);
        int sectors = min(dataSectorsToAllocate, SectorsPerSecondaryIndex);
        for (int j = start; j < sectors; j++)
            dataSectors[j] = freeMap->Find();

        synchDisk->WriteSector(secondaryIndexSectors[i], (char *)dataSectors);
        dataSectorsToAllocate -= (sectors - start);
        start = 0;
    }

    numBytes = newBytes;
    numSectors = newSectors;
    numSecondaryIndexes = newSecondaryIndexes;

    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int FileHeader::ByteToSector(int offset)
{
    int toSecondaryIndex = offset / (SectorsPerSecondaryIndex * SectorSize);
    int dataSectors[SectorsPerSecondaryIndex];
    synchDisk->ReadSector(secondaryIndexSectors[toSecondaryIndex],
                          (char *)dataSectors);

    offset %= (SectorsPerSecondaryIndex * SectorSize);
    return (dataSectors[offset / SectorSize]);
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void FileHeader::Print(bool showContent)
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d. File type: %s.\n",
           numBytes, type ? "directory" : "common");
    printf("Created at: %d. Last Visited at: %d.\n", created, lastVisited);
    printf("Last Modified at: %d. File blocks:\n", lastModified);

    if (showContent)
    {
        int _numSectors = numSectors;
        for (i = 0; i < numSecondaryIndexes; i++)
        {
            int dataSectors[SectorsPerSecondaryIndex];
            synchDisk->ReadSector(secondaryIndexSectors[i],
                                  (char *)dataSectors);
            int sectors = min(_numSectors, SectorsPerSecondaryIndex);
            for (j = 0; j < sectors; j++)
                printf("%d ", dataSectors[j]);
            _numSectors -= sectors;
        }
        printf("\nFile contents:\n");
        _numSectors = numSectors;
        for (i = 0; i < numSecondaryIndexes; i++)
        {
            int dataSectors[SectorsPerSecondaryIndex];
            synchDisk->ReadSector(secondaryIndexSectors[i],
                                  (char *)dataSectors);
            int sectors = min(_numSectors, SectorsPerSecondaryIndex);
            for (i = k = 0; i < sectors; i++)
            {
                synchDisk->ReadSector(dataSectors[i], data);
                for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
                {
                    // isprint(data[j])
                    if ('\040' <= data[j] && data[j] <= '\176')
                        printf("%c", data[j]);
                    else
                        printf("\\%x", (unsigned char)data[j]);
                }
                printf("\n");
            }
            _numSectors -= numSectors;
        }
    }
    delete[] data;
}

//----------------------------------------------------------------------
// FileHeader::SetType
//  Set file type.
//----------------------------------------------------------------------

void FileHeader::SetType(int _type)
{
    type = _type;
}

//----------------------------------------------------------------------
// FileHeader::UpdateCreated
//  Update created timestamp.
//----------------------------------------------------------------------

void FileHeader::UpdateCreated()
{
    created = time(NULL);
}

//----------------------------------------------------------------------
// FileHeader::UpdateLastVisited
//  Update lastVisited timestamp.
//----------------------------------------------------------------------

void FileHeader::UpdateLastVisited()
{
    lastVisited = time(NULL);
}

//----------------------------------------------------------------------
// FileHeader::UpdateLastModified
//  Update lastModified timestamp.
//----------------------------------------------------------------------

void FileHeader::UpdateLastModified()
{
    lastModified = time(NULL);
}

//----------------------------------------------------------------------
// FileHeader::GetType
//  Get type of file.
//----------------------------------------------------------------------

int FileHeader::GetType()
{
    return type;
}
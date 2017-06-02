#pragma once

#include <stdint.h>
#include <stddef.h>
#include "messaging.h"

#define FREELIST_DEBUG (debuglevel >= 3)

typedef struct FreeList_Header_ {
    struct FreeList_Header_*nextPtr;
    uint32_t                numBlocks;
} FreeList_Header;

typedef void*(*SbrkFunc)(intptr_t increment);

typedef struct {
    SbrkFunc        sbrkFunc;       // sbrk() type function to increments the program's data space
    uint32_t        heapSize;       // total number of bytes retrieved from sbrk() (i.e. sum of allocated and free bytes)
    FreeList_Header root;
    FreeList_Header*freeList;       // circular linked list of free memory blocks
#if FREELIST_DEBUG
    FreeList_Header*allocList;      // circular linked list of allocated memory blocks
#endif
} FreeList_Context;

void* FreeList_Alloc(FreeList_Context* context, size_t size);
void  FreeList_Free(FreeList_Context* context, void* ptr);

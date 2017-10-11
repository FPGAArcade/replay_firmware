#include "freelist.h"
#include "messaging.h"
#include <string.h>

void* FreeList_Alloc(FreeList_Context* context, size_t size, uint32_t tag)
{
    size_t numBlocks = (size + sizeof(FreeList_Header) - 1) / sizeof(FreeList_Header) + 1;
    FreeList_Header* prevPtr = context->freeList;

    if (prevPtr == NULL) {
        context->root.numBlocks = 0;
        context->root.nextPtr = &context->root;
        context->freeList = context->root.nextPtr;
        prevPtr = context->freeList;
#if FREELIST_DEBUG
        context->allocList = NULL;
#endif
    }

    for (FreeList_Header* p = prevPtr->nextPtr; ; prevPtr = p, p = p->nextPtr) {

        if (p->numBlocks >= numBlocks) {
            if (p->numBlocks == numBlocks) {
                prevPtr->nextPtr = p->nextPtr;

            } else {
                p->numBlocks -= numBlocks;
                p += p->numBlocks;
                p->numBlocks = numBlocks;
            }

            context->freeList = prevPtr;
#if FREELIST_DEBUG
            p->tag = tag;
            p->nextPtr = context->allocList;
            context->allocList = p;
            memset(p + 1, 0xcd, (numBlocks - 1) * sizeof(FreeList_Header));
#endif
            return (void*) (p + 1);
        }

        if (p == context->freeList) {
            size_t addBlocks = numBlocks;

            if (addBlocks < 1024) {
                addBlocks = 1024;
            }

            size_t addBytes = 0;

            if (context->sbrkFunc == NULL) {
                ERROR("FreeList_Alloc ERROR : no sbrk() function!");
                return 0;
            }

            size_t divider = 1;

            do {
                addBytes = (addBlocks / divider) * sizeof(FreeList_Header);
                p = (FreeList_Header*) context->sbrkFunc(addBytes);
                divider++;
            } while (p == (FreeList_Header*) - 1 && addBytes >= numBlocks * sizeof(FreeList_Header));

            if (p == (FreeList_Header*) - 1) {
                ERROR("FreeList_Alloc ERROR : Out-Of-Memory!");
                return 0;
            }

            p->nextPtr = NULL;
            p->numBlocks = addBytes / sizeof(FreeList_Header);
            FreeList_Free(context, p + 1);
            context->heapSize += addBytes;
            p = context->freeList;
        }
    }
}


void FreeList_Free(FreeList_Context* context, void* ptr)
{
    FreeList_Header* blockPtr = (FreeList_Header*) ptr - 1;

#if FREELIST_DEBUG

    for (FreeList_Header** p = &context->allocList; *p != NULL; p = &((*p)->nextPtr)) {
        if (*p == blockPtr) {
            *p = (*p)->nextPtr;
            break;
        }
    }

#endif

    FreeList_Header* p = context->freeList;

    for (; !(p < blockPtr && blockPtr < p->nextPtr); p = p->nextPtr)
        if (p >= p->nextPtr && (p < blockPtr || blockPtr < p->nextPtr)) {
            break;
        }

    if (blockPtr + blockPtr->numBlocks == p->nextPtr) {
        blockPtr->numBlocks += p->nextPtr->numBlocks;
        blockPtr->nextPtr = p->nextPtr->nextPtr;

    } else {
        blockPtr->nextPtr = p->nextPtr;
    }

    if ( p + p->numBlocks == blockPtr ) {
        p->numBlocks += blockPtr->numBlocks;
        p->nextPtr = blockPtr->nextPtr;

    } else {
        p->nextPtr = blockPtr;
    }

    context->freeList = p;
}

#include <stdlib.h>
#include <unistd.h>
#include "config.h"
#include "messaging.h"
#include "freelist.h"

#define FF_HEAP_SIZE 1024*2/4
static uint32_t ff_heap[FF_HEAP_SIZE];

static void* ff_sbrk(ptrdiff_t increment)
{
    static uint8_t init = 0;

    if (init) {
        return (void*) - 1;
    }

    DEBUG(3, ">> ff_sbrk(%d)", increment);

    if (increment > sizeof(ff_heap)) {
        return (void*) - 1;
    }

    init = 1;
    return &ff_heap[0];
}

static FreeList_Context s_MallocContext = {
    .sbrkFunc	= ff_sbrk,
    .heapSize	= 0,
    .root		= { .nextPtr = NULL, .numBlocks = 0 },
    .freeList	= NULL
};

void* ff_malloc(size_t size)
{
    void* p = FreeList_Alloc(&s_MallocContext, size, 0x0ff4110c);
    DEBUG(3, ">> ff_malloc(%d) => %08x", size, p);
    return p;
}

void ff_free(void* ptr)
{
    FreeList_Free(&s_MallocContext, ptr);
}

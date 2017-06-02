#include <malloc.h>
#include <unistd.h>
#include "config.h"
#include "messaging.h"
#include "freelist.h"

void* _sbrk_r(void *_s_r, ptrdiff_t nbytes);
void* sbrk(ptrdiff_t increment)
{
	mallinfo();
	DEBUG(2, ">> sbrk(%d)", increment);
	return _sbrk_r(NULL, increment);
}

static FreeList_Context s_MallocContext = {
	.sbrkFunc	= &sbrk,
	.heapSize	= 0,
	.root		= { .nextPtr = NULL, .numBlocks = 0 }, 
	.freeList	= NULL
};

void* malloc(size_t size)
{
	return FreeList_Alloc(&s_MallocContext, size);
}

void* calloc(size_t num, size_t size)
{
	size_t len = num*size;
	void* p = malloc(len);
	if (p != NULL)
		memset(p, 0x00, len);
	return p;
}

void free(void *ptr)
{
	FreeList_Free(&s_MallocContext, ptr);
}

static struct mallinfo s_mallinfo = { 0 };
struct mallinfo mallinfo(void)
{
	DEBUG(2, "== mallinfo ==");
	DEBUG(2, "heapSize = %d", s_MallocContext.heapSize);
	DEBUG(2, "&root    = %08x", &s_MallocContext.root);
	DEBUG(2, "freeList = %08x", s_MallocContext.freeList);
	s_mallinfo.arena = s_MallocContext.heapSize;

	s_mallinfo.fordblks = 0;
	for (FreeList_Header* p = s_MallocContext.root.nextPtr; p != &s_MallocContext.root; p = p->nextPtr) {
		DEBUG(3, "p = %08x ; numBlocks = %d", p, p->numBlocks);
		s_mallinfo.fordblks += p->numBlocks * sizeof(FreeList_Header);
	}
	DEBUG(2, "freeSize = %d (list)", s_mallinfo.fordblks);

#if FREELIST_DEBUG
	s_mallinfo.uordblks = 0;
	for (FreeList_Header* p = s_MallocContext.allocList; p != NULL; p = p->nextPtr) {
		DEBUG(3, "p = %08x ; numBlocks = %d", p, p->numBlocks);
		s_mallinfo.uordblks += p->numBlocks * sizeof(FreeList_Header);
//		DumpBuffer((uint8_t*)p, p->numBlocks * sizeof(FreeList_Header));
	}
	DEBUG(2, "allocate = %d (list)", s_mallinfo.uordblks);
#endif

	s_mallinfo.uordblks = s_mallinfo.arena - s_mallinfo.fordblks;
	DEBUG(2, "freeSize = %d", s_mallinfo.fordblks);
	DEBUG(2, "allocate = %d", s_mallinfo.uordblks);
	DEBUG(2, "^^ mallinfo ^^");
	return s_mallinfo;
}

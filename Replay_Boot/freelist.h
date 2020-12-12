/*--------------------------------------------------------------------
 *                       Replay Firmware
 *                      www.fpgaarcade.com
 *                     All rights reserved.
 *
 *                     admin@fpgaarcade.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *--------------------------------------------------------------------
 *
 * Copyright (c) 2020, The FPGAArcade community (see AUTHORS.txt)
 *
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "messaging.h"

#define FREELIST_DEBUG (debuglevel >= 3)

typedef struct FreeList_Header_ {
    struct FreeList_Header_* nextPtr;
    uint32_t                numBlocks;
#if FREELIST_DEBUG
    uint32_t                tag;
#endif
} FreeList_Header;

typedef void* (*SbrkFunc)(intptr_t increment);

typedef struct {
    SbrkFunc        sbrkFunc;       // sbrk() type function to increments the program's data space
    uint32_t        heapSize;       // total number of bytes retrieved from sbrk() (i.e. sum of allocated and free bytes)
    FreeList_Header root;
    FreeList_Header* freeList;      // circular linked list of free memory blocks
#if FREELIST_DEBUG
    FreeList_Header* allocList;     // circular linked list of allocated memory blocks
#endif
} FreeList_Context;

void* FreeList_Alloc(FreeList_Context* context, size_t size, uint32_t tag);
void  FreeList_Free(FreeList_Context* context, void* ptr);

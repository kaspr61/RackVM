/*
BSD 2-Clause License

Copyright (c) 2022, Kasper Skott

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vm_memory.h"

/* Uncomment to enable trace printing for VM heap memory.*/
/* #define TRACE_MEMORY */

typedef struct Alloc {
    uint32_t safebytes;
    uint8_t occupied;
    struct Alloc *next;
    struct Alloc *prev;
} Alloc_t;

uint8_t *heap;
uint64_t heapSize;

int i = sizeof(Alloc_t);

/* Gets the number of bytes between alloc and its 'next' pointer. */
#define GetAllocSize(alloc) ((size_t)((uint8_t*)(alloc->next) - (uint8_t*)alloc))

#define ALLOC_SAFE_BYTES 0xDEADC0DE

bool AllocateHeap(uint64_t size, uint64_t maxSize)
{
    if (size > maxSize || size > 0x100000000 /* 4 GiB */ )
        return false;

    heap = calloc(size, 1);
    if (!heap)
        return false;

    heapSize = size;
    Alloc_t *head = (Alloc_t*)heap;
    head->next = (Alloc_t*)(heap + heapSize);
    head->prev = NULL;
    head->safebytes = ALLOC_SAFE_BYTES;

    return true;
}

void DeallocateHeap()
{
    free(heap);
}

Addr_t VMHeapAlloc(uint32_t size)
{
    if (size == 0)
        return 0;

    /* Must align up the size to a multiple of 24 (i.e. size of Alloc_t). */
    size = ((size-1) / sizeof(Alloc_t) + 1) * sizeof(Alloc_t);

    Alloc_t *curr = (Alloc_t*)heap;
    
    /* Search for a free block that is big enough. */
    while ((uint8_t*)curr < heap + heapSize && 
           (curr->occupied || GetAllocSize(curr) < size + sizeof(Alloc_t)))
    {
        curr = curr->next;
        if (curr == NULL)
            break;
    }

    if (curr == NULL)
    {
        puts("[RackVM] Heap memory corruption.");
        return 0;
    }

#if !defined(NDEBUG) && defined(TRACE_MEMORY)
    printf("Found free block of size %llu, starting at %lu.\n", 
        GetAllocSize(curr), (Addr_t)((uint8_t *)curr - heap));
#endif

    Alloc_t *next = (Alloc_t *)((uint8_t*)curr + sizeof(Alloc_t) + size);

    if ((uint8_t*)curr >= heap + heapSize || (uint8_t*)next >= heap + heapSize)
    {
        /* TODO: Handle heap resize. */
    }

    next->prev = curr;
    curr->occupied = true;

    Alloc_t *tmp = curr->next;
    curr->safebytes = ALLOC_SAFE_BYTES;
    curr->next = next;
    curr->next->next = tmp;
    curr->next->prev = curr;

    return (Addr_t)((uint8_t *)curr - heap) + sizeof(Alloc_t);
}

Addr_t VMHeapRealloc(Addr_t address, uint32_t size)
{
    if (size == 0)
        return 0;

    address -= sizeof(Alloc_t);

    /* Must align up the size to a multiple of 24 (i.e. size of Alloc_t). */
    size = ((size-1) / sizeof(Alloc_t) + 1) * sizeof(Alloc_t);
    uint32_t reqSize = size + sizeof(Alloc_t);

    Alloc_t *curr = (Alloc_t*)(heap + address);
    uint32_t currSize = GetAllocSize(curr);
    if (reqSize == currSize)
        return address + sizeof(Alloc_t);

    if (reqSize < currSize)
    {
        Alloc_t *next = (Alloc_t *)((uint8_t*)curr + reqSize);
        next->prev = curr;
        next->safebytes = ALLOC_SAFE_BYTES;
        next->next = curr->next->next;

        curr->next = next;

        return address + sizeof(Alloc_t);
    }

    Addr_t newAddress = address;

    /* Check if next is suitable for expansion. Best case scenario! */
    if ((uint8_t*)(curr->next) < heap + heapSize && 
        !curr->next->occupied && 
        GetAllocSize(curr->next) >= reqSize)
    {
        Alloc_t *next = (Alloc_t *)((uint8_t*)curr + reqSize);
        next->prev = curr;
        next->safebytes = ALLOC_SAFE_BYTES;
        next->next = curr->next->next;

        curr->next = next;
    }
    else /* Otherwise, find a new block of memory to inhabit. */
    {
        Alloc_t *old = (Alloc_t*)(heap + address);
        uint32_t oldDataSize = ((uint8_t*)(old->next) - (uint8_t*)old) - sizeof(Alloc_t);
        address += sizeof(Alloc_t);

        newAddress = VMHeapAlloc(size);

        /* Copy the old data to the new data. No Alloc_t header, only data.*/
        memcpy(heap + newAddress, heap + address,  oldDataSize);

        VMHeapFree(address);
    }

    return newAddress;
}

void VMHeapFree(Addr_t address)
{
    address -= sizeof(Alloc_t);

    /* Check for corruption by checking the safebytes.*/
    Alloc_t *curr = (Alloc_t*)(heap + address);
    if (curr->safebytes != ALLOC_SAFE_BYTES)
    {
        printf("[RackVM] Warning: heap corrupted near 0x%0lX.\n", address);
    }

#if !defined(NDEBUG) && defined(TRACE_MEMORY)
    printf("Deallocating %llu bytes at %lu.\n", 
        GetAllocSize(curr), (Addr_t)((uint8_t *)curr - heap));
#endif

    
    /* If the prev is free, join them. */
    if (curr->prev != NULL && !curr->prev->occupied)
    {
        curr->prev->next = curr->next;
    }

    /* If the next is free, join them. */
    if ((uint8_t*)(curr->next) < heap + heapSize && !curr->next->occupied)
        curr->next = curr->next->next;

    /* The previous statements will mitigate memory fragmentation. */

    curr->occupied = false;
}

Addr_t VMHeapAllocString(const char *content)
{
    size_t size = strlen(content)+1;
    Addr_t str = VMHeapAlloc(size);

    strcpy(heap + str, content);

#if !defined(NDEBUG) && defined(TRACE_MEMORY)
    printf("Created new string \"%s\" at %lu.\n", 
        (const char *)(heap+str), str);
#endif

    return str;
}

Addr_t VMHeapAllocCombinedString(const char *content1, const char *content2)
{
    size_t content1Size = strlen(content1);
    size_t content2Size = strlen(content2);
    size_t size = content1Size + content2Size + 1;
    Addr_t str = VMHeapAlloc(size);

    memcpy(heap + str, content1, content1Size);
    memcpy(heap + str + content1Size, content2, content2Size);
    *(char*)(heap + str + size) = '\0';

#if !defined(NDEBUG) && defined(TRACE_MEMORY)
    printf("Created new string \"%s\" at %lu.\n", 
        (const char *)(heap+str), str);
#endif

    return str;
}

Addr_t VMHeapAllocSubStr(const char *content, uint32_t size)
{
    size_t realSize = strlen(content);
    if (size > realSize)
        size = realSize;

    Addr_t str = VMHeapAlloc(size);

    memcpy(heap + str, content, size);
    *(char*)(heap+str+size) = '\0';

#if !defined(NDEBUG) && defined(TRACE_MEMORY)
    printf("Created new string \"%s\" at %lu.\n", 
        (const char *)(heap+str), str);
#endif

    return str;
}

uint32_t VMGetHeapAllocSize(Addr_t address)
{
    Alloc_t *curr = (Alloc_t*)(heap + address - sizeof(Alloc_t));
    return (uint32_t)GetAllocSize(curr);
}

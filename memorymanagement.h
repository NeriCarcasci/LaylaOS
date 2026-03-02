#ifndef MEMORYMANAGEMENT_H
#define MEMORYMANAGEMENT_H

#include "types.h"

struct MemoryChunk
{
    MemoryChunk* prev;
    MemoryChunk* next;
    bool         allocated;
    uint32_t     size;
} __attribute__((packed));

class MemoryManager
{
public:
    MemoryChunk* first;

    MemoryManager(uint32_t start, uint32_t size);
    ~MemoryManager();

    void* malloc(uint32_t size);
    void  free(void* ptr);
};

#endif

#include "memorymanagement.h"

MemoryManager::MemoryManager(uint32_t start, uint32_t size)
{
    first            = (MemoryChunk*)start;
    first->prev      = 0;
    first->next      = 0;
    first->allocated = false;
    first->size      = size - sizeof(MemoryChunk);
}

MemoryManager::~MemoryManager()
{
}

void* MemoryManager::malloc(uint32_t size)
{
    for (MemoryChunk* chunk = first; chunk != 0; chunk = chunk->next)
    {
        if (chunk->allocated || chunk->size < size)
            continue;

        if (chunk->size > size + sizeof(MemoryChunk))
        {
            MemoryChunk* new_chunk = (MemoryChunk*)((uint8_t*)chunk + sizeof(MemoryChunk) + size);
            new_chunk->size        = chunk->size - size - sizeof(MemoryChunk);
            new_chunk->prev        = chunk;
            new_chunk->next        = chunk->next;
            new_chunk->allocated   = false;
            if (chunk->next)
                chunk->next->prev = new_chunk;
            chunk->next = new_chunk;
            chunk->size = size;
        }

        chunk->allocated = true;
        return (void*)((uint8_t*)chunk + sizeof(MemoryChunk));
    }
    return 0;
}

void MemoryManager::free(void* ptr)
{
    MemoryChunk* chunk = (MemoryChunk*)((uint8_t*)ptr - sizeof(MemoryChunk));
    chunk->allocated = false;

    if (chunk->next && !chunk->next->allocated)
    {
        chunk->size += sizeof(MemoryChunk) + chunk->next->size;
        chunk->next  = chunk->next->next;
        if (chunk->next)
            chunk->next->prev = chunk;
    }

    if (chunk->prev && !chunk->prev->allocated)
    {
        chunk->prev->size += sizeof(MemoryChunk) + chunk->size;
        chunk->prev->next  = chunk->next;
        if (chunk->next)
            chunk->next->prev = chunk->prev;
    }
}

#ifndef __ALLOCATOR_H
#define __ALLOCATOR_H

#include "types.h"
#include "memorymanagement.h"

extern MemoryManager* active_memory_manager;

void* operator new(uint32_t size);
void* operator new[](uint32_t size);
void  operator delete(void* ptr);
void  operator delete(void* ptr, uint32_t size);
void  operator delete[](void* ptr);
void  operator delete[](void* ptr, uint32_t size);

#endif

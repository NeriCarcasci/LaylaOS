#include "allocator.h"

MemoryManager* active_memory_manager = nullptr;

void* operator new(uint32_t size)   { return active_memory_manager->malloc(size); }
void* operator new[](uint32_t size) { return active_memory_manager->malloc(size); }
void  operator delete(void* ptr)                 { active_memory_manager->free(ptr); }
void  operator delete(void* ptr, uint32_t size)  { active_memory_manager->free(ptr); }
void  operator delete[](void* ptr)               { active_memory_manager->free(ptr); }
void  operator delete[](void* ptr, uint32_t size){ active_memory_manager->free(ptr); }

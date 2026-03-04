#include "pmm.h"

uint32_t* PhysicalMemoryManager::bitmap      = nullptr;
uint32_t  PhysicalMemoryManager::total_frames = 0;
uint32_t  PhysicalMemoryManager::free_frames  = 0;

void PhysicalMemoryManager::Init(uint32_t bitmap_base, uint32_t mem_size) {
    bitmap       = (uint32_t*)bitmap_base;
    total_frames = mem_size / 4096;
    free_frames  = 0;
    uint32_t dwords = (total_frames + 31) / 32;
    for (uint32_t i = 0; i < dwords; i++)
        bitmap[i] = 0xFFFFFFFF;
}

uint32_t PhysicalMemoryManager::AllocFrame() {
    int frame = FirstFree();
    if (frame < 0) return 0;
    SetBit((uint32_t)frame);
    if (free_frames > 0) free_frames--;
    return (uint32_t)frame * 4096;
}

void PhysicalMemoryManager::FreeFrame(uint32_t addr) {
    ClearBit(addr / 4096);
    free_frames++;
}

void PhysicalMemoryManager::MarkUsed(uint32_t addr, uint32_t length) {
    uint32_t start = addr / 4096;
    uint32_t end   = (addr + length + 4095) / 4096;
    for (uint32_t i = start; i < end && i < total_frames; i++) {
        if (!TestBit(i) && free_frames > 0)
            free_frames--;
        SetBit(i);
    }
}

void PhysicalMemoryManager::MarkFree(uint32_t addr, uint32_t length) {
    uint32_t start = addr / 4096;
    uint32_t end   = (addr + length) / 4096;
    for (uint32_t i = start; i < end && i < total_frames; i++) {
        if (TestBit(i))
            free_frames++;
        ClearBit(i);
    }
}

uint32_t PhysicalMemoryManager::FreeFrameCount()  { return free_frames; }
uint32_t PhysicalMemoryManager::TotalFrameCount() { return total_frames; }

void PhysicalMemoryManager::SetBit(uint32_t frame) {
    bitmap[frame / 32] |= (1u << (frame % 32));
}

void PhysicalMemoryManager::ClearBit(uint32_t frame) {
    bitmap[frame / 32] &= ~(1u << (frame % 32));
}

bool PhysicalMemoryManager::TestBit(uint32_t frame) {
    return (bitmap[frame / 32] & (1u << (frame % 32))) != 0;
}

int PhysicalMemoryManager::FirstFree() {
    uint32_t dwords = (total_frames + 31) / 32;
    for (uint32_t i = 0; i < dwords; i++) {
        if (bitmap[i] != 0xFFFFFFFF) {
            uint32_t inv = ~bitmap[i];
            for (int b = 0; b < 32; b++) {
                if (inv & (1u << b)) {
                    uint32_t frame = i * 32 + (uint32_t)b;
                    if (frame < total_frames)
                        return (int)frame;
                }
            }
        }
    }
    return -1;
}

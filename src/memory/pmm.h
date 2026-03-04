#ifndef __PMM_H
#define __PMM_H

#include "types.h"

class PhysicalMemoryManager {
public:
    static void     Init(uint32_t bitmap_base, uint32_t mem_size);
    static uint32_t AllocFrame();
    static void     FreeFrame(uint32_t physical_addr);
    static void     MarkUsed(uint32_t physical_addr, uint32_t length);
    static void     MarkFree(uint32_t physical_addr, uint32_t length);
    static uint32_t FreeFrameCount();
    static uint32_t TotalFrameCount();

private:
    static uint32_t* bitmap;
    static uint32_t  total_frames;
    static uint32_t  free_frames;

    static void SetBit(uint32_t frame);
    static void ClearBit(uint32_t frame);
    static bool TestBit(uint32_t frame);
    static int  FirstFree();
};

#endif

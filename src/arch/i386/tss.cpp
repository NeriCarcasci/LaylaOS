#include "tss.h"

TaskStateSegment::TaskStateSegment() {
    uint8_t* p = (uint8_t*)&tss;
    for (uint32_t i = 0; i < sizeof(TSS); i++) p[i] = 0;
    tss.ss0        = 0x10;
    tss.iomap_base = sizeof(TSS);
}

void TaskStateSegment::SetKernelStack(uint32_t esp) {
    tss.esp0 = esp;
}

uint32_t TaskStateSegment::GetBase() const {
    return (uint32_t)&tss;
}

uint32_t TaskStateSegment::GetSize() const {
    return sizeof(TSS);
}

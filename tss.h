#ifndef __TSS_H
#define __TSS_H

#include "types.h"

struct TSS {
    uint16_t link,    link_h;
    uint32_t esp0;
    uint16_t ss0,     ss0_h;
    uint32_t esp1;
    uint16_t ss1,     ss1_h;
    uint32_t esp2;
    uint16_t ss2,     ss2_h;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint16_t es,      es_h;
    uint16_t cs,      cs_h;
    uint16_t ss,      ss_h;
    uint16_t ds,      ds_h;
    uint16_t fs,      fs_h;
    uint16_t gs,      gs_h;
    uint16_t ldt,     ldt_h;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

class TaskStateSegment {
public:
    TaskStateSegment();
    void     SetKernelStack(uint32_t esp);
    uint32_t GetBase() const;
    uint32_t GetSize() const;
private:
    TSS tss;
};

#endif

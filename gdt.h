#ifndef __GDT_H
#define __GDT_H

#include "types.h"

class GlobalDescriptorTable {
private:
    struct GdtEntry {
        uint16_t limit_low;
        uint16_t base_low;
        uint8_t  base_mid;
        uint8_t  access;
        uint8_t  granularity;
        uint8_t  base_high;
    } __attribute__((packed));

    struct GdtDescriptor {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed));

    GdtEntry null_descriptor;
    GdtEntry code_descriptor;
    GdtEntry data_descriptor;
    GdtDescriptor gdtr;

public:
    GlobalDescriptorTable();
    uint16_t CodeSegmentSelector();
    uint16_t DataSegmentSelector();
};

#endif

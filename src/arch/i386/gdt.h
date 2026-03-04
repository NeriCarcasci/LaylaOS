#ifndef __GDT_H
#define __GDT_H

#include "types.h"

class TaskStateSegment;

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
    GdtEntry user_code_descriptor;
    GdtEntry user_data_descriptor;
    GdtEntry tss_descriptor;
    GdtDescriptor gdtr;

    void EncodeEntry(GdtEntry* e, uint32_t base, uint32_t limit,
                     uint8_t access, uint8_t gran_flags);

public:
    GlobalDescriptorTable(TaskStateSegment* tss = nullptr);
    uint16_t CodeSegmentSelector();
    uint16_t DataSegmentSelector();
    uint16_t UserCodeSegmentSelector();
    uint16_t UserDataSegmentSelector();
    uint16_t TSSSelector();
};

#endif

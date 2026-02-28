#include "gdt.h"

extern "C" void gdt_flush(uint32_t gdtr_ptr);

GlobalDescriptorTable::GlobalDescriptorTable() {
    null_descriptor.limit_low   = 0;
    null_descriptor.base_low    = 0;
    null_descriptor.base_mid    = 0;
    null_descriptor.access      = 0;
    null_descriptor.granularity = 0;
    null_descriptor.base_high   = 0;

    code_descriptor.limit_low   = 0xFFFFF & 0xFFFF;
    code_descriptor.base_low    = 0;
    code_descriptor.base_mid    = 0;
    code_descriptor.access      = 0x9A;
    code_descriptor.granularity = (0xCF & 0xF0) | ((0xFFFFF >> 16) & 0x0F);
    code_descriptor.base_high   = 0;

    data_descriptor.limit_low   = 0xFFFFF & 0xFFFF;
    data_descriptor.base_low    = 0;
    data_descriptor.base_mid    = 0;
    data_descriptor.access      = 0x92;
    data_descriptor.granularity = (0xCF & 0xF0) | ((0xFFFFF >> 16) & 0x0F);
    data_descriptor.base_high   = 0;

    gdtr.limit = sizeof(GdtEntry) * 3 - 1;
    gdtr.base  = (uint32_t)&null_descriptor;

    gdt_flush((uint32_t)&gdtr);
}

uint16_t GlobalDescriptorTable::CodeSegmentSelector() {
    return (uint8_t*)&code_descriptor - (uint8_t*)&null_descriptor;
}

uint16_t GlobalDescriptorTable::DataSegmentSelector() {
    return (uint8_t*)&data_descriptor - (uint8_t*)&null_descriptor;
}

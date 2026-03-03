#include "gdt.h"
#include "tss.h"

extern "C" void gdt_flush(uint32_t gdtr_ptr);

void GlobalDescriptorTable::EncodeEntry(GdtEntry* e, uint32_t base, uint32_t limit,
                                         uint8_t access, uint8_t gran_flags) {
    e->limit_low   = limit & 0xFFFF;
    e->base_low    = base & 0xFFFF;
    e->base_mid    = (base >> 16) & 0xFF;
    e->access      = access;
    e->granularity = (gran_flags & 0xF0) | ((limit >> 16) & 0x0F);
    e->base_high   = (base >> 24) & 0xFF;
}

GlobalDescriptorTable::GlobalDescriptorTable(TaskStateSegment* tss) {
    EncodeEntry(&null_descriptor,      0, 0,       0x00, 0x00);
    EncodeEntry(&code_descriptor,      0, 0xFFFFF, 0x9A, 0xC0);
    EncodeEntry(&data_descriptor,      0, 0xFFFFF, 0x92, 0xC0);
    EncodeEntry(&user_code_descriptor, 0, 0xFFFFF, 0xFA, 0xC0);
    EncodeEntry(&user_data_descriptor, 0, 0xFFFFF, 0xF2, 0xC0);

    if (tss != nullptr) {
        uint32_t base  = tss->GetBase();
        uint32_t limit = tss->GetSize() - 1;
        tss_descriptor.limit_low   = limit & 0xFFFF;
        tss_descriptor.base_low    = base & 0xFFFF;
        tss_descriptor.base_mid    = (base >> 16) & 0xFF;
        tss_descriptor.access      = 0x89;
        tss_descriptor.granularity = (limit >> 16) & 0x0F;
        tss_descriptor.base_high   = (base >> 24) & 0xFF;
    } else {
        EncodeEntry(&tss_descriptor, 0, 0, 0x00, 0x00);
    }

    gdtr.limit = sizeof(GdtEntry) * 6 - 1;
    gdtr.base  = (uint32_t)&null_descriptor;

    gdt_flush((uint32_t)&gdtr);

    if (tss != nullptr) {
        uint16_t sel = TSSSelector();
        __asm__ volatile("ltr %0" :: "r"(sel));
    }
}

uint16_t GlobalDescriptorTable::CodeSegmentSelector() {
    return (uint8_t*)&code_descriptor - (uint8_t*)&null_descriptor;
}

uint16_t GlobalDescriptorTable::DataSegmentSelector() {
    return (uint8_t*)&data_descriptor - (uint8_t*)&null_descriptor;
}

uint16_t GlobalDescriptorTable::UserCodeSegmentSelector() {
    return ((uint8_t*)&user_code_descriptor - (uint8_t*)&null_descriptor) | 3;
}

uint16_t GlobalDescriptorTable::UserDataSegmentSelector() {
    return ((uint8_t*)&user_data_descriptor - (uint8_t*)&null_descriptor) | 3;
}

uint16_t GlobalDescriptorTable::TSSSelector() {
    return (uint8_t*)&tss_descriptor - (uint8_t*)&null_descriptor;
}

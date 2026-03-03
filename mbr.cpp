#include "mbr.h"

static void mem_copy(uint8_t* dst, uint8_t* src, uint32_t n) {
    for (uint32_t i = 0; i < n; i++)
        dst[i] = src[i];
}

MBRParser::MBRParser(ATADriver* ata) : ata(ata), valid(false) {}

bool MBRParser::Read() {
    uint8_t buffer[512];
    ata->Read28(0, buffer, 1);
    mem_copy((uint8_t*)&mbr, buffer, 512);
    valid = (mbr.signature == 0xAA55);
    return valid;
}

uint8_t MBRParser::PartitionCount() {
    uint8_t count = 0;
    for (int i = 0; i < 4; i++)
        if (mbr.partitions[i].type != 0x00)
            count++;
    return count;
}

MBRPartitionEntry MBRParser::GetPartition(uint8_t index) {
    if (index < 4)
        return mbr.partitions[index];
    MBRPartitionEntry empty = {};
    return empty;
}

bool MBRParser::IsValid() {
    return valid;
}

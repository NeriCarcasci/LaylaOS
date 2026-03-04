#ifndef __MBR_H
#define __MBR_H

#include "types.h"
#include "ata.h"

struct MBRPartitionEntry {
    uint8_t  status;
    uint8_t  chs_first[3];
    uint8_t  type;
    uint8_t  chs_last[3];
    uint32_t lba_start;
    uint32_t sector_count;
} __attribute__((packed));

struct MBR {
    uint8_t           bootstrap[446];
    MBRPartitionEntry partitions[4];
    uint16_t          signature;
} __attribute__((packed));

class MBRParser {
public:
    MBRParser(ATADriver* ata);
    bool              Read();
    uint8_t           PartitionCount();
    MBRPartitionEntry GetPartition(uint8_t index);
    bool              IsValid();
private:
    ATADriver* ata;
    MBR        mbr;
    bool       valid;
};

#endif

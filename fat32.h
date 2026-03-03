#ifndef __FAT32_H
#define __FAT32_H

#include "types.h"
#include "ata.h"
#include "mbr.h"

struct FAT32BPB {
    uint8_t  jump[3];
    uint8_t  oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    uint8_t  volume_label[11];
    uint8_t  fs_type[8];
} __attribute__((packed));

struct FAT32DirEntry {
    uint8_t  name[8];
    uint8_t  ext[3];
    uint8_t  attributes;
    uint8_t  reserved;
    uint8_t  create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t cluster_low;
    uint32_t size;
} __attribute__((packed));

class FAT32 {
public:
    FAT32(ATADriver* ata, MBRPartitionEntry* partition);
    bool Mount();
    bool ReadFile(const char* path, uint8_t* buffer, uint32_t* size);
    bool ListDirectory(const char* path);
private:
    ATADriver*  ata;
    uint32_t    partition_lba;
    FAT32BPB    bpb;
    uint32_t    fat_lba;
    uint32_t    data_lba;
    uint32_t    cluster_size;

    uint32_t ClusterToLBA(uint32_t cluster);
    uint32_t NextCluster(uint32_t cluster);
    bool     ReadCluster(uint32_t cluster, uint8_t* buffer);
    bool     FindEntry(uint32_t dir_cluster, const char* name, FAT32DirEntry* out);
    bool     ReadSector(uint32_t lba, uint8_t* buffer);
    static void ToShortName(const char* component, char out[11]);
    static bool NamesMatch(const uint8_t* dir_name, const char* name83);
};

#endif

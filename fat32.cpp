#include "fat32.h"
#include "vga.h"

static void fat_mem_copy(uint8_t* dst, const uint8_t* src, uint32_t n) {
    for (uint32_t i = 0; i < n; i++)
        dst[i] = src[i];
}

FAT32::FAT32(ATADriver* ata, MBRPartitionEntry* partition)
    : ata(ata),
      partition_lba(partition->lba_start),
      fat_lba(0),
      data_lba(0),
      cluster_size(0)
{
}

bool FAT32::ReadSector(uint32_t lba, uint8_t* buffer) {
    return ata->Read28(lba, buffer, 1);
}

bool FAT32::Mount() {
    uint8_t sector[512];
    ReadSector(partition_lba, sector);
    fat_mem_copy((uint8_t*)&bpb, sector, sizeof(FAT32BPB));
    fat_lba      = partition_lba + bpb.reserved_sectors;
    data_lba     = fat_lba + (uint32_t)bpb.fat_count * bpb.fat_size_32;
    cluster_size = (uint32_t)bpb.sectors_per_cluster * 512;
    return bpb.bytes_per_sector == 512;
}

uint32_t FAT32::ClusterToLBA(uint32_t cluster) {
    return data_lba + (cluster - 2) * bpb.sectors_per_cluster;
}

uint32_t FAT32::NextCluster(uint32_t cluster) {
    uint8_t sector[512];
    uint32_t fat_offset  = cluster * 4;
    uint32_t fat_sector  = fat_lba + (fat_offset / 512);
    uint32_t entry_offset = fat_offset % 512;
    ReadSector(fat_sector, sector);
    uint32_t next = *(uint32_t*)(sector + entry_offset) & 0x0FFFFFFF;
    return next;
}

bool FAT32::ReadCluster(uint32_t cluster, uint8_t* buffer) {
    for (uint8_t i = 0; i < bpb.sectors_per_cluster; i++)
        ReadSector(ClusterToLBA(cluster) + i, buffer + (uint32_t)i * 512);
    return true;
}

void FAT32::ToShortName(const char* component, char out[11]) {
    for (int i = 0; i < 11; i++)
        out[i] = ' ';
    int i = 0, o = 0;
    while (component[i] != '\0' && component[i] != '.' && o < 8) {
        char c = component[i++];
        if (c >= 'a' && c <= 'z') c -= 32;
        out[o++] = c;
    }
    if (component[i] == '.') {
        i++;
        o = 8;
        while (component[i] != '\0' && o < 11) {
            char c = component[i++];
            if (c >= 'a' && c <= 'z') c -= 32;
            out[o++] = c;
        }
    }
}

bool FAT32::NamesMatch(const uint8_t* dir_name, const char* name83) {
    for (int i = 0; i < 11; i++)
        if (dir_name[i] != (uint8_t)name83[i])
            return false;
    return true;
}

bool FAT32::FindEntry(uint32_t dir_cluster, const char* name, FAT32DirEntry* out) {
    char name83[11];
    ToShortName(name, name83);

    uint8_t cluster_buf[4096];
    uint32_t cluster = dir_cluster;

    while (cluster < 0x0FFFFFF8) {
        ReadCluster(cluster, cluster_buf);
        uint32_t entries_per_cluster = cluster_size / 32;
        FAT32DirEntry* entries = (FAT32DirEntry*)cluster_buf;

        for (uint32_t i = 0; i < entries_per_cluster; i++) {
            if (entries[i].name[0] == 0x00)
                return false;
            if (entries[i].name[0] == 0xE5)
                continue;
            if (entries[i].attributes == 0x0F)
                continue;
            if (NamesMatch(entries[i].name, name83)) {
                fat_mem_copy((uint8_t*)out, (uint8_t*)&entries[i], sizeof(FAT32DirEntry));
                return true;
            }
        }
        cluster = NextCluster(cluster);
    }
    return false;
}

bool FAT32::ReadFile(const char* path, uint8_t* buffer, uint32_t* size) {
    uint32_t dir_cluster = bpb.root_cluster;
    FAT32DirEntry entry;

    int i = 0;
    if (path[0] == '/') i++;

    while (path[i] != '\0') {
        char component[13];
        int j = 0;
        while (path[i] != '\0' && path[i] != '/' && j < 12)
            component[j++] = path[i++];
        component[j] = '\0';
        if (path[i] == '/') i++;

        if (!FindEntry(dir_cluster, component, &entry))
            return false;

        if (path[i] == '\0') {
            *size = entry.size;
            uint32_t cluster = ((uint32_t)entry.cluster_high << 16) | entry.cluster_low;
            uint32_t written = 0;
            while (cluster < 0x0FFFFFF8) {
                ReadCluster(cluster, buffer + written);
                written += cluster_size;
                cluster = NextCluster(cluster);
            }
            return true;
        } else {
            dir_cluster = ((uint32_t)entry.cluster_high << 16) | entry.cluster_low;
        }
    }
    return false;
}

bool FAT32::ListDirectory(const char* path) {
    uint32_t dir_cluster = bpb.root_cluster;
    FAT32DirEntry entry;

    int i = 0;
    if (path[0] == '/') i++;

    while (path[i] != '\0') {
        char component[13];
        int j = 0;
        while (path[i] != '\0' && path[i] != '/' && j < 12)
            component[j++] = path[i++];
        component[j] = '\0';
        if (path[i] == '/') i++;

        if (!FindEntry(dir_cluster, component, &entry))
            return false;
        dir_cluster = ((uint32_t)entry.cluster_high << 16) | entry.cluster_low;
    }

    uint8_t cluster_buf[4096];
    uint32_t cluster = dir_cluster;

    while (cluster < 0x0FFFFFF8) {
        ReadCluster(cluster, cluster_buf);
        uint32_t entries_per_cluster = cluster_size / 32;
        FAT32DirEntry* entries = (FAT32DirEntry*)cluster_buf;

        for (uint32_t j = 0; j < entries_per_cluster; j++) {
            if (entries[j].name[0] == 0x00) return true;
            if (entries[j].name[0] == 0xE5) continue;
            if (entries[j].attributes == 0x0F) continue;

            for (int k = 0; k < 8; k++) {
                if (entries[j].name[k] != ' ')
                    VGA::PutChar((char)entries[j].name[k]);
            }
            if (entries[j].ext[0] != ' ') {
                VGA::PutChar('.');
                for (int k = 0; k < 3; k++) {
                    if (entries[j].ext[k] != ' ')
                        VGA::PutChar((char)entries[j].ext[k]);
                }
            }
            VGA::Print("  ");
        }
        cluster = NextCluster(cluster);
    }
    return true;
}

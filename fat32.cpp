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

// ─── write helpers ──────────────────────────────────────────────────────────

bool FAT32::WriteSector(uint32_t lba, const uint8_t* buffer) {
    return ata->Write28(lba, (uint8_t*)buffer, 1);
}

bool FAT32::WriteCluster(uint32_t cluster, const uint8_t* buffer) {
    uint32_t lba = ClusterToLBA(cluster);
    for (uint8_t i = 0; i < bpb.sectors_per_cluster; i++) {
        if (!WriteSector(lba + i, buffer + (uint32_t)i * 512))
            return false;
    }
    return true;
}

uint32_t FAT32::AllocCluster() {
    uint8_t sector[512];
    uint32_t total_sectors = bpb.total_sectors_32
                             ? bpb.total_sectors_32
                             : (uint32_t)bpb.total_sectors_16;
    uint32_t data_offset  = data_lba - partition_lba;
    uint32_t max_cluster  = 2 + (total_sectors > data_offset
                                 ? (total_sectors - data_offset) / bpb.sectors_per_cluster
                                 : 0);

    for (uint32_t s = 0; s < bpb.fat_size_32; s++) {
        if (!ReadSector(fat_lba + s, sector)) continue;
        uint32_t* entries = (uint32_t*)sector;
        for (uint32_t i = 0; i < 128; i++) {
            uint32_t cluster = s * 128 + i;
            if (cluster < 3 || cluster >= max_cluster) continue;
            if ((entries[i] & 0x0FFFFFFF) == 0)
                return cluster;
        }
    }
    return 0;
}

bool FAT32::SetFATEntry(uint32_t cluster, uint32_t value) {
    uint8_t sector[512];
    uint32_t fat_offset   = cluster * 4;
    uint32_t fat_sector   = fat_lba + fat_offset / 512;
    uint32_t entry_offset = fat_offset % 512;

    if (!ReadSector(fat_sector, sector)) return false;
    *(uint32_t*)(sector + entry_offset) = value & 0x0FFFFFFF;
    if (!WriteSector(fat_sector, sector)) return false;

    if (bpb.fat_count > 1) {
        uint32_t fat2_sector = fat_lba + bpb.fat_size_32 + fat_offset / 512;
        if (!ReadSector(fat2_sector, sector)) return false;
        *(uint32_t*)(sector + entry_offset) = value & 0x0FFFFFFF;
        if (!WriteSector(fat2_sector, sector)) return false;
    }
    return true;
}

bool FAT32::WriteClusterChain(uint32_t first_cluster,
                               const uint8_t* buf, uint32_t size) {
    uint8_t cluster_buf[4096];
    uint32_t buf_size = cluster_size < 4096 ? cluster_size : 4096;
    uint32_t cluster  = first_cluster;

    if (size == 0) {
        for (uint32_t i = 0; i < buf_size; i++) cluster_buf[i] = 0;
        WriteCluster(cluster, cluster_buf);
        return SetFATEntry(cluster, 0x0FFFFFFF);
    }

    uint32_t written = 0;
    while (written < size) {
        uint32_t to_write = size - written;
        if (to_write > buf_size) to_write = buf_size;
        for (uint32_t i = 0; i < buf_size; i++)
            cluster_buf[i] = (i < to_write) ? buf[written + i] : 0;
        if (!WriteCluster(cluster, cluster_buf)) return false;
        written += to_write;

        if (written < size) {
            uint32_t next = AllocCluster();
            if (next == 0) return false;
            if (!SetFATEntry(cluster, next)) return false;
            if (!SetFATEntry(next, 0x0FFFFFFF)) return false;
            cluster = next;
        } else {
            if (!SetFATEntry(cluster, 0x0FFFFFFF)) return false;
        }
    }
    return true;
}

void FAT32::BuildDirEntry(FAT32DirEntry* out, const char* name83,
                           uint8_t attr, uint32_t cluster, uint32_t size) {
    for (int i = 0; i < 8; i++) out->name[i] = (uint8_t)name83[i];
    for (int i = 0; i < 3; i++) out->ext[i]  = (uint8_t)name83[8 + i];
    out->attributes        = attr;
    out->reserved          = 0;
    out->create_time_tenth = 0;
    out->create_time       = 0x0000;
    out->create_date       = 0x4A21;
    out->access_date       = 0x4A21;
    out->cluster_high      = (uint16_t)(cluster >> 16);
    out->write_time        = 0x0000;
    out->write_date        = 0x4A21;
    out->cluster_low       = (uint16_t)(cluster & 0xFFFF);
    out->size              = size;
}

bool FAT32::AddDirEntry(uint32_t dir_cluster, FAT32DirEntry* entry) {
    uint8_t cluster_buf[4096];
    uint32_t cluster      = dir_cluster;
    uint32_t prev_cluster = 0;
    uint32_t buf_size     = cluster_size < 4096 ? cluster_size : 4096;

    while (cluster < 0x0FFFFFF8) {
        if (!ReadCluster(cluster, cluster_buf)) return false;
        uint32_t epc = buf_size / 32;
        FAT32DirEntry* entries = (FAT32DirEntry*)cluster_buf;
        for (uint32_t i = 0; i < epc; i++) {
            if (entries[i].name[0] == 0x00 ||
                entries[i].name[0] == (uint8_t)0xE5) {
                fat_mem_copy((uint8_t*)&entries[i],
                             (uint8_t*)entry, sizeof(FAT32DirEntry));
                return WriteCluster(cluster, cluster_buf);
            }
        }
        prev_cluster = cluster;
        cluster = NextCluster(cluster);
    }

    uint32_t new_cluster = AllocCluster();
    if (new_cluster == 0) return false;
    for (uint32_t i = 0; i < buf_size; i++) cluster_buf[i] = 0;
    fat_mem_copy(cluster_buf, (uint8_t*)entry, sizeof(FAT32DirEntry));
    if (!SetFATEntry(prev_cluster, new_cluster)) return false;
    if (!SetFATEntry(new_cluster, 0x0FFFFFFF)) return false;
    return WriteCluster(new_cluster, cluster_buf);
}

bool FAT32::MarkEntryDeleted(uint32_t dir_cluster, const char* name83) {
    uint8_t cluster_buf[4096];
    uint32_t cluster  = dir_cluster;
    uint32_t buf_size = cluster_size < 4096 ? cluster_size : 4096;

    while (cluster < 0x0FFFFFF8) {
        if (!ReadCluster(cluster, cluster_buf)) return false;
        uint32_t epc = buf_size / 32;
        FAT32DirEntry* entries = (FAT32DirEntry*)cluster_buf;
        for (uint32_t i = 0; i < epc; i++) {
            if (entries[i].name[0] == 0x00) return false;
            if (entries[i].name[0] == (uint8_t)0xE5) continue;
            if (NamesMatch(entries[i].name, name83)) {
                entries[i].name[0] = (uint8_t)0xE5;
                return WriteCluster(cluster, cluster_buf);
            }
        }
        cluster = NextCluster(cluster);
    }
    return false;
}

bool FAT32::ResolveParentDir(const char* path,
                              uint32_t* dir_cluster_out,
                              char component_out[13]) {
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

        if (path[i] == '\0') {
            for (int k = 0; k <= j; k++) component_out[k] = component[k];
            *dir_cluster_out = dir_cluster;
            return true;
        }
        if (!FindEntry(dir_cluster, component, &entry)) return false;
        dir_cluster = ((uint32_t)entry.cluster_high << 16) | entry.cluster_low;
    }
    return false;
}

// ─── public write API ────────────────────────────────────────────────────────

bool FAT32::WriteFile(const char* path, const uint8_t* buffer, uint32_t size) {
    uint32_t dir_cluster;
    char component[13];
    if (!ResolveParentDir(path, &dir_cluster, component)) return false;

    char name83[11];
    ToShortName(component, name83);

    FAT32DirEntry existing;
    bool exists = FindEntry(dir_cluster, component, &existing);
    if (exists) {
        uint32_t c = ((uint32_t)existing.cluster_high << 16) | existing.cluster_low;
        while (c >= 2 && c < 0x0FFFFFF8) {
            uint32_t next = NextCluster(c);
            SetFATEntry(c, 0);
            c = next;
        }
    }

    uint32_t first = AllocCluster();
    if (first == 0) return false;
    if (!SetFATEntry(first, 0x0FFFFFFF)) return false;
    if (!WriteClusterChain(first, buffer, size)) return false;
    if (exists) MarkEntryDeleted(dir_cluster, name83);

    FAT32DirEntry new_entry;
    BuildDirEntry(&new_entry, name83, 0x20, first, size);
    return AddDirEntry(dir_cluster, &new_entry);
}

bool FAT32::DeleteFile(const char* path) {
    uint32_t dir_cluster;
    char component[13];
    if (!ResolveParentDir(path, &dir_cluster, component)) return false;

    char name83[11];
    ToShortName(component, name83);

    FAT32DirEntry entry;
    if (!FindEntry(dir_cluster, component, &entry)) return false;

    uint32_t c = ((uint32_t)entry.cluster_high << 16) | entry.cluster_low;
    while (c >= 2 && c < 0x0FFFFFF8) {
        uint32_t next = NextCluster(c);
        SetFATEntry(c, 0);
        c = next;
    }
    return MarkEntryDeleted(dir_cluster, name83);
}

bool FAT32::CreateFile(const char* path) {
    return WriteFile(path, nullptr, 0);
}

bool FAT32::MakeDir(const char* path) {
    uint32_t dir_cluster;
    char component[13];
    if (!ResolveParentDir(path, &dir_cluster, component)) return false;

    char name83[11];
    ToShortName(component, name83);

    uint32_t new_cluster = AllocCluster();
    if (new_cluster == 0) return false;
    if (!SetFATEntry(new_cluster, 0x0FFFFFFF)) return false;

    uint8_t cluster_buf[4096];
    uint32_t buf_size = cluster_size < 4096 ? cluster_size : 4096;
    for (uint32_t i = 0; i < buf_size; i++) cluster_buf[i] = 0;

    uint32_t parent_for_dotdot = (dir_cluster == bpb.root_cluster) ? 0 : dir_cluster;

    FAT32DirEntry dot;
    BuildDirEntry(&dot,   ".          ", 0x10, new_cluster,      0);
    FAT32DirEntry dotdot;
    BuildDirEntry(&dotdot,"..         ", 0x10, parent_for_dotdot, 0);
    fat_mem_copy(cluster_buf,      (uint8_t*)&dot,    32);
    fat_mem_copy(cluster_buf + 32, (uint8_t*)&dotdot, 32);

    if (!WriteCluster(new_cluster, cluster_buf)) return false;

    FAT32DirEntry dir_entry;
    BuildDirEntry(&dir_entry, name83, 0x10, new_cluster, 0);
    return AddDirEntry(dir_cluster, &dir_entry);
}

bool FAT32::ListDirectoryToBuffer(const char* path, char* buf, uint32_t* len) {
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
        if (!FindEntry(dir_cluster, component, &entry)) return false;
        dir_cluster = ((uint32_t)entry.cluster_high << 16) | entry.cluster_low;
    }

    uint8_t cluster_buf[4096];
    uint32_t cluster  = dir_cluster;
    uint32_t buf_size = cluster_size < 4096 ? cluster_size : 4096;
    uint32_t out_pos  = 0;
    bool     done     = false;

    while (cluster < 0x0FFFFFF8 && !done) {
        ReadCluster(cluster, cluster_buf);
        uint32_t epc = buf_size / 32;
        FAT32DirEntry* entries = (FAT32DirEntry*)cluster_buf;

        for (uint32_t j = 0; j < epc; j++) {
            if (entries[j].name[0] == 0x00) { done = true; break; }
            if (entries[j].name[0] == (uint8_t)0xE5) continue;
            if (entries[j].attributes == 0x0F) continue;
            if (entries[j].name[0] == '.') continue;

            for (int k = 0; k < 8; k++)
                if (entries[j].name[k] != ' ')
                    buf[out_pos++] = (char)entries[j].name[k];
            if (entries[j].ext[0] != ' ') {
                buf[out_pos++] = '.';
                for (int k = 0; k < 3; k++)
                    if (entries[j].ext[k] != ' ')
                        buf[out_pos++] = (char)entries[j].ext[k];
            }
            if (entries[j].attributes & 0x10)
                buf[out_pos++] = '/';
            buf[out_pos++] = '\n';
        }
        cluster = NextCluster(cluster);
    }
    *len = out_pos;
    return true;
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

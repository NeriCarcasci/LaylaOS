#ifndef __PAGING_H
#define __PAGING_H

#include "types.h"

// NOTE: this kernel uses identity mapping (virtual == physical for all kernel
// addresses). User processes are isolated by having their own page directory
// with distinct mappings at 0x00400000. This is a deliberate simplification —
// a higher-half kernel with separate virtual address spaces is the natural
// follow-on.

const uint32_t PAGE_PRESENT = 0x001;
const uint32_t PAGE_WRITE   = 0x002;
const uint32_t PAGE_USER    = 0x004;
const uint32_t PAGE_KERNEL  = PAGE_PRESENT | PAGE_WRITE;
const uint32_t PAGE_USER_RW = PAGE_PRESENT | PAGE_WRITE | PAGE_USER;

class PageDirectory {
public:
    static PageDirectory* CreateKernel();
    static PageDirectory* CreateUser();
    ~PageDirectory();

    void     MapPage(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
    void     UnmapPage(uint32_t virtual_addr);
    uint32_t GetPhysical(uint32_t virtual_addr) const;
    bool     IsMapped(uint32_t virtual_addr) const;
    uint32_t GetCR3() const;

private:
    uint32_t* phys_dir;          // 4KB-aligned physical frame for this PD
    uint32_t* page_tables[1024]; // virtual ptrs to page table frames

    PageDirectory();
    uint32_t* GetOrCreatePageTable(uint32_t pd_index, uint32_t flags);
};

class PagingManager {
public:
    static void           Init();
    static void           Enable(PageDirectory* dir);
    static void           SwitchDirectory(PageDirectory* dir);
    static PageDirectory* GetKernelDirectory();
    static void           HandlePageFault(uint32_t faulting_addr, uint32_t error_code);

    static PageDirectory* kernel_dir;
};

#endif

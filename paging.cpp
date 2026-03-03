#include "paging.h"
#include "pmm.h"
#include "vga.h"

PageDirectory* PagingManager::kernel_dir = nullptr;

static void memzero32(uint32_t* ptr, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) ptr[i] = 0;
}

PageDirectory::PageDirectory() {
    uint32_t phys = PhysicalMemoryManager::AllocFrame();
    phys_dir = (uint32_t*)phys;
    memzero32(phys_dir, 1024);
    for (int i = 0; i < 1024; i++)
        page_tables[i] = nullptr;
}

PageDirectory::~PageDirectory() {
    for (int i = 0; i < 1024; i++) {
        if (page_tables[i]) {
            uint32_t* kpt = (PagingManager::kernel_dir && this != PagingManager::kernel_dir)
                            ? PagingManager::kernel_dir->page_tables[i] : nullptr;
            if (page_tables[i] != kpt)
                PhysicalMemoryManager::FreeFrame((uint32_t)page_tables[i]);
        }
    }
    PhysicalMemoryManager::FreeFrame((uint32_t)phys_dir);
}

uint32_t* PageDirectory::GetOrCreatePageTable(uint32_t pd_index, uint32_t flags) {
    if (phys_dir[pd_index] & PAGE_PRESENT)
        return page_tables[pd_index];

    uint32_t phys = PhysicalMemoryManager::AllocFrame();
    if (!phys) return nullptr;

    uint32_t* virt = (uint32_t*)phys;
    memzero32(virt, 1024);

    phys_dir[pd_index]    = phys | PAGE_PRESENT | PAGE_WRITE | (flags & PAGE_USER);
    page_tables[pd_index] = virt;
    return virt;
}

void PageDirectory::MapPage(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

    uint32_t* pt = GetOrCreatePageTable(pd_index, flags);
    if (!pt) return;

    pt[pt_index] = (physical_addr & 0xFFFFF000) | (flags & 0xFFF);
}

void PageDirectory::UnmapPage(uint32_t virtual_addr) {
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

    if (!(phys_dir[pd_index] & PAGE_PRESENT)) return;
    if (!page_tables[pd_index]) return;

    page_tables[pd_index][pt_index] = 0;
    __asm__ volatile("invlpg (%0)" :: "r"(virtual_addr) : "memory");
}

uint32_t PageDirectory::GetPhysical(uint32_t virtual_addr) const {
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

    if (!(phys_dir[pd_index] & PAGE_PRESENT)) return 0;
    if (!page_tables[pd_index]) return 0;

    uint32_t pte = page_tables[pd_index][pt_index];
    if (!(pte & PAGE_PRESENT)) return 0;
    return (pte & 0xFFFFF000) | (virtual_addr & 0xFFF);
}

bool PageDirectory::IsMapped(uint32_t virtual_addr) const {
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

    if (!(phys_dir[pd_index] & PAGE_PRESENT)) return false;
    if (!page_tables[pd_index]) return false;
    return (page_tables[pd_index][pt_index] & PAGE_PRESENT) != 0;
}

uint32_t PageDirectory::GetCR3() const {
    return (uint32_t)phys_dir;
}

PageDirectory* PageDirectory::CreateKernel() {
    PageDirectory* dir = new PageDirectory();

    for (uint32_t addr = 0x00000000; addr < 0x01000000; addr += 0x1000)
        dir->MapPage(addr, addr, PAGE_KERNEL);

    return dir;
}

PageDirectory* PageDirectory::CreateUser() {
    PageDirectory* dir = new PageDirectory();

    for (int i = 0; i < 1024; i++) {
        dir->phys_dir[i]    = PagingManager::kernel_dir->phys_dir[i];
        dir->page_tables[i] = PagingManager::kernel_dir->page_tables[i];
    }

    return dir;
}

void PagingManager::Init() {
    kernel_dir = PageDirectory::CreateKernel();
}

void PagingManager::Enable(PageDirectory* dir) {
    __asm__ volatile(
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or $0x80000001, %%eax\n"
        "mov %%eax, %%cr0\n"
        :: "r"(dir->GetCR3()) : "eax"
    );
}

void PagingManager::SwitchDirectory(PageDirectory* dir) {
    __asm__ volatile("mov %0, %%cr3\n" :: "r"(dir->GetCR3()) : "memory");
}

PageDirectory* PagingManager::GetKernelDirectory() {
    return kernel_dir;
}

void PagingManager::HandlePageFault(uint32_t faulting_addr, uint32_t error_code) {
    VGA::Print("PAGE FAULT at 0x");
    VGA::PrintHex32(faulting_addr);
    VGA::Print(" error=0x");
    VGA::PrintHex((uint8_t)error_code);
    VGA::Print("\n");
    __asm__ volatile("cli; hlt");
}

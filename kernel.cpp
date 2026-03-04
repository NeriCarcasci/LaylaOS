#include "types.h"
#include "gdt.h"
#include "tss.h"
#include "memorymanagement.h"
#include "allocator.h"
#include "interrupts.h"
#include "keyboard.h"
#include "mouse.h"
#include "pci.h"
#include "driver.h"
#include "vga.h"
#include "rtl8139.h"
#include "arp.h"
#include "ipv4.h"
#include "icmp.h"
#include "udp.h"
#include "tcp.h"
#include "process.h"
#include "scheduler.h"
#include "pmm.h"
#include "paging.h"
#include "keyboard_buffer.h"
#include "fat32.h"
#include "mbr.h"
#include "ata.h"
#include "gui.h"
#include "terminal.h"
#include "boot_anim.h"

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for (constructor* i = &start_ctors; i != &end_ctors; ++i)
        (*i)();
}

struct MultibootInfo
{
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint8_t  _pad[32];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} __attribute__((packed));

struct MemoryMapEntry
{
    uint32_t size;
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
} __attribute__((packed));

extern "C" uint8_t  _binary_shell_program_bin_start[];
extern "C" uint32_t _binary_shell_program_bin_size;

static uint32_t pmm_bitmap[32768];

FAT32* global_fat32 = nullptr;

extern "C" void kernelMain(void* multiboot_structure, uint32_t magic)
{
    TaskStateSegment tss;
    GlobalDescriptorTable gdt(&tss);

    MultibootInfo* mb   = (MultibootInfo*)multiboot_structure;
    uint32_t heap_start = 0;
    uint32_t heap_size  = 0;

    if (mb->flags & (1 << 6))
    {
        MemoryMapEntry* entry = (MemoryMapEntry*)mb->mmap_addr;
        MemoryMapEntry* end   = (MemoryMapEntry*)((uint8_t*)mb->mmap_addr + mb->mmap_length);

        while (entry < end)
        {
            if (entry->type == 1 && entry->base_addr < 0xFFFFFFFF)
            {
                uint32_t base = (uint32_t)entry->base_addr;
                uint32_t len  = (uint32_t)entry->length;

                if (base < 0x00200000)
                {
                    uint32_t delta = 0x00200000 - base;
                    if (delta >= len)
                    {
                        entry = (MemoryMapEntry*)((uint8_t*)entry + entry->size + 4);
                        continue;
                    }
                    base = 0x00200000;
                    len -= delta;
                }

                if (len > heap_size)
                {
                    heap_start = base;
                    heap_size  = len;
                }
            }
            entry = (MemoryMapEntry*)((uint8_t*)entry + entry->size + 4);
        }
    }

    // Cap heap to first 16MB; carve top 512KB for PMM page-frame pool
    if (heap_start + heap_size > 0x01000000)
        heap_size = 0x01000000 - heap_start;

    const uint32_t PMM_POOL_SIZE = 128 * 4096;
    uint32_t pmm_pool_base = heap_start + heap_size - PMM_POOL_SIZE;
    heap_size -= PMM_POOL_SIZE;

    MemoryManager mm(heap_start, heap_size);
    active_memory_manager = &mm;

    uint32_t total_mem = (uint32_t)mb->mem_upper * 1024 + 0x100000;
    PhysicalMemoryManager::Init((uint32_t)pmm_bitmap, total_mem);
    PhysicalMemoryManager::MarkFree(pmm_pool_base, PMM_POOL_SIZE);

    PagingManager::Init();
    PagingManager::Enable(PagingManager::GetKernelDirectory());

    VGA::Clear();
    VGA::Print("Kernel loaded\n");

    InterruptManager interrupts(0x20, &gdt);
    KeyboardBuffer::Init();
    KeyboardDriver   keyboard(&interrupts);
    MouseDriver      mouse(&interrupts);

    DriverManager driver_manager;

    PeripheralComponentInterconnect pci;
    pci.SelectDrivers(&driver_manager, &interrupts);
    driver_manager.ActivateAll();

    ATADriver* ata_drv = nullptr;
    for (int i = 0; i < driver_manager.Count(); i++) {
        Driver* d = driver_manager.Get(i);
        if (d && d->TypeID() == 0x0101) {
            ata_drv = (ATADriver*)d;
            break;
        }
    }

    if (ata_drv) {
        MBRParser mbr_parser(ata_drv);
        if (mbr_parser.Read() && mbr_parser.PartitionCount() > 0) {
            MBRPartitionEntry part = mbr_parser.GetPartition(0);
            global_fat32 = new FAT32(ata_drv, &part);
            if (!global_fat32->Mount()) {
                delete global_fat32;
                global_fat32 = nullptr;
            }
        }
    }


    RTL8139Driver* nic = nullptr;
    for (int i = 0; i < driver_manager.Count(); i++) {
        Driver* d = driver_manager.Get(i);
        if (d && d->TypeID() == 0x8139) {
            nic = (RTL8139Driver*)d;
            break;
        }
    }

    if (nic) {
        IPv4Address local_ip = IPv4Address::FromBytes(10, 0, 2, 15);

        ARPHandler  arp(nic, local_ip);
        IPv4Handler ipv4(nic, &arp, local_ip);
        ICMPHandler icmp(&ipv4);
        UDPSocket   udp(&ipv4, 1234);
        TCPSocket   tcp(&ipv4, 1234);

        nic->RegisterHandler(ETHERTYPE_ARP,  &arp);
        nic->RegisterHandler(ETHERTYPE_IPV4, &ipv4);
        ipv4.RegisterHandler(IPV4_PROTO_ICMP, &icmp);
        ipv4.RegisterHandler(IPV4_PROTO_UDP,  &udp);
        ipv4.RegisterHandler(IPV4_PROTO_TCP,  &tcp);

        arp.SendRequest(IPv4Address::FromBytes(10, 0, 2, 2));
    }

    uint8_t* load_addr = (uint8_t*)0x00400000;
    uint32_t prog_size = (uint32_t)&_binary_shell_program_bin_size;
    for (uint32_t i = 0; i < prog_size; i++)
        load_addr[i] = _binary_shell_program_bin_start[i];

    Port8Bit pit_cmd(0x43);
    Port8Bit pit_data(0x40);
    pit_cmd.Write(0x36);
    pit_data.Write(11932 & 0xFF);
    pit_data.Write(11932 >> 8);

    VGA::EnterGraphicsMode();
    BootAnimation();
    Desktop desktop(320, 200, Color::DarkGray);
    Terminal term(0, 0, 320, 200);
    Terminal::SetActive(&term);
    desktop.AddWindow(&term);
    SetActiveDesktop(&desktop);

    term.Print("MyOS ready.\n");

    Scheduler scheduler(&interrupts, &tss);

    Process shell_proc(0x00400000, 1);
    scheduler.AddProcess(&shell_proc);

    interrupts.Activate();
    while (1);
}

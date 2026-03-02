#include "types.h"
#include "gdt.h"
#include "memorymanagement.h"
#include "interrupts.h"
#include "keyboard.h"
#include "mouse.h"
#include "pci.h"

int vga_cursor = 0;

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
    uint8_t  _pad[20];
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

extern "C" void kernelMain(void* multiboot_structure, uint32_t magic)
{
    GlobalDescriptorTable gdt;

    MultibootInfo* mb      = (MultibootInfo*)multiboot_structure;
    uint32_t heap_start    = 0;
    uint32_t heap_size     = 0;

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

    MemoryManager mm(heap_start, heap_size);

    void* a = mm.malloc(128);
    void* b = mm.malloc(64);
    mm.free(a);
    void* c = mm.malloc(32);

    InterruptManager interrupts(0x20, &gdt);
    KeyboardDriver   keyboard(&interrupts);
    MouseDriver      mouse(&interrupts);

    PeripheralComponentInterconnect pci;
    pci.SelectDrivers(&interrupts);

    interrupts.Activate();
    while (1);
}

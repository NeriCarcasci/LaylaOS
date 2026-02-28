#include "types.h"
#include "gdt.h"
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

extern "C" void kernelMain(void* multiboot_structure, uint32_t magic)
{
    GlobalDescriptorTable gdt;
    InterruptManager interrupts(0x20, &gdt);
    KeyboardDriver keyboard(&interrupts);
    MouseDriver mouse(&interrupts);
    PeripheralComponentInterconnect pci;
    pci.SelectDrivers(&interrupts);
    interrupts.Activate();
    while(1);
}

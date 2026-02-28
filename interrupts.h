#ifndef __INTERRUPTS_H
#define __INTERRUPTS_H

#include "types.h"
#include "port.h"
#include "gdt.h"

class InterruptManager;

class InterruptHandler {
protected:
    uint8_t interrupt_number;
    InterruptManager* interrupt_manager;
    InterruptHandler(uint8_t interrupt, InterruptManager* manager);
    ~InterruptHandler();
public:
    virtual uint32_t HandleInterrupt(uint32_t esp);
};

class InterruptManager {
    friend class InterruptHandler;
private:
    struct GateDescriptor {
        uint16_t offset_low;
        uint16_t selector;
        uint8_t  reserved;
        uint8_t  type_attr;
        uint16_t offset_high;
    } __attribute__((packed));

    struct InterruptDescriptorTablePointer {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed));

    static GateDescriptor idt[256];
    static InterruptManager* active_manager;

    InterruptHandler* handlers[256];

    Port8BitSlow pic_master_command;
    Port8BitSlow pic_master_data;
    Port8BitSlow pic_slave_command;
    Port8BitSlow pic_slave_data;

    uint16_t hardware_interrupt_offset;

    static void SetGate(uint8_t interrupt, uint32_t handler, uint16_t selector, uint8_t type_attr);
    static uint32_t HandleInterrupt(uint8_t interrupt, uint32_t esp);
    uint32_t DoHandleInterrupt(uint8_t interrupt, uint32_t esp);

public:
    InterruptManager(uint16_t hardware_interrupt_offset, GlobalDescriptorTable* gdt);
    ~InterruptManager();
    uint16_t HardwareInterruptOffset();
    void Activate();
    void Deactivate();
};

#endif

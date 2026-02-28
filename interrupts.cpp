#include "interrupts.h"

extern "C" {
    void isr_32();  void isr_33();  void isr_34();  void isr_35();
    void isr_36();  void isr_37();  void isr_38();  void isr_39();
    void isr_40();  void isr_41();  void isr_42();  void isr_43();
    void isr_44();  void isr_45();  void isr_46();  void isr_47();
    void ignore_interrupt_request();
}

InterruptManager::GateDescriptor InterruptManager::idt[256];
InterruptManager* InterruptManager::active_manager = nullptr;

InterruptHandler::InterruptHandler(uint8_t interrupt, InterruptManager* manager) {
    interrupt_number   = interrupt;
    interrupt_manager  = manager;
    manager->handlers[interrupt] = this;
}

InterruptHandler::~InterruptHandler() {
    if (interrupt_manager->handlers[interrupt_number] == this)
        interrupt_manager->handlers[interrupt_number] = nullptr;
}

uint32_t InterruptHandler::HandleInterrupt(uint32_t esp) {
    return esp;
}

void InterruptManager::SetGate(uint8_t interrupt, uint32_t handler,
                                uint16_t selector, uint8_t type_attr) {
    idt[interrupt].offset_low  = handler & 0xFFFF;
    idt[interrupt].selector    = selector;
    idt[interrupt].reserved    = 0;
    idt[interrupt].type_attr   = type_attr;
    idt[interrupt].offset_high = (handler >> 16) & 0xFFFF;
}

InterruptManager::InterruptManager(uint16_t hw_offset, GlobalDescriptorTable* gdt)
    : hardware_interrupt_offset(hw_offset),
      pic_master_command(0x20), pic_master_data(0x21),
      pic_slave_command(0xA0),  pic_slave_data(0xA1)
{
    for (int i = 0; i < 256; i++)
        handlers[i] = nullptr;

    uint16_t selector = gdt->CodeSegmentSelector();

    for (int i = 0; i < 256; i++)
        SetGate(i, (uint32_t)ignore_interrupt_request, selector, 0x8E);

    void (*stubs[16])() = {
        isr_32, isr_33, isr_34, isr_35,
        isr_36, isr_37, isr_38, isr_39,
        isr_40, isr_41, isr_42, isr_43,
        isr_44, isr_45, isr_46, isr_47
    };
    for (int i = 0; i < 16; i++)
        SetGate(hw_offset + i, (uint32_t)stubs[i], selector, 0x8E);

    pic_master_command.Write(0x11);
    pic_slave_command.Write(0x11);

    pic_master_data.Write(hw_offset);
    pic_slave_data.Write(hw_offset + 8);

    pic_master_data.Write(0x04);
    pic_slave_data.Write(0x02);

    pic_master_data.Write(0x01);
    pic_slave_data.Write(0x01);

    pic_master_data.Write(0x00);
    pic_slave_data.Write(0x00);

    InterruptDescriptorTablePointer idtp;
    idtp.limit = 256 * sizeof(GateDescriptor) - 1;
    idtp.base  = (uint32_t)idt;
    __asm__ volatile("lidt %0" : : "m"(idtp));
}

InterruptManager::~InterruptManager() {
    Deactivate();
}

uint16_t InterruptManager::HardwareInterruptOffset() {
    return hardware_interrupt_offset;
}

void InterruptManager::Activate() {
    if (active_manager != nullptr)
        active_manager->Deactivate();
    active_manager = this;
    __asm__ volatile("sti");
}

void InterruptManager::Deactivate() {
    __asm__ volatile("cli");
    active_manager = nullptr;
}

uint32_t InterruptManager::HandleInterrupt(uint8_t interrupt, uint32_t esp) {
    if (active_manager != nullptr)
        return active_manager->DoHandleInterrupt(interrupt, esp);
    return esp;
}

uint32_t InterruptManager::DoHandleInterrupt(uint8_t interrupt, uint32_t esp) {
    if (handlers[interrupt] != nullptr)
        esp = handlers[interrupt]->HandleInterrupt(esp);

    if (interrupt >= hardware_interrupt_offset
     && interrupt <  hardware_interrupt_offset + 16)
    {
        if (interrupt >= hardware_interrupt_offset + 8)
            pic_slave_command.Write(0x20);
        pic_master_command.Write(0x20);
    }

    return esp;
}

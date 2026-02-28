#include "pci.h"

extern int vga_cursor;

static void vga_write_char(char c) {
    uint16_t* vga = (uint16_t*)0xB8000;
    if (c == '\n') {
        vga_cursor = (vga_cursor / 80 + 1) * 80;
    } else {
        vga[vga_cursor++] = 0x0F00 | (uint8_t)c;
    }
}

static void print_hex(uint8_t val) {
    static const char hex[] = "0123456789ABCDEF";
    vga_write_char(hex[val >> 4]);
    vga_write_char(hex[val & 0xF]);
}

static void print_str(const char* str) {
    for (int i = 0; str[i] != '\0'; i++)
        vga_write_char(str[i]);
}

PeripheralComponentInterconnect::PeripheralComponentInterconnect()
    : data_port(0x0CFC), address_port(0x0CF8)
{
}

uint32_t PeripheralComponentInterconnect::Read(
    uint16_t bus, uint16_t device, uint16_t func, uint32_t offset)
{
    uint32_t addr = (1 << 31) | ((uint32_t)bus << 16)
                  | ((uint32_t)device << 11) | ((uint32_t)func << 8)
                  | (offset & 0xFC);
    address_port.Write(addr);
    return data_port.Read() >> (8 * (offset % 4));
}

void PeripheralComponentInterconnect::Write(
    uint16_t bus, uint16_t device, uint16_t func, uint32_t offset, uint32_t value)
{
    address_port.Write((1 << 31) | ((uint32_t)bus << 16)
                      | ((uint32_t)device << 11) | ((uint32_t)func << 8)
                      | (offset & 0xFC));
    data_port.Write(value);
}

bool PeripheralComponentInterconnect::DeviceHasFunctions(uint16_t bus, uint16_t device) {
    return Read(bus, device, 0, 0x0E) & (1 << 7);
}

PCIDeviceDescriptor PeripheralComponentInterconnect::GetDeviceDescriptor(
    uint16_t bus, uint16_t device, uint16_t func)
{
    PCIDeviceDescriptor result;
    result.bus      = bus;
    result.device   = device;
    result.function = func;
    result.port_base = 0;
    result.mem_base  = 0;

    result.vendor_id   = Read(bus, device, func, 0x00);
    result.device_id   = Read(bus, device, func, 0x02);
    result.class_id    = Read(bus, device, func, 0x0B);
    result.subclass_id = Read(bus, device, func, 0x0A);
    result.interface_id = Read(bus, device, func, 0x09);
    result.revision    = Read(bus, device, func, 0x08);
    result.interrupt   = Read(bus, device, func, 0x3C);

    return result;
}

void PeripheralComponentInterconnect::SelectDrivers(InterruptManager* interrupts) {
    for (uint16_t bus = 0; bus < 8; bus++) {
        for (uint16_t device = 0; device < 32; device++) {
            int numFunctions = DeviceHasFunctions(bus, device) ? 8 : 1;
            for (int function = 0; function < numFunctions; function++) {
                PCIDeviceDescriptor desc = GetDeviceDescriptor(bus, device, function);
                if (desc.vendor_id == 0xFFFF)
                    continue;

                print_str("PCI 0x");
                print_hex(desc.class_id);
                print_hex(desc.subclass_id);
                vga_write_char('/');
                print_hex((uint8_t)(desc.vendor_id >> 8));
                print_hex((uint8_t)(desc.vendor_id & 0xFF));
                vga_write_char(':');
                print_hex((uint8_t)(desc.device_id >> 8));
                print_hex((uint8_t)(desc.device_id & 0xFF));
                vga_write_char('\n');
            }
        }
    }
}

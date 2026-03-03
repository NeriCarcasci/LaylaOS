#include "pci.h"
#include "vga.h"
#include "ata.h"
#include "rtl8139.h"

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

    result.vendor_id    = Read(bus, device, func, 0x00);
    result.device_id    = Read(bus, device, func, 0x02);
    result.class_id     = Read(bus, device, func, 0x0B);
    result.subclass_id  = Read(bus, device, func, 0x0A);
    result.interface_id = Read(bus, device, func, 0x09);
    result.revision     = Read(bus, device, func, 0x08);
    result.interrupt    = Read(bus, device, func, 0x3C);

    for (int i = 0; i < 6; i++) {
        result.port_base[i]      = 0;
        result.mem_base[i]       = 0;
        result.port_base_is_io[i]= false;
        result.bar_size[i]       = 0;

        uint32_t offset = 0x10 + i * 4;
        uint32_t bar = Read(bus, device, func, offset);
        if (bar == 0)
            continue;

        if (bar & 0x1) {
            result.port_base_is_io[i] = true;
            result.port_base[i] = bar & 0xFFFFFFFC;
            Write(bus, device, func, offset, 0xFFFFFFFF);
            uint32_t sized = Read(bus, device, func, offset);
            Write(bus, device, func, offset, bar);
            result.bar_size[i] = ~(sized & 0xFFFFFFFC) + 1;
        } else {
            result.port_base_is_io[i] = false;
            result.mem_base[i] = bar & 0xFFFFFFF0;
            Write(bus, device, func, offset, 0xFFFFFFFF);
            uint32_t sized = Read(bus, device, func, offset);
            Write(bus, device, func, offset, bar);
            result.bar_size[i] = ~(sized & 0xFFFFFFF0) + 1;
        }
    }

    return result;
}

Driver* PeripheralComponentInterconnect::GetDriver(
    PCIDeviceDescriptor& desc, InterruptManager* interrupts)
{
    if (desc.class_id == 0x01 && desc.subclass_id == 0x01)
        return new ATADriver(desc, interrupts);
    if (desc.class_id == 0x02 && desc.subclass_id == 0x00)
        return new RTL8139Driver(desc, interrupts);
    return nullptr;
}

void PeripheralComponentInterconnect::SelectDrivers(
    DriverManager* driver_manager, InterruptManager* interrupts)
{
    for (uint16_t bus = 0; bus < 8; bus++) {
        for (uint16_t device = 0; device < 32; device++) {
            int numFunctions = DeviceHasFunctions(bus, device) ? 8 : 1;
            for (int function = 0; function < numFunctions; function++) {
                PCIDeviceDescriptor desc = GetDeviceDescriptor(bus, device, function);
                if (desc.vendor_id == 0xFFFF)
                    continue;

                VGA::Print("PCI 0x");
                VGA::PrintHex(desc.class_id);
                VGA::PrintHex(desc.subclass_id);
                VGA::PutChar('/');
                VGA::PrintHex((uint8_t)(desc.vendor_id >> 8));
                VGA::PrintHex((uint8_t)(desc.vendor_id & 0xFF));
                VGA::PutChar(':');
                VGA::PrintHex((uint8_t)(desc.device_id >> 8));
                VGA::PrintHex((uint8_t)(desc.device_id & 0xFF));
                VGA::PutChar('\n');

                Driver* drv = GetDriver(desc, interrupts);
                if (drv != nullptr)
                    driver_manager->AddDriver(drv);
            }
        }
    }
}

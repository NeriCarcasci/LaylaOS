#ifndef __PCI_H
#define __PCI_H

#include "types.h"
#include "port.h"
#include "interrupts.h"

struct PCIDeviceDescriptor {
    uint32_t port_base;
    uint32_t mem_base;
    uint16_t bus;
    uint16_t device;
    uint16_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  class_id;
    uint8_t  subclass_id;
    uint8_t  interface_id;
    uint8_t  revision;
    uint8_t  interrupt;
};

class PeripheralComponentInterconnect {
private:
    Port32Bit data_port;
    Port32Bit address_port;

public:
    PeripheralComponentInterconnect();
    uint32_t Read(uint16_t bus, uint16_t device, uint16_t func, uint32_t offset);
    void     Write(uint16_t bus, uint16_t device, uint16_t func, uint32_t offset, uint32_t value);
    bool     DeviceHasFunctions(uint16_t bus, uint16_t device);
    PCIDeviceDescriptor GetDeviceDescriptor(uint16_t bus, uint16_t device, uint16_t func);
    void     SelectDrivers(InterruptManager* interrupts);
};

#endif

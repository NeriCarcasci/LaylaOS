#ifndef __RTL8139_H
#define __RTL8139_H

#include "types.h"
#include "net.h"
#include "port.h"
#include "interrupts.h"
#include "ethernet.h"
#include "pci.h"

class RTL8139Driver : public EthernetDriver, public InterruptHandler {
public:
    RTL8139Driver(PCIDeviceDescriptor& desc, InterruptManager* interrupts);
    void       Activate() override;
    bool       Send(MACAddress dst, uint16_t ethertype,
                    uint8_t* payload, uint32_t size) override;
    MACAddress GetMAC() override;
    uint32_t   HandleInterrupt(uint32_t esp) override;
    uint32_t   TypeID() override { return 0x8139; }
private:
    Port8Bit   cr_port;
    Port16Bit  isr_port;
    Port16Bit  imr_port;
    Port32Bit  rbstart_port;
    Port32Bit  tcr_port;
    Port32Bit  rcr_port;
    Port32Bit  tsd[4];
    Port32Bit  tsad[4];
    Port8Bit   config1_port;
    Port16Bit  capr_port;
    uint16_t   io_base;
    MACAddress mac;
    uint32_t   rx_read_ptr;

    void HwReset();
    void ReadMAC();
    void HandleReceive();
};

#endif

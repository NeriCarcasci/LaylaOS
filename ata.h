#ifndef __ATA_H
#define __ATA_H

#include "types.h"
#include "port.h"
#include "interrupts.h"
#include "driver.h"
#include "pci.h"

class ATADriver : public Driver, public InterruptHandler {
public:
    ATADriver(PCIDeviceDescriptor& dev, InterruptManager* interrupts);
    ~ATADriver();
    void     Activate() override;
    uint32_t TypeID() override { return 0x0101; }
    bool Read28(uint32_t lba, uint8_t* buffer, uint32_t count);
    bool Write28(uint32_t lba, uint8_t* buffer, uint32_t count);
    bool Identify(uint16_t* buffer);
private:
    Port16Bit data_port;
    Port8Bit  error_port;
    Port8Bit  sector_count_port;
    Port8Bit  lba_low_port;
    Port8Bit  lba_mid_port;
    Port8Bit  lba_hi_port;
    Port8Bit  device_port;
    Port8Bit  command_port;
    Port8Bit  control_port;

    bool master;

    bool WaitBSY();
    bool WaitDRQ();
    uint32_t HandleInterrupt(uint32_t esp) override;
};

#endif

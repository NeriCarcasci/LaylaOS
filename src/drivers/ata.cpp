#include "ata.h"
#include "vga.h"

ATADriver::ATADriver(PCIDeviceDescriptor& dev, InterruptManager* interrupts)
    : Driver(),
      InterruptHandler(interrupts->HardwareInterruptOffset() + 14, interrupts),
      data_port(0x1F0),
      error_port(0x1F1),
      sector_count_port(0x1F2),
      lba_low_port(0x1F3),
      lba_mid_port(0x1F4),
      lba_hi_port(0x1F5),
      device_port(0x1F6),
      command_port(0x1F7),
      control_port(0x3F6),
      master(true)
{
}

ATADriver::~ATADriver() {}

bool ATADriver::WaitBSY() {
    for (uint32_t i = 0; i < 0x100000; i++)
        if (!(command_port.Read() & 0x80)) return true;
    return false;
}

bool ATADriver::WaitDRQ() {
    for (uint32_t i = 0; i < 0x100000; i++)
        if (command_port.Read() & 0x08) return true;
    return false;
}

bool ATADriver::Identify(uint16_t* buffer) {
    device_port.Write(master ? 0xA0 : 0xB0);
    command_port.Write(0xEC);
    if (!WaitBSY()) return false;
    if (command_port.Read() == 0) return false;
    if (!WaitDRQ()) return false;
    for (int i = 0; i < 256; i++)
        buffer[i] = data_port.Read();
    return true;
}

bool ATADriver::Read28(uint32_t lba, uint8_t* buf, uint32_t count) {
    if (!WaitBSY()) return false;
    device_port.Write((master ? 0xE0 : 0xF0) | ((lba >> 24) & 0x0F));
    error_port.Write(0x00);
    sector_count_port.Write((uint8_t)count);
    lba_low_port.Write((uint8_t)(lba & 0xFF));
    lba_mid_port.Write((uint8_t)((lba >> 8) & 0xFF));
    lba_hi_port.Write((uint8_t)((lba >> 16) & 0xFF));
    command_port.Write(0x20);

    for (uint32_t s = 0; s < count; s++) {
        if (!WaitBSY()) return false;
        if (!WaitDRQ()) return false;
        uint16_t* dst = (uint16_t*)(buf + s * 512);
        for (int i = 0; i < 256; i++)
            dst[i] = data_port.Read();
    }
    return true;
}

bool ATADriver::Write28(uint32_t lba, uint8_t* buf, uint32_t count) {
    WaitBSY();
    device_port.Write((master ? 0xE0 : 0xF0) | ((lba >> 24) & 0x0F));
    error_port.Write(0x00);
    sector_count_port.Write((uint8_t)count);
    lba_low_port.Write((uint8_t)(lba & 0xFF));
    lba_mid_port.Write((uint8_t)((lba >> 8) & 0xFF));
    lba_hi_port.Write((uint8_t)((lba >> 16) & 0xFF));
    command_port.Write(0x30);

    for (uint32_t s = 0; s < count; s++) {
        WaitBSY();
        WaitDRQ();
        uint16_t* src = (uint16_t*)(buf + s * 512);
        for (int i = 0; i < 256; i++)
            data_port.Write(src[i]);
        command_port.Write(0xE7);
        WaitBSY();
    }
    return true;
}

uint32_t ATADriver::HandleInterrupt(uint32_t esp) {
    return esp;
}

void ATADriver::Activate() {
    uint16_t buf[256];
    if (Identify(buf))
        VGA::Print("ATA: drive identified\n");
    else
        VGA::Print("ATA: no drive found\n");
}

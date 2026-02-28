#include "mouse.h"

void MouseDriver::WaitWrite() {
    while (command_port.Read() & 0x02);
}

void MouseDriver::WaitRead() {
    while (!(command_port.Read() & 0x01));
}

MouseDriver::MouseDriver(InterruptManager* manager)
    : InterruptHandler(manager->HardwareInterruptOffset() + 0x0C, manager),
      data_port(0x60),
      command_port(0x64),
      offset(0),
      buttons(0),
      x(40),
      y(12)
{
    WaitWrite(); command_port.Write(0xA8);

    WaitWrite(); command_port.Write(0x20);
    WaitRead();
    uint8_t status = data_port.Read() | 0x02;
    WaitWrite(); command_port.Write(0x60);
    WaitWrite(); data_port.Write(status);

    WaitWrite(); command_port.Write(0xD4);
    WaitWrite(); data_port.Write(0xF4);
    WaitRead();  data_port.Read();
}

uint32_t MouseDriver::HandleInterrupt(uint32_t esp) {
    if ((command_port.Read() & 0x20) == 0)
        return esp;

    buffer[offset++] = data_port.Read();

    if (offset != 3)
        return esp;

    offset = 0;

    int8_t dx = (int8_t)buffer[1];
    int8_t dy = (int8_t)buffer[2];

    uint16_t* vga = (uint16_t*)0xB8000;
    vga[y * 80 + x / 2] = 0x0F00 | ' ';

    x += dx;
    y -= dy;

    if (x < 0)   x = 0;
    if (x >= 320) x = 319;
    if (y < 0)   y = 0;
    if (y >= 200) y = 199;

    vga[y * 80 + x / 2] = 0x0F00 | '+';

    return esp;
}

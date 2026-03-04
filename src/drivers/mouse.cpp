#include "mouse.h"
#include "gui.h"

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
      x(160),
      y(100)
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
    uint8_t btns = buffer[0] & 0x07;

    Desktop* desktop = GetActiveDesktop();
    if (desktop != nullptr)
        desktop->OnMouseMove((int32_t)dx, (int32_t)dy, btns);

    return esp;
}

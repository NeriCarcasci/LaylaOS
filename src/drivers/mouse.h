#ifndef __MOUSE_H
#define __MOUSE_H

#include "types.h"
#include "port.h"
#include "interrupts.h"

class MouseDriver : public InterruptHandler {
private:
    Port8Bit data_port;
    Port8Bit command_port;
    uint8_t  buffer[3];
    uint8_t  offset;
    uint8_t  buttons;
    int32_t  x;
    int32_t  y;

    void WaitWrite();
    void WaitRead();

public:
    MouseDriver(InterruptManager* manager);
    uint32_t HandleInterrupt(uint32_t esp);
};

#endif

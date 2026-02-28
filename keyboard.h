#ifndef __KEYBOARD_H
#define __KEYBOARD_H

#include "types.h"
#include "port.h"
#include "interrupts.h"

class KeyboardDriver : public InterruptHandler {
private:
    Port8Bit data_port;
    bool shift;
    bool caps_lock;
public:
    KeyboardDriver(InterruptManager* manager);
    uint32_t HandleInterrupt(uint32_t esp);
};

#endif

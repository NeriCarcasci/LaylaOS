#ifndef __KEYBOARD_BUFFER_H
#define __KEYBOARD_BUFFER_H

#include "types.h"

class KeyboardBuffer {
public:
    static void Init();
    static void Enqueue(char c);
    static bool Dequeue(char* out);
    static bool IsEmpty();
    static void WakeWaiters();

private:
    static const int BUFFER_SIZE = 256;
    static char      buffer[BUFFER_SIZE];
    static int       head;
    static int       tail;
};

#endif

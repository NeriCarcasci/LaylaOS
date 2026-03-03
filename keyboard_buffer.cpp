#include "keyboard_buffer.h"
#include "scheduler.h"
#include "process.h"

char KeyboardBuffer::buffer[BUFFER_SIZE];
int  KeyboardBuffer::head = 0;
int  KeyboardBuffer::tail = 0;

void KeyboardBuffer::Init() {
    head = 0;
    tail = 0;
}

void KeyboardBuffer::Enqueue(char c) {
    int next = (tail + 1) % BUFFER_SIZE;
    if (next == head) return;
    buffer[tail] = c;
    tail = next;
}

bool KeyboardBuffer::Dequeue(char* out) {
    if (head == tail) return false;
    *out = buffer[head];
    head = (head + 1) % BUFFER_SIZE;
    return true;
}

bool KeyboardBuffer::IsEmpty() {
    return head == tail;
}

void KeyboardBuffer::WakeWaiters() {
    Scheduler* s = Scheduler::GetInstance();
    if (s) s->WakeParent(-2);
}

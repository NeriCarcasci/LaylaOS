#include "keyboard.h"
#include "vga.h"
#include "gui.h"
#include "keyboard_buffer.h"

static const char normal_table[88] = {
    0,    0,    '1',  '2',  '3',  '4',  '5',  '6',
    '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
    'o',  'p',  '[',  ']',  '\n', 0,    'a',  's',
    'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',
    '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',
    'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',
    0,    ' ',  0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0
};

static const char shifted_table[88] = {
    0,    0,    '!',  '@',  '#',  '$',  '%',  '^',
    '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
    'O',  'P',  '{',  '}',  '\n', 0,    'A',  'S',
    'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',
    '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',
    'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',
    0,    ' ',  0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0
};

KeyboardDriver::KeyboardDriver(InterruptManager* manager)
    : InterruptHandler(manager->HardwareInterruptOffset() + 0x01, manager),
      data_port(0x60),
      shift(false),
      ctrl(false),
      caps_lock(false)
{
}

uint32_t KeyboardDriver::HandleInterrupt(uint32_t esp) {
    uint8_t scancode = data_port.Read();

    if (scancode == 0x9D) {
        ctrl = false;
        return esp;
    }
    if (scancode == 0xAA || scancode == 0xB6) {
        shift = false;
        return esp;
    }
    if (scancode == 0x1D) {
        ctrl = true;
        return esp;
    }
    if (scancode == 0x2A || scancode == 0x36) {
        shift = true;
        return esp;
    }
    if (scancode == 0x3A) {
        caps_lock = !caps_lock;
        return esp;
    }
    if (scancode & 0x80)
        return esp;

    bool upper = shift ^ caps_lock;
    const char* table = upper ? shifted_table : normal_table;

    if (scancode >= 88)
        return esp;

    char c = table[scancode];
    if (c == 0)
        return esp;

    if (ctrl && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
        c = (char)(c & 0x1F);

    if (VGA::IsGraphicsMode()) {
        Desktop* desktop = GetActiveDesktop();
        if (desktop != nullptr) {
            desktop->OnKeyPress(c);
            return esp;
        }
    }

    KeyboardBuffer::Enqueue(c);
    KeyboardBuffer::WakeWaiters();

    if (!VGA::IsGraphicsMode())
        VGA::PutChar(c);

    return esp;
}

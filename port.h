#ifndef __PORT_H
#define __PORT_H

#include "types.h"

class Port8Bit {
protected:
    uint16_t port;
public:
    Port8Bit(uint16_t port) : port(port) {}

    uint8_t Read() const {
        uint8_t result;
        __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
        return result;
    }

    void Write(uint8_t value) {
        __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
    }
};

class Port8BitSlow : public Port8Bit {
public:
    Port8BitSlow(uint16_t port) : Port8Bit(port) {}

    void Write(uint8_t value) {
        Port8Bit::Write(value);
        __asm__ volatile("outb %0, $0x80" : : "a"((uint8_t)0));
    }
};

class Port16Bit {
protected:
    uint16_t port;
public:
    Port16Bit(uint16_t port) : port(port) {}

    uint16_t Read() const {
        uint16_t result;
        __asm__ volatile("inw %1, %0" : "=a"(result) : "Nd"(port));
        return result;
    }

    void Write(uint16_t value) {
        __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
    }
};

class Port32Bit {
protected:
    uint16_t port;
public:
    Port32Bit(uint16_t port) : port(port) {}

    uint32_t Read() const {
        uint32_t result;
        __asm__ volatile("inl %1, %0" : "=a"(result) : "Nd"(port));
        return result;
    }

    void Write(uint32_t value) {
        __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
    }
};

#endif

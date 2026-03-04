#ifndef __VGA_H
#define __VGA_H

#include "types.h"
#include "port.h"

class VGA {
public:
    static void     Clear();
    static void     PutChar(char c);
    static void     Print(const char* str);
    static void     PrintHex(uint8_t val);
    static void     PrintHex16(uint16_t val);
    static void     PrintHex32(uint32_t val);
    static void     SetColor(uint8_t fg, uint8_t bg);
    static void     SetCursor(uint16_t pos);
    static uint16_t GetCursor();

    static void EnterGraphicsMode();
    static void ExitGraphicsMode();
    static void DrawPixel(uint16_t x, uint16_t y, uint8_t color);
    static void DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color);
    static void DrawChar(uint16_t x, uint16_t y, char c, uint8_t fg, uint8_t bg);
    static bool IsGraphicsMode();

private:
    static uint16_t* const buffer;
    static uint16_t        cursor;
    static uint8_t         color;
    static bool            graphics_mode;

    static const uint8_t font[128][8];
};

#endif

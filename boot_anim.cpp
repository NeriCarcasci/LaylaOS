#include "boot_anim.h"
#include "vga.h"
#include "port.h"
#include "types.h"

static const int8_t sin_table[64] = {
      0,  12,  25,  37,  49,  60,  71,  81,
     90,  98, 106, 112, 117, 122, 125, 127,
    127, 127, 125, 122, 117, 112, 106,  98,
     90,  81,  71,  60,  49,  37,  25,  12,
      0, -12, -25, -37, -49, -60, -71, -81,
    -90, -98,-106,-112,-117,-122,-125,-127,
   -127,-127,-125,-122,-117,-112,-106, -98,
    -90, -81, -71, -60, -49, -37, -25, -12
};

static int sin64(int i) { return sin_table[((i % 64) + 64) % 64]; }
static int cos64(int i) { return sin64(i + 16); }

static uint32_t isqrt(uint32_t n) {
    if (n == 0) return 0;
    uint32_t x = n, y = (x + 1) >> 1;
    while (y < x) { x = y; y = (x + n / x) >> 1; }
    return x;
}

static void set_palette(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    Port8Bit addr(0x3C8), data(0x3C9);
    addr.Write(idx);
    data.Write(r >> 2);
    data.Write(g >> 2);
    data.Write(b >> 2);
}

static void put_px(uint8_t* scr, int x, int y, uint8_t col) {
    if ((uint16_t)x < 320 && (uint16_t)y < 200)
        scr[(uint32_t)y * 320 + x] = col;
}

void BootAnimation() {
    const uint8_t BG     = 240;
    const uint8_t GOLD_D = 241;
    const uint8_t GOLD_M = 242;
    const uint8_t GOLD_B = 243;
    const uint8_t GOLD_H = 244;

    set_palette(BG,     10,  10,  20);
    set_palette(GOLD_D, 100,  50,   0);
    set_palette(GOLD_M, 180, 110,  10);
    set_palette(GOLD_B, 230, 175,  30);
    set_palette(GOLD_H, 255, 240, 160);

    uint8_t* scr = (uint8_t*)0xA0000;
    for (uint32_t i = 0; i < 320 * 200; i++) scr[i] = BG;

    const char* title    = "FRODO OS";
    const char* subtitle = "The One Ring";
    const char* tagline  = "Loading...";

    uint16_t tx  = (320 - 8  * 8) / 2;
    uint16_t stx = (320 - 12 * 8) / 2;
    uint16_t ltx = (320 - 10 * 8) / 2;

    for (int i = 0; title[i];    i++) VGA::DrawChar(tx  + i * 8,  10, title[i],    GOLD_H, BG);
    for (int i = 0; title[i];    i++) VGA::DrawChar(tx  + i * 8,  11, title[i],    GOLD_B, BG);
    for (int i = 0; subtitle[i]; i++) VGA::DrawChar(stx + i * 8,  22, subtitle[i], GOLD_M, BG);
    for (int i = 0; tagline[i];  i++) VGA::DrawChar(ltx + i * 8, 188, tagline[i],  GOLD_D, BG);

    const int cx = 160, cy = 110;
    const int Rout = 44, Rin = 27;

    for (int frame = 0; frame < 192; frame++) {
        int ai   = frame & 63;
        int W    = cos64(ai);
        if (W < 0) W = -W;

        for (int dy = -(Rout + 2); dy <= Rout + 2; dy++) {
            int py = cy + dy;
            for (int px = cx - Rout - 2; px <= cx + Rout + 2; px++)
                put_px(scr, px, py, BG);
        }

        for (int dy = -Rout; dy <= Rout; dy++) {
            int py = cy + dy;

            int32_t disc_o = (int32_t)Rout * Rout - (int32_t)dy * dy;
            if (disc_o < 0) continue;
            int xo = (int)(W * (int)isqrt((uint32_t)disc_o) / 127);

            int xi = 0;
            int32_t disc_i = (int32_t)Rin * Rin - (int32_t)dy * dy;
            if (disc_i >= 0)
                xi = (int)(W * (int)isqrt((uint32_t)disc_i) / 127);
            if (xi > xo) xi = xo;

            uint8_t col;
            if      (dy < -Rout * 2 / 3) col = GOLD_H;
            else if (dy < -Rout / 3)     col = GOLD_B;
            else if (dy <  Rout / 3)     col = GOLD_M;
            else                         col = GOLD_D;

            for (int px = cx - xo; px <= cx - xi - 1; px++) put_px(scr, px, py, col);
            for (int px = cx + xi + 1; px <= cx + xo; px++) put_px(scr, px, py, col);
            if (xo > 0 && xi == 0) put_px(scr, cx, py, col);
        }

        for (volatile uint32_t d = 0; d < 3000000; d++);
    }

    for (uint32_t i = 0; i < 320 * 200; i++) scr[i] = 0;
}

#ifndef __GUI_H
#define __GUI_H

#include "types.h"

namespace Color {
    const uint8_t Black        = 0x00;
    const uint8_t Blue         = 0x01;
    const uint8_t Green        = 0x02;
    const uint8_t Cyan         = 0x03;
    const uint8_t Red          = 0x04;
    const uint8_t Magenta      = 0x05;
    const uint8_t Brown        = 0x06;
    const uint8_t LightGray    = 0x07;
    const uint8_t DarkGray     = 0x08;
    const uint8_t LightBlue    = 0x09;
    const uint8_t LightGreen   = 0x0A;
    const uint8_t LightCyan    = 0x0B;
    const uint8_t LightRed     = 0x0C;
    const uint8_t LightMagenta = 0x0D;
    const uint8_t Yellow       = 0x0E;
    const uint8_t White        = 0x0F;
}

class Desktop;

class Widget {
public:
    Widget(int32_t x, int32_t y, int32_t w, int32_t h);
    virtual ~Widget();
    virtual void Draw();
    virtual void OnMouseDown(int32_t x, int32_t y, uint8_t button);
    virtual void OnMouseUp(int32_t x, int32_t y, uint8_t button);
    virtual void OnKeyPress(char c);
    bool ContainsPoint(int32_t x, int32_t y);
protected:
    int32_t x, y, w, h;
};

class Window : public Widget {
public:
    Window(int32_t x, int32_t y, int32_t w, int32_t h,
           const char* title, uint8_t color);
    void Draw() override;
    void AddWidget(Widget*);
    void OnMouseDown(int32_t x, int32_t y, uint8_t button) override;
    void OnKeyPress(char c) override;
private:
    static const int MAX_WIDGETS = 32;
    Widget*     widgets[MAX_WIDGETS];
    int         widget_count;
    const char* title;
    uint8_t     color;
};

class Desktop : public Widget {
public:
    Desktop(int32_t w, int32_t h, uint8_t color);
    void Draw() override;
    void AddWindow(Window*);
    void OnMouseMove(int32_t x, int32_t y, uint8_t buttons);
    void OnMouseDown(int32_t x, int32_t y, uint8_t button) override;
    void OnMouseUp(int32_t x, int32_t y, uint8_t button) override;
    void OnKeyPress(char c) override;
private:
    static const int MAX_WINDOWS = 16;
    Window*  windows[MAX_WINDOWS];
    int      window_count;
    uint8_t  color;
    int32_t  mouse_x, mouse_y;
    uint8_t  mouse_buttons;
};

void    SetActiveDesktop(Desktop* d);
Desktop* GetActiveDesktop();

#endif

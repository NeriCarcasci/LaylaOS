#include "gui.h"
#include "vga.h"

static Desktop* active_desktop = nullptr;

void SetActiveDesktop(Desktop* d) { active_desktop = d; }
Desktop* GetActiveDesktop()       { return active_desktop; }

Widget::Widget(int32_t x, int32_t y, int32_t w, int32_t h)
    : x(x), y(y), w(w), h(h) {}
Widget::~Widget() {}
void Widget::Draw() {}
void Widget::OnMouseDown(int32_t, int32_t, uint8_t) {}
void Widget::OnMouseUp(int32_t, int32_t, uint8_t) {}
void Widget::OnKeyPress(char) {}

bool Widget::ContainsPoint(int32_t px, int32_t py) {
    return px >= x && px < x + w && py >= y && py < y + h;
}

Window::Window(int32_t x, int32_t y, int32_t w, int32_t h,
               const char* title, uint8_t color)
    : Widget(x, y, w, h), title(title), color(color), widget_count(0)
{
    for (int i = 0; i < MAX_WIDGETS; i++)
        widgets[i] = nullptr;
}

void Window::Draw() {
    VGA::DrawRect((uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)h, color);
    VGA::DrawRect((uint16_t)x, (uint16_t)y, (uint16_t)w, 10, Color::DarkGray);

    uint16_t tx = (uint16_t)(x + 4);
    uint16_t ty = (uint16_t)(y + 1);
    for (int i = 0; title[i] != '\0'; i++) {
        VGA::DrawChar(tx, ty, title[i], Color::White, Color::DarkGray);
        tx += 8;
    }

    for (int i = 0; i < widget_count; i++)
        widgets[i]->Draw();
}

void Window::AddWidget(Widget* w) {
    if (widget_count < MAX_WIDGETS)
        widgets[widget_count++] = w;
}

void Window::OnMouseDown(int32_t px, int32_t py, uint8_t button) {
    for (int i = 0; i < widget_count; i++)
        if (widgets[i]->ContainsPoint(px, py))
            widgets[i]->OnMouseDown(px, py, button);
}

void Window::OnKeyPress(char c) {
    if (widget_count > 0)
        widgets[widget_count - 1]->OnKeyPress(c);
}

Desktop::Desktop(int32_t w, int32_t h, uint8_t color)
    : Widget(0, 0, w, h), color(color),
      window_count(0), mouse_x(w / 2), mouse_y(h / 2), mouse_buttons(0)
{
    for (int i = 0; i < MAX_WINDOWS; i++)
        windows[i] = nullptr;
}

void Desktop::Draw() {
    VGA::DrawRect(0, 0, (uint16_t)w, (uint16_t)h, color);
    for (int i = 0; i < window_count; i++)
        windows[i]->Draw();
    VGA::DrawPixel((uint16_t)mouse_x, (uint16_t)mouse_y, Color::White);
}

void Desktop::AddWindow(Window* win) {
    if (window_count < MAX_WINDOWS)
        windows[window_count++] = win;
}

void Desktop::OnMouseMove(int32_t dx, int32_t dy, uint8_t buttons) {
    VGA::DrawPixel((uint16_t)mouse_x, (uint16_t)mouse_y, color);

    mouse_x += dx;
    mouse_y -= dy;
    mouse_buttons = buttons;

    if (mouse_x < 0)    mouse_x = 0;
    if (mouse_x >= w)   mouse_x = w - 1;
    if (mouse_y < 0)    mouse_y = 0;
    if (mouse_y >= h)   mouse_y = h - 1;

    VGA::DrawPixel((uint16_t)mouse_x, (uint16_t)mouse_y, Color::White);
}

void Desktop::OnMouseDown(int32_t px, int32_t py, uint8_t button) {
    for (int i = window_count - 1; i >= 0; i--)
        if (windows[i]->ContainsPoint(px, py)) {
            windows[i]->OnMouseDown(px, py, button);
            return;
        }
}

void Desktop::OnMouseUp(int32_t px, int32_t py, uint8_t button) {
    for (int i = window_count - 1; i >= 0; i--)
        if (windows[i]->ContainsPoint(px, py)) {
            windows[i]->OnMouseUp(px, py, button);
            return;
        }
}

void Desktop::OnKeyPress(char c) {
    if (window_count > 0)
        windows[window_count - 1]->OnKeyPress(c);
}

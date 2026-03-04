#include "terminal.h"
#include "vga.h"
#include "keyboard_buffer.h"

Terminal* Terminal::active_terminal = nullptr;

Terminal::Terminal(int32_t x, int32_t y, int32_t w, int32_t h)
    : Window(x, y, w, h, "", COL_BG),
      cursor_col(0), cursor_row(0), input_len(0)
{
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++) {
            grid[r][c]   = ' ';
            colors[r][c] = COL_TEXT;
        }
    for (int i = 0; i < 256; i++) input_line[i] = 0;
}

Terminal* Terminal::GetActive()          { return active_terminal; }
void      Terminal::SetActive(Terminal* t) { active_terminal = t; }

void Terminal::ScrollUp() {
    for (int r = 0; r < ROWS - 1; r++)
        for (int c = 0; c < COLS; c++) {
            grid[r][c]   = grid[r + 1][c];
            colors[r][c] = colors[r + 1][c];
        }
    for (int c = 0; c < COLS; c++) {
        grid[ROWS - 1][c]   = ' ';
        colors[ROWS - 1][c] = COL_TEXT;
    }
    if (cursor_row > 0) cursor_row--;
}

void Terminal::NewLine() {
    cursor_col = 0;
    cursor_row++;
    if (cursor_row >= ROWS)
        ScrollUp();
}

void Terminal::Backspace() {
    if (cursor_col > 0) {
        cursor_col--;
    } else if (cursor_row > 0) {
        cursor_row--;
        cursor_col = COLS - 1;
    }
    grid[cursor_row][cursor_col]   = ' ';
    colors[cursor_row][cursor_col] = COL_TEXT;
}

void Terminal::PutChar(char c) {
    if (c == '\n') {
        NewLine();
    } else if (c == '\b') {
        Backspace();
    } else {
        grid[cursor_row][cursor_col]   = c;
        colors[cursor_row][cursor_col] = COL_TEXT;
        cursor_col++;
        if (cursor_col >= COLS)
            NewLine();
    }
}

void Terminal::Print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++)
        PutChar(str[i]);
    Draw();
}

void Terminal::SetShellOutput(const char* str, uint32_t len) {
    for (uint32_t i = 0; i < len; i++)
        PutChar(str[i]);
    Draw();
}

void Terminal::DrawHeader() {
    VGA::DrawRect((uint16_t)x, (uint16_t)y, (uint16_t)w, HEADER_H, COL_HDR_BG);

    const char* title = "FRODO OS";
    int tx = x + 2;
    for (int i = 0; title[i]; i++)
        VGA::DrawChar((uint16_t)(tx + i * 6), (uint16_t)(y + 1), title[i], COL_GOLD, COL_HDR_BG);

    const char* user = "root@ring";
    int ux = x + w - 9 * 6 - 2;
    for (int i = 0; user[i]; i++)
        VGA::DrawChar((uint16_t)(ux + i * 6), (uint16_t)(y + 1), user[i], COL_DIM, COL_HDR_BG);

    VGA::DrawRect((uint16_t)x, (uint16_t)(y + HEADER_H - 1), (uint16_t)w, 1, COL_GOLD);
}

void Terminal::Draw() {
    DrawHeader();

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            VGA::DrawChar(
                (uint16_t)(x + c * 8),
                (uint16_t)(y + HEADER_H + r * 8),
                grid[r][c],
                colors[r][c],
                COL_BG
            );
        }
    }

    VGA::DrawRect(
        (uint16_t)(x + cursor_col * 8),
        (uint16_t)(y + HEADER_H + cursor_row * 8),
        8, 8,
        COL_TEXT
    );
}

void Terminal::OnKeyPress(char c) {
    if (c == '\b') {
        if (input_len > 0) {
            input_len--;
            Backspace();
            Draw();
        }
    } else if (c == '\n') {
        input_line[input_len++] = '\n';
        input_line[input_len]   = '\0';
        PutChar('\n');
        for (int i = 0; i < input_len; i++)
            KeyboardBuffer::Enqueue(input_line[i]);
        KeyboardBuffer::WakeWaiters();
        input_len = 0;
        Draw();
    } else if (c >= 32 && input_len < 255) {
        input_line[input_len++] = c;
        PutChar(c);
        Draw();
    }
}

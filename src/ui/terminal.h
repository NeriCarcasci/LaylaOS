#ifndef __TERMINAL_H
#define __TERMINAL_H

#include "types.h"
#include "gui.h"

class Terminal : public Window {
public:
    Terminal(int32_t x, int32_t y, int32_t w, int32_t h);
    void Draw() override;
    void OnKeyPress(char c) override;
    void Print(const char* str);
    void PutChar(char c);
    void SetShellOutput(const char* str, uint32_t len);
    void SetRawInputMode(bool enabled);
    bool IsRawInputMode() const;

    static Terminal* GetActive();
    static void      SetActive(Terminal* t);

    static const uint8_t COL_BG     = 0;
    static const uint8_t COL_TEXT   = 220;
    static const uint8_t COL_GOLD   = 221;
    static const uint8_t COL_DIM    = 222;
    static const uint8_t COL_HDR_BG = 223;

private:
    static const int COLS     = 40;
    static const int ROWS     = 23;
    static const int HEADER_H = 10;

    char    grid[ROWS][COLS];
    uint8_t colors[ROWS][COLS];
    int     cursor_col;
    int     cursor_row;
    char    input_line[256];
    int     input_len;
    bool    raw_input_mode;

    static Terminal* active_terminal;

    void ScrollUp();
    void NewLine();
    void Backspace();
    void DrawHeader();
};

#endif

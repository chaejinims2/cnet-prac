#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
    constexpr const char* kAnsiHighlight = "\033[1;33m";
    constexpr const char* kAnsiReset = "\033[0m";
    
    void enable_ansi_stdout() {
    #ifdef _WIN32
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        if (out == INVALID_HANDLE_VALUE) {
            return;
        }
        DWORD mode = 0;
        if (!GetConsoleMode(out, &mode)) {
            return;
        }
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    #endif
    }
    
}  // namespace debug

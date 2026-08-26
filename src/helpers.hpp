#pragma once
#include <string>
#include <unistd.h>

inline void emit_terminal_command(const std::string &s) {
    ssize_t ignored = ::write(STDOUT_FILENO, s.data(), s.size());
    (void)ignored;
}

inline std::string at(int row, int col) {
    return "\x1b[" + std::to_string(row) + ";" + std::to_string(col) + "H";
}

inline void clear_screen() {
    emit_terminal_command("\x1b[2J");
}
#pragma once
#include <string>
#include <unistd.h>

static void emit_terminal_command(const std::string &s) {
    ssize_t ignored = ::write(STDOUT_FILENO, s.data(), s.size());
    (void)ignored;
}

static std::string at(int row, int col) {
    return "\x1b[" + std::to_string(row) + ";" + std::to_string(col) + "H";
}

static void clear_screen() {
    emit_terminal_command("\x1b[2J");
}
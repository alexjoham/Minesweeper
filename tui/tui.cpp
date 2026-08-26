#include "tui.hpp"

int Tui::drawGame(int x, int y, const Game::Board& board) {
    const int size = static_cast<int>(board.size());
    emit_terminal_command(at(x, y) + "\u250C");
    for (int i = 1; i < 3*size; i+=3) {
        emit_terminal_command(at(x, y+i) + "\u2500");
        emit_terminal_command(at(x, y+i+1) + "\u2500");
        emit_terminal_command(at(x, y+i+2) + "\u2500");
    }
    emit_terminal_command(at(x, y+3*size+1) + "\u2510");
    for (int i = 1; i <= size; i++) {
        emit_terminal_command(at(x+i, y) + "\u2502");
        for (int j = 0; j < size; j++) {
            Playerield field = board[static_cast<size_t>(i-1)][static_cast<size_t>(j)];
            std::string_view colour = Tui::colour(field);
            std::string_view glyph = Tui::glyph(field);
            std::string face = "";
            face += colour;
            face += std::string(1, ' ');
            face += glyph;
            face += std::string(1, ' ');
            face += "\x1b[0m";
            emit_terminal_command(at(x+i, y+j*3 + 1) + face);
        }
        emit_terminal_command(at(x+i, y+3*size+1) + "\u2502");
    }
    emit_terminal_command(at(x+size+1, y) + "\u2514");
    for (int i = 1; i < 3*size; i+=3) {
        emit_terminal_command(at(x+size+1, y+i) + "\u2500");
        emit_terminal_command(at(x+size+1, y+i+1) + "\u2500");
        emit_terminal_command(at(x+size+1, y+i+2) + "\u2500");
    }
    emit_terminal_command(at(x+size+1, y+3*size+1) + "\u2518");
    return size+2;
}
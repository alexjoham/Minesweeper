#include "tui.hpp"

int Tui::drawGame(const Game::Board& board) {
    const int size = static_cast<int>(board.size());
    emit_terminal_command(at(origin_row_, origin_col_) + "\u250C");
    for (int i = 1; i < kCellWidth*size; i+=kCellWidth) {
        emit_terminal_command(at(origin_row_, origin_col_+i) + "\u2500");
        emit_terminal_command(at(origin_row_, origin_col_+i+1) + "\u2500");
        emit_terminal_command(at(origin_row_, origin_col_+i+2) + "\u2500");
    }
    emit_terminal_command(at(origin_row_, origin_col_+kCellWidth*size+1) + "\u2510");
    for (int i = 1; i <= size; i++) {
        emit_terminal_command(at(origin_row_+i, origin_col_) + "\u2502");
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
            emit_terminal_command(at(origin_row_+i, origin_col_+j*kCellWidth + 1) + face);
        }
        emit_terminal_command(at(origin_row_+i, origin_col_+kCellWidth*size+1) + "\u2502");
    }
    emit_terminal_command(at(origin_row_+size+1, origin_col_) + "\u2514");
    for (int i = 1; i < kCellWidth*size; i+=kCellWidth) {
        emit_terminal_command(at(origin_row_+size+1, origin_col_+i) + "\u2500");
        emit_terminal_command(at(origin_row_+size+1, origin_col_+i+1) + "\u2500");
        emit_terminal_command(at(origin_row_+size+1, origin_col_+i+2) + "\u2500");
    }
    emit_terminal_command(at(origin_row_+size+1, origin_col_+kCellWidth*size+1) + "\u2518");
    return size+2;
}
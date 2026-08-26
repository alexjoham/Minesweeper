#pragma once
#include <memory>
#include <vector>
#include <optional>
#include <string_view>
#include "../structs/board_coord.hpp"
#include "../game/game.hpp"
#include "../helpers.hpp"

class Tui {
    private:
        static constexpr int kCellWidth = 3;
        int origin_row_;
        int origin_col_;

    public:
        Tui(int origin_row, int origin_col) noexcept
            : origin_row_(origin_row), origin_col_(origin_col) {}
        /**
         * @return height of the field
         */
        int drawGame(const Game::Board& board);

        static std::string_view glyph(const Playerield& c) {
            if (c.hidden && c.flagged) return "\u2691";
            switch (c.fieldType) {
                case FieldType::HIDDEN:     return "\u25A2";
                case FieldType::MINE:       return "★";
                case FieldType::NEUTRAL:    return " ";
                case FieldType::ONE:        return "1";
                case FieldType::TWO:        return "2";
                case FieldType::THREE:      return "3";
                case FieldType::FOUR:       return "4";
                case FieldType::FIVE:       return "5";
                case FieldType::SIX:        return "6";
                case FieldType::SEVEN:      return "7";
                case FieldType::EIGHT:      return "8";
                default: return "";
            }
        }
        
        static std::string_view colour(const Playerield& c) {
            if (c.hidden && c.flagged) return "\x1b[93m";
            switch (c.fieldType) {
                case FieldType::HIDDEN:     return "\x1b[97m";
                case FieldType::MINE:       return "\x1b[91m";
                case FieldType::NEUTRAL:    return "\x1b[97m";
                case FieldType::ONE:        return "\x1b[94m";
                case FieldType::TWO:        return "\x1b[92m";
                case FieldType::THREE:      return "\x1b[91m";
                case FieldType::FOUR:       return "\x1b[95m";
                case FieldType::FIVE:       return "\x1b[93m";
                case FieldType::SIX:        return "\x1b[96m";
                case FieldType::SEVEN:      return "\x1b[90m";
                case FieldType::EIGHT:      return "\x1b[37m";
                default: return "";
            }
        }

        std::optional<BoardCoord> boardCellAt(int screen_row, int screen_col) const {
            const int r = screen_row - origin_row_ - 1;
            const int c = screen_col - origin_col_ - 1;
            if (r < 0 || c < 0) return std::nullopt;

            size_t row = static_cast<size_t>(r);
            size_t column = static_cast<size_t>(c / kCellWidth);
            if (row >= Game::kFieldSize || column >= Game::kFieldSize) return std::nullopt;
            return BoardCoord{row, column};
        }
};
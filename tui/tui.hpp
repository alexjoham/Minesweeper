#pragma once
#include <memory>
#include <vector>
#include "../game/game.hpp"
#include "../helpers.hpp"

class Tui {
    public:
        /**
         * @return height of the field
         */
        int drawGame(int x, int y, const Game::Board& board);

        static std::string_view glyph(const Playerield& c) {
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
};
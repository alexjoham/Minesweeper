#pragma once
#include <vector>
#include <array>
#include <optional>
#include <cstdlib> 
#include <ctime>
#include <algorithm>
#include "../structs/board_coord.hpp"
#include "../enums/field_type.hpp"

struct Playerield
{
    bool hidden = true;
    bool flagged = false;
    FieldType fieldType = FieldType::HIDDEN;
};

struct RevealedCell { 
    BoardCoord coordinates;
    Playerield cell;
};


class Game {
    public:
        static constexpr size_t kFieldSize = 9;
        const int NUM_MINES = 10;
        using Board = std::array<std::array<Playerield, kFieldSize>, kFieldSize>;

        using MineLayout = std::array<std::array<bool, kFieldSize>, kFieldSize>;

        void startGame();

        std::vector<RevealedCell> revealAll();

        std::vector<RevealedCell> makeMove(size_t row, size_t column);

        const Board& getPlayerfield() const {
            return playerfield_;
        }

        std::optional<Playerield> toggleFlag(size_t row, size_t column) {
        if (row >= kFieldSize || column >= kFieldSize) {
            return std::nullopt;
        }
            if (playerfield_[row][column].hidden) {
                playerfield_[row][column].flagged = !playerfield_[row][column].flagged;
                return playerfield_[row][column];
            }
            return std::nullopt;
        }

        bool game_won();
    private:

        void createNewGame(const MineLayout& mines);

        void updateNearbyMineNumbers();

        static MineLayout randomLayout(int mine_count) {
            MineLayout minefield{};
            static bool seeded = false;
            if (!seeded) { srand((unsigned)time(0)); seeded = true; }

            std::vector<int> mines;

            // Fill the filed with mines
            for (int i = 0; i < mine_count; i++) {
                const size_t j = (static_cast<size_t>(rand())%(kFieldSize*kFieldSize));
                bool in_array = std::find(mines.begin(), mines.end(), j) != mines.end();
                if (in_array) {
                    i -= 1;
                    continue;
                } else {
                    mines.emplace_back(j);
                    size_t row = j / kFieldSize;
                    size_t column = j % kFieldSize;
                    minefield[row][column] = true;
                }
            }
            return minefield;
        }

        void updateFieldsAroundMine(size_t row, size_t column);

        std::vector<RevealedCell> revealFieldsAroundMove(size_t row, size_t column);

        /**
         * minefield state
         * -1: mine
         * 0: none
         * 1: one mine nearby, for all other numbers the same
         */
        std::array<std::array<int, kFieldSize>, kFieldSize> minefield_{};
        Board playerfield_{};

        FieldType getFieldType(size_t row, size_t column) const {
            switch (minefield_[row][column])
            {
            case -1:
                return FieldType::MINE;
                break;
            case 0:
                return FieldType::NEUTRAL;
                break;
            case 1:
                return FieldType::ONE;
                break;
            case 2:
                return FieldType::TWO;
                break;
            case 3:
                return FieldType::THREE;
                break;
            case 4:
                return FieldType::FOUR;
                break;
            case 5:
                return FieldType::FIVE;
                break;
            case 6:
                return FieldType::SIX;
                break;
            case 7:
                return FieldType::SEVEN;
                break;
            case 8:
                return FieldType::EIGHT;
                break;
            default:
                return FieldType::NEUTRAL;
                break;
            }
        }
};
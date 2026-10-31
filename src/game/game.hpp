#pragma once
#include "../enums/field_type.hpp"
#include "../enums/game_outcome.hpp"
#include "../structs/board_coord.hpp"
#include "random_layout.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

struct Playerield {
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
    static constexpr int kNumMines = 10;

    using Board = std::array<std::array<Playerield, kFieldSize>, kFieldSize>;

    using MineLayout = std::array<std::array<bool, kFieldSize>, kFieldSize>;

    void restart();

    std::vector<RevealedCell> revealAll();

    // Once the outcome is decided the board is frozen: makeMove reveals
    // nothing and toggleFlag returns nullopt. revealAll() still works.
    std::vector<RevealedCell> makeMove(size_t row, size_t column);

    const Board& getPlayerfield() const { return playerfield_; }

    std::optional<Playerield> toggleFlag(size_t row, size_t column) {
        if (outcome_ != GameOutcome::Playing || row >= kFieldSize || column >= kFieldSize) {
            return std::nullopt;
        }
        if (playerfield_[row][column].hidden) {
            playerfield_[row][column].flagged = !playerfield_[row][column].flagged;
            return playerfield_[row][column];
        }
        return std::nullopt;
    }

    // Decided inside makeMove: Lost the moment a mine is revealed, Won once
    // every cell that is not a mine is revealed. revealAll() never changes it.
    GameOutcome outcome() const noexcept { return outcome_; }

    bool game_won() const noexcept { return outcome_ == GameOutcome::Won; }

    explicit Game(const MineLayout& mines);

    Game() : Game(randomLayout<kFieldSize>(kNumMines, defaultRng())) {}

private:
    void createNewGame(const MineLayout& mines);

    void updateNearbyMineNumbers();

    bool allSafeCellsRevealed() const;

    void updateOutcome(bool hit_mine);

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
    GameOutcome outcome_ = GameOutcome::Playing;

    FieldType getFieldType(size_t row, size_t column) const {
        switch (minefield_[row][column]) {
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

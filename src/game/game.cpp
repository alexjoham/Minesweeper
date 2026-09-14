#include "game.hpp"
#include "random_layout.hpp"
#include "../enums/field_type.hpp"
#include "../structs/board_coord.hpp"

#include <array>
#include <cstddef>
#include <queue>
#include <vector>

void Game::updateNearbyMineNumbers() {
    for (size_t i = 0; i < kFieldSize; i++) {
        for(size_t j = 0; j < kFieldSize; j++) {
            if (minefield_[i][j] == -1) {
                updateFieldsAroundMine(i, j);
            }
        }
    }
}

void Game::updateFieldsAroundMine(size_t row, size_t column) {
    size_t r_start = row > 0 ? row - 1 : 0;
    size_t r_end = row + 1 >= kFieldSize ? kFieldSize - 1 : row + 1;
    size_t c_start = column > 0 ? column - 1 : 0;
    size_t c_end = column + 1 >= kFieldSize ? kFieldSize - 1 : column + 1;

    for(size_t i = r_start; i <= r_end; i++) {
        for(size_t j = c_start; j <= c_end; j++) {
            if (row == i && column == j) continue; // mine that we update around
            if (minefield_[i][j] == -1) continue; // mine itself, do not update!
            minefield_[i][j] += 1;
        }
    }
}

void Game::createNewGame(const MineLayout& mines) {
    for (size_t r = 0; r < kFieldSize; ++r) {
        for (size_t c = 0; c < kFieldSize; ++c) {
            minefield_[r][c] = mines[r][c] ? -1 : 0;
        }
    }
    playerfield_ = Board{};
    updateNearbyMineNumbers();
}

void Game::restart() {
    createNewGame(randomLayout<kFieldSize>(kNumMines, defaultRng()));
}

std::vector<RevealedCell> Game::makeMove(size_t row, size_t column) {
    std::vector<RevealedCell> revealedButtons;
    if (row >= kFieldSize || column >= kFieldSize) {
        return revealedButtons;
    }
    if(playerfield_[row][column].hidden && !playerfield_[row][column].flagged) {
        FieldType fieldType = getFieldType(row, column);
        playerfield_[row][column].fieldType = fieldType;
        if (fieldType != FieldType::NEUTRAL) {
            playerfield_[row][column].hidden = false;
            revealedButtons.push_back(RevealedCell{BoardCoord{row, column}, playerfield_[row][column]});
            return revealedButtons;
        }
        std::vector<RevealedCell> vector = revealFieldsAroundMove(row, column);
        return vector;
    }
    return revealedButtons;
}

/**
 * Reveals the fields around the move, including the move itself. Uses BFS
 * @returns revealed cells
 */
std::vector<RevealedCell> Game::revealFieldsAroundMove(size_t row, size_t column) {
    std::vector<RevealedCell> revealedFields;
    std::queue<BoardCoord> toProcess;
    std::array<std::array<bool, kFieldSize>, kFieldSize> visited{};
    toProcess.push(BoardCoord{row, column});
    visited[row][column] = true;

    while(toProcess.size() > 0) {
        BoardCoord processed = toProcess.front();
        toProcess.pop();
        row = processed.row;
        column = processed.column;
        if (playerfield_[row][column].hidden) {
            FieldType fieldType = getFieldType(row, column);
            if (fieldType == FieldType::MINE) {
                continue;
            }
            playerfield_[row][column].hidden = false;
            playerfield_[row][column].fieldType = fieldType;
            revealedFields.push_back(RevealedCell{BoardCoord{row, column}, playerfield_[row][column]});
            if (fieldType != FieldType::NEUTRAL) {
                continue;
            }
        }
        size_t r_start = row > 0 ? row - 1 : 0;
        size_t r_end = row + 1 >= kFieldSize ? kFieldSize - 1 : row + 1;
        size_t c_start = column > 0 ? column - 1 : 0;
        size_t c_end = column + 1 >= kFieldSize ? kFieldSize - 1 : column + 1;
        for(size_t i = r_start; i <= r_end; i++) {
            for(size_t j = c_start; j <= c_end; j++) {
                if (playerfield_[i][j].hidden && !visited[i][j] && !playerfield_[i][j].flagged) {
                    visited[i][j] = true;
                    toProcess.push(BoardCoord{i, j});
                }
            }
        }
    }

    

    return revealedFields;
}

std::vector<RevealedCell> Game::revealAll() {
    std::vector<RevealedCell> revealedButtons;
    for (size_t i = 0; i < kFieldSize; i++) {
        for(size_t j = 0; j < kFieldSize; j++) {
            if(playerfield_[i][j].hidden) {
                playerfield_[i][j].hidden = false;
                playerfield_[i][j].fieldType = getFieldType(i, j);
                revealedButtons.push_back(RevealedCell{BoardCoord{i, j}, playerfield_[i][j]});
            }
        }
    }
    return revealedButtons;
}

bool Game::game_won() const {
    for (size_t i = 0; i < kFieldSize; i++) {
        for(size_t j = 0; j < kFieldSize; j++) {
            if(playerfield_[i][j].hidden) {
                if(getFieldType(i, j) != FieldType::MINE) return false;
            }
        }
    }
    return true;
}

Game::Game(const MineLayout& mines) {
    createNewGame(mines);
}
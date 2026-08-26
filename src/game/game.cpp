#include "game.hpp"
#include <cstdlib> 
#include <ctime>
#include <algorithm>
#include <vector>
#include "../helpers.hpp"
#include <queue>

void Game::generateRandomField() {
    static bool seeded = false;
    if (!seeded) { srand((unsigned)time(0)); seeded = true; }

    std::vector<int> mines;

    // Fill the filed with mines
    for (int i = 0; i < NUM_MINES; i++) {
        const size_t j = (static_cast<size_t>(rand())%(kFieldSize*kFieldSize));
        bool in_array = std::find(mines.begin(), mines.end(), j) != mines.end();
        if (in_array) {
            i -= 1;
            continue;
        } else {
            mines.emplace_back(j);
            size_t row = j / kFieldSize;
            size_t column = j % kFieldSize;
            minefield_[row][column] = -1;
        }
    }
    
    // Set the number of mines nearby
    for (size_t i = 0; i < kFieldSize; i++) {
        for(size_t j = 0; j < kFieldSize; j++) {
            if (minefield_[i][j] == -1) {
                updateFieldsAroundmine(i, j);
            }
        }
    }
}

void Game::updateFieldsAroundmine(size_t row, size_t column) {
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

void Game::startGame() {
    for(auto &row : minefield_) std::fill(std::begin(row), std::end(row), 0);
    for(auto &row : playerfield_) std::fill(std::begin(row), std::end(row), Playerield{});
    generateRandomField();
}

std::vector<RevealedCell> Game::makeMove(size_t row, size_t column) {
    std::vector<RevealedCell> revealedButtons;
    if (row >= kFieldSize || column >= kFieldSize) {
        return revealedButtons;
    }
    if(playerfield_[row][column].hidden) {
        // So it is ignored in the revealAll method
        playerfield_[row][column].hidden = false;
        playerfield_[row][column].fieldType = getFieldType(row, column);
        if (playerfield_[row][column].fieldType == FieldType::MINE) {
            revealedButtons.push_back(RevealedCell{BoardCoord{row, column}, playerfield_[row][column]});
            playerfield_[row][column].hidden = true; // Will be revealed later
            return revealedButtons;
        }
        std::vector<RevealedCell> vector = revealFieldsAroundMove(row, column);
        return vector;
    } else {
        return revealedButtons;
    }
    return revealedButtons;
}

std::vector<RevealedCell> Game::revealFieldsAroundMove(size_t row, size_t column) {
    std::vector<RevealedCell> revealedFields;
    std::queue<RevealedCell> toProcess;
    toProcess.push(RevealedCell{BoardCoord{row, column}, playerfield_[row][column]});
    revealedFields.push_back(RevealedCell{BoardCoord{row, column}, playerfield_[row][column]});

    while(toProcess.size() > 0) {
        RevealedCell processed = toProcess.front();
        toProcess.pop();
        row = static_cast<size_t>(processed.coordinates.row);
        column = static_cast<size_t>(processed.coordinates.column);
        if (processed.cell.hidden) {
            FieldType fieldType = getFieldType(row, column);
            if (fieldType == FieldType::MINE) {
                continue;
            }
            processed.cell.hidden = false;
            playerfield_[row][column].hidden = false;
            playerfield_[row][column].fieldType = fieldType;
            revealedFields.push_back(RevealedCell{BoardCoord{row, column}, playerfield_[row][column]});
            if (fieldType == FieldType::ONE || fieldType == FieldType::TWO || fieldType == FieldType::THREE || fieldType == FieldType::FOUR || fieldType == FieldType::FIVE) {
                continue;
            }
        }
        size_t r_start = row > 0 ? row - 1 : 0;
        size_t r_end = row + 1 >= kFieldSize ? kFieldSize - 1 : row + 1;
        size_t c_start = column > 0 ? column - 1 : 0;
        size_t c_end = column + 1 >= kFieldSize ? kFieldSize - 1 : column + 1;
        for(size_t i = r_start; i <= r_end; i++) {
            for(size_t j = c_start; j <= c_end; j++) {
                if (playerfield_[i][j].hidden) {
                    toProcess.push(RevealedCell{BoardCoord{i, j}, playerfield_[i][j]});
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

bool Game::game_won() {
    for (size_t i = 0; i < kFieldSize; i++) {
        for(size_t j = 0; j < kFieldSize; j++) {
            if(playerfield_[i][j].hidden) {
                if(getFieldType(i, j) != FieldType::MINE) return false;
            }
        }
    }
    return true;
}
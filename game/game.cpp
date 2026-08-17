#include "game/game.hpp"
#include <cstdlib> 
#include <ctime>
#include <algorithm>
#include <vector>
#include "helpers.hpp"
#include <queue>

void Game::generateRandomField() {
    static bool seeded = false;
    if (!seeded) { srand((unsigned)time(0)); seeded = true; }

    std::vector<int> mines;

    // Fill the filed with mines
    for (int i = 0; i < NUM_MINES; i++) {
        const int j = (rand()%(FIELD_SIZE*FIELD_SIZE));
        bool in_array = std::find(mines.begin(), mines.end(), j) != mines.end();
        if (in_array) {
            i -= 1;
            continue;
        } else {
            mines.emplace_back(j);
            int row = j / FIELD_SIZE;
            int column = j % FIELD_SIZE;
            minefield[row][column] = -1;
        }
    }
    
    // Set the number of mines nearby
    for (int i = 0; i < FIELD_SIZE; i++) {
        for(int j = 0; j < FIELD_SIZE; j++) {
            if (minefield[i][j] == -1) {
                updateFieldsAroundmine(i, j);
            }
        }
    }
}

void Game::updateFieldsAroundmine(int row, int column) {
    int r_start = row - 1 < 0 ? 0 : row - 1;
    int r_end = row + 1 >= FIELD_SIZE ? FIELD_SIZE - 1 : row + 1;
    int c_start = column - 1 < 0 ? 0 : column - 1;
    int c_end = column + 1 >= FIELD_SIZE ? FIELD_SIZE - 1 : column + 1;

    for(int i = r_start; i <= r_end; i++) {
        for(int j = c_start; j <= c_end; j++) {
            if (row == i && column == j) continue; // mine that we update around
            if (minefield[i][j] == -1) continue; // mine itself, do not update!
            minefield[i][j] += 1;
        }
    }
}

void Game::startGame() {
    for(auto &row : minefield) std::fill(std::begin(row), std::end(row), 0);
    for(auto &row : playerfield) std::fill(std::begin(row), std::end(row), Playerield{});
    generateRandomField();
}

int Game::drawGame(int x, int y, std::vector<std::unique_ptr<Button>> &buttons) {
    emit_terminal_command(at(x, y) + "\u250C");
    for (int i = 1; i < 3*FIELD_SIZE; i+=3) {
        emit_terminal_command(at(x, y+i) + "\u2500");
        emit_terminal_command(at(x, y+i+1) + "\u2500");
        emit_terminal_command(at(x, y+i+2) + "\u2500");
    }
    emit_terminal_command(at(x, y+3*FIELD_SIZE+1) + "\u2510");
    for (int i = 1; i <= FIELD_SIZE; i++) {
        emit_terminal_command(at(x+i, y) + "\u2502");
        for (int j = 0; j < FIELD_SIZE; j++) {
            buttons.at((i-1) * FIELD_SIZE + j)->x = x+j*3+1;
            buttons.at((i-1) * FIELD_SIZE + j)->y = y+i;
            buttons.at((i-1) * FIELD_SIZE + j)->w = 3;
            buttons.at((i-1) * FIELD_SIZE + j)->draw();
        }
        emit_terminal_command(at(x+i, y+3*FIELD_SIZE+1) + "\u2502");
    }
    emit_terminal_command(at(x+FIELD_SIZE+1, y) + "\u2514");
    for (int i = 1; i < 3*FIELD_SIZE; i+=3) {
        emit_terminal_command(at(x+FIELD_SIZE+1, y+i) + "\u2500");
        emit_terminal_command(at(x+FIELD_SIZE+1, y+i+1) + "\u2500");
        emit_terminal_command(at(x+FIELD_SIZE+1, y+i+2) + "\u2500");
    }
    emit_terminal_command(at(x+FIELD_SIZE+1, y+3*FIELD_SIZE+1) + "\u2518");
    return FIELD_SIZE+2;
}

std::vector<Playerield> Game::makeMove(int buttonID) {
    std::vector<Playerield> revealedButtons;
    for (int i = 0; i < FIELD_SIZE; i++) {
        for(int j = 0; j < FIELD_SIZE; j++) {
            if(playerfield[i][j].buttonID == buttonID) {
                if(playerfield[i][j].hidden) {
                    // So it is ignored in the revealAll method
                    playerfield[i][j].hidden = false;
                    playerfield[i][j].fieldType = getFieldType(i, j);
                    if (playerfield[i][j].fieldType == FieldType::MINE) {
                        revealedButtons.push_back(playerfield[i][j]);
                        playerfield[i][j].hidden = true; // Will be revealed later
                        return revealedButtons;
                    }
                    std::vector<Playerield> vector = revealFieldsAroundMove(i, j);
                    return vector;
                } else {
                    return revealedButtons;
                }
            }
        }
    }
    return revealedButtons;
}

std::vector<Playerield> Game::revealFieldsAroundMove(int row, int column) {
    std::vector<Playerield> revealedFields;
    std::queue<Playerield> toProcess;
    playerfield[row][column].row = row;
    playerfield[row][column].column = column;
    toProcess.push(playerfield[row][column]);
    revealedFields.push_back(playerfield[row][column]);

    while(toProcess.size() > 0) {
        Playerield processed = toProcess.front();
        toProcess.pop();
        row = processed.row;
        column = processed.column;
        if (processed.hidden) {
            FieldType fieldType = getFieldType(row, column);
            if (fieldType == FieldType::MINE) {
                continue;
            }
            processed.hidden = false;
            playerfield[row][column].hidden = false;
            playerfield[row][column].fieldType = fieldType;
            revealedFields.push_back(playerfield[row][column]);
            if (fieldType == FieldType::ONE || fieldType == FieldType::TWO || fieldType == FieldType::THREE || fieldType == FieldType::FOUR || fieldType == FieldType::FIVE) {
                continue;
            }
        }
        int r_start = row - 1 < 0 ? 0 : row - 1;
        int r_end = row + 1 >= FIELD_SIZE ? FIELD_SIZE - 1 : row + 1;
        int c_start = column - 1 < 0 ? 0 : column - 1;
        int c_end = column + 1 >= FIELD_SIZE ? FIELD_SIZE - 1 : column + 1;
        for(int i = r_start; i <= r_end; i++) {
            for(int j = c_start; j <= c_end; j++) {
                if (playerfield[i][j].hidden) {
                    playerfield[i][j].row = i;
                    playerfield[i][j].column = j;
                    toProcess.push(playerfield[i][j]);
                }
            }
        }
    }

    

    return revealedFields;
}

std::vector<Playerield> Game::revealAll() {
    std::vector<Playerield> revealedButtons;
    for (int i = 0; i < FIELD_SIZE; i++) {
        for(int j = 0; j < FIELD_SIZE; j++) {
            if(playerfield[i][j].hidden) {
                playerfield[i][j].hidden = false;
                playerfield[i][j].fieldType = getFieldType(i, j);
                revealedButtons.push_back(playerfield[i][j]);
            }
        }
    }
    return revealedButtons;
}

bool Game::game_won() {
    for (int i = 0; i < FIELD_SIZE; i++) {
        for(int j = 0; j < FIELD_SIZE; j++) {
            if(playerfield[i][j].hidden) {
                if(getFieldType(i, j) != FieldType::MINE) return false;
            }
        }
    }
    return true;
}
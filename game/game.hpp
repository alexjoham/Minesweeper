#pragma once
#include <vector>
#include <memory>
#include "../button/button.hpp"
#include "../enums/field_type.hpp"

struct Playerield
{
    bool hidden = true;
    bool flagged = false;
    int buttonID = -1;
    FieldType fieldType = FieldType::HIDDEN;
    int row;
    int column;
};


class Game {
    private:
        /**
         * minefield state
         * -1: mine
         * 0: none
         * 1: one mine nearby, for all other numbers the same
         */
        int minefield[9][9];

        Playerield playerfield[9][9];

        void generateRandomField();

        void updateFieldsAroundmine(int row, int column);

        std::vector<Playerield> revealFieldsAroundMove(int row, int column);

        FieldType getFieldType(int row, int column) {
            switch (minefield[row][column])
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
            default:
                return FieldType::NEUTRAL;
                break;
            }
        }

    public:
        const int NUM_MINES = 10;
        const int FIELD_SIZE = 9;

        void setPlayerfield(std::vector<int> buttonIDs) {
            size_t index = 0;
            for (int i = 0; i < FIELD_SIZE; i++) {
                for(int j = 0; j < FIELD_SIZE; j++) {
                    playerfield[i][j].buttonID = buttonIDs.at(index);
                    index++;
                }
            }
        }

        void startGame();

        /**
         * @return height of the field
         */
        int drawGame(int x, int y, std::vector<std::unique_ptr<Button>> &buttons);

        std::vector<Playerield> revealAll();

        std::vector<Playerield> makeMove(int buttonID);

        Playerield toggleFlag(int buttonID) { 
            for (int i = 0; i < FIELD_SIZE; i++) {
                for(int j = 0; j < FIELD_SIZE; j++) {
                    if (playerfield[i][j].buttonID == buttonID && playerfield[i][j].hidden) {
                        playerfield[i][j].flagged = !playerfield[i][j].flagged;
                        return playerfield[i][j];
                    }
                }
            }
            return Playerield{};
        }

        bool game_won();
};
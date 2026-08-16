#include "button.hpp"
#include <vector>
#include "enums/field_type.hpp"

struct Playerield
{
    bool hidden = true;
    int buttonID;
    FieldType fieldType = FieldType::HIDDEN;
    int row;
    int column;
};


class Game {
    private:
        /**
         * minefield state
         * -1: bomb
         * 0: none
         * 1: one bomb nearby, for all other numbers the same
         */
        int minefield[9][9];

        Playerield playerfield[9][9];

        void generateRandomField();

        void updateFieldsAroundBomb(int row, int column);

        std::vector<Playerield> revealFieldsAroundMove(int row, int column);

        FieldType getFieldType(int row, int column) {
            switch (minefield[row][column])
            {
            case -1:
                return FieldType::BOMB;
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
            int index = 0;
            for (int i = 0; i < FIELD_SIZE; i++) {
                for(int j = 0; j < FIELD_SIZE; j++) {
                    playerfield[i][j].buttonID = buttonIDs.at(index);
                    index++;
                }
            }
        }

        void startGame();

        void drawGame(int x, int y, std::vector<std::unique_ptr<Button>> &buttons);

        std::vector<Playerield> makeMove(int buttonID);
};
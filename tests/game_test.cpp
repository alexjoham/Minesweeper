#include "src/game/game.hpp"
#include "src/structs/board_coord.hpp"

#include <cstdio>
#include <string>
#include <cstddef>
#include <algorithm>
#include <tuple>


constexpr std::string_view RESET = "\033[0m";
constexpr std::string_view RED = "\033[31m";
constexpr std::string_view GREEN = "\033[32m";

namespace {

    struct AcceptCase {
        Game::MineLayout layout;
        Game::Board expected_board;
        const char* what;
    };

    int failures = 0;

    Game::Board getEmptyRevealedBoard() {
        Game::Board board;
        for(auto &row : board) std::fill(std::begin(row), std::end(row), Playerield{ false, false, FieldType::NEUTRAL });
        return board;
    }

    void check(bool condition, const char* what) {
        if (!condition) {
            std::printf("%s FAIL: %s %s\n", RED.data(), RESET.data(), what);
            ++failures;
        }
    }

    void checkAcceptCase(AcceptCase acc_case) {
        Game game = Game(acc_case.layout);
        game.revealAll();
        Game::Board returned_field = game.getPlayerfield();

        bool match = true;
        for (size_t i = 0; i < Game::kFieldSize; i++) {
            for(size_t j = 0; j < Game::kFieldSize; j++) {
                if (returned_field[i][j].fieldType == acc_case.expected_board[i][j].fieldType) {
                    continue;
                }
                match = false;
                break;
            }
            if (match == false) {
                break;
            }
        }
        check(match, acc_case.what);
    }

    void runCases() {
        Game::MineLayout test_layout_1{};
        test_layout_1[0][0] = true;
        Game::Board exp_field_1 = getEmptyRevealedBoard();
        exp_field_1[0][0] = { false, false, FieldType::MINE };
        exp_field_1[1][0] = { false, false, FieldType::ONE };
        exp_field_1[1][1] = { false, false, FieldType::ONE };
        exp_field_1[0][1] = { false, false, FieldType::ONE };
        AcceptCase acc_case_1 = { test_layout_1, exp_field_1, "Mine in { 0, 0 } is handled correctly"};

        Game::MineLayout test_layout_2{};
        Game::Board exp_field_2 = getEmptyRevealedBoard();
        for (size_t i = 3; i <= 5; i++) {
            for (size_t j = 3; j <= 5; j++) {
                if (i == j && i == 4) {
                    continue;
                }
                test_layout_2[i][j] = true;
                exp_field_2[i][j] = { false, false, FieldType::MINE };
            }
        }
        exp_field_2[4][4] = { false, false, FieldType::EIGHT };
        exp_field_2[2][2] = { false, false, FieldType::ONE };
        exp_field_2[2][6] = { false, false, FieldType::ONE };
        exp_field_2[6][2] = { false, false, FieldType::ONE };
        exp_field_2[6][6] = { false, false, FieldType::ONE };

        exp_field_2[2][3] = { false, false, FieldType::TWO };
        exp_field_2[2][5] = { false, false, FieldType::TWO };
        exp_field_2[3][2] = { false, false, FieldType::TWO };
        exp_field_2[3][6] = { false, false, FieldType::TWO };
        exp_field_2[5][2] = { false, false, FieldType::TWO };
        exp_field_2[5][6] = { false, false, FieldType::TWO };
        exp_field_2[6][3] = { false, false, FieldType::TWO };
        exp_field_2[6][5] = { false, false, FieldType::TWO };

        exp_field_2[2][4] = { false, false, FieldType::THREE };
        exp_field_2[4][2] = { false, false, FieldType::THREE };
        exp_field_2[4][6] = { false, false, FieldType::THREE };
        exp_field_2[6][4] = { false, false, FieldType::THREE };
        AcceptCase acc_case_2 = { test_layout_2, exp_field_2, "Eight mines around field { 4, 4 } is handled correctly"};
        
        checkAcceptCase(acc_case_1);
        checkAcceptCase(acc_case_2);

        Game game = Game();
        game.revealAll();
        Game::Board board = game.getPlayerfield();
        int rem_mines = 10;
        for (size_t i = 0; i < game.kFieldSize; i++) {
            for (size_t j = 0; j < game.kFieldSize; j++) {
                if (board[i][j].fieldType == FieldType::MINE) {
                    rem_mines -= 1;
                }
            }
        }
        check(rem_mines == 0, "Random generated field has 10 mines");
    }
}

int main() {
    static_assert(std::is_copy_assignable_v<Game>);
    runCases();

    if (failures == 0) {
        std::printf("%sall checks passed%s\n", GREEN.data(), RESET.data());
        return 0;
    }
    std::printf("%s %d %s\n", RED.data(), failures, RESET.data());
    return 1;
}
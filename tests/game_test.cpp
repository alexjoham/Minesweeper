#include "src/game/game.hpp"
#include "src/structs/board_coord.hpp"

#include <cstdio>
#include <string>
#include <cstddef>
#include <algorithm>
#include <tuple>
#include <set>


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

    void check(bool condition, const std::string& what) {
        if (!condition) {
            std::printf("%s FAIL: %s %s\n", RED.data(), RESET.data(), what.c_str());
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

    Game::MineLayout layoutWith(std::initializer_list<BoardCoord> mines) {
        Game::MineLayout test_layout{};
        for (BoardCoord mine : mines) {
            test_layout[mine.row][mine.column] = true;
        }
        return test_layout;
    }

    void runCases() {
        // Test 1: Mine in { 0, 0 } is handled correctly
        Game::Board exp_field_1 = getEmptyRevealedBoard();
        exp_field_1[0][0] = { false, false, FieldType::MINE };
        exp_field_1[1][0] = { false, false, FieldType::ONE };
        exp_field_1[1][1] = { false, false, FieldType::ONE };
        exp_field_1[0][1] = { false, false, FieldType::ONE };
        AcceptCase acc_case_1 = { layoutWith({ { 0, 0 } }), exp_field_1, "Mine in { 0, 0 } is handled correctly"};

        // Test 2: Eight mines around field { 4, 4 } is handled correctly
        Game::Board exp_field_2 = getEmptyRevealedBoard();
        for (size_t i = 3; i <= 5; i++) {
            for (size_t j = 3; j <= 5; j++) {
                if (i == j && i == 4) {
                    continue;
                }
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
        AcceptCase acc_case_2 = { layoutWith({
            { 3, 3 }, { 3, 4 }, { 3, 5 },
            { 4, 3 },            { 4, 5 },
            { 5, 3 }, { 5, 4 }, { 5, 5 },
        }), exp_field_2, "Eight mines around field { 4, 4 } is handled correctly"};
        
        checkAcceptCase(acc_case_1);
        checkAcceptCase(acc_case_2);

        // Test 3: Random generated field has 10 mines
        Game game = Game();
        game.revealAll();
        Game::Board board = game.getPlayerfield();
        int rem_mines = Game::kNumMines;
        for (size_t i = 0; i < Game::kFieldSize; i++) {
            for (size_t j = 0; j < Game::kFieldSize; j++) {
                if (board[i][j].fieldType == FieldType::MINE) {
                    rem_mines -= 1;
                }
            }
        }
        check(rem_mines == 0, "Random generated field has 10 mines");

        // Test 4: Hitting a number cell only reveals this cell
        game = Game(layoutWith({ { 0, 0 } }));
        std::vector<RevealedCell> revealed_cells = game.makeMove(0, 1);
        check(revealed_cells.size() == 1, "Hitting a number cell only reveals this cell");

        // Test 5: game.MakeMove does not reveal duplicate cells
        game = Game(layoutWith({ { 0, 0 } }));
        revealed_cells = game.makeMove(8, 8);
        std::set<std::pair<size_t, size_t>> revealed_set;
        int duplicates = 0;
        for (RevealedCell cell : revealed_cells) {
            std::pair<size_t, size_t> coord_pair = std::pair(cell.coordinates.row, cell.coordinates.column);
            if (revealed_set.contains(coord_pair)) {
                duplicates++;
                continue;
            }
            revealed_set.emplace(coord_pair);
        }
        check(duplicates == 0, "game.MakeMove does not reveal duplicate cells (found " + std::to_string(duplicates) + " duplicates) for a setup with one mine in (0,0)");

        // Test 6: flagged cells are not revealed
        game = Game(layoutWith({ { 0, 0 } }));
        for (size_t i = 0; i < Game::kFieldSize; i++) {
            for (size_t j = 0; j < Game::kFieldSize; j++) {
                if ((i == 0 && j == 0) || (i == 8 && j == 8)) {
                    continue;
                }
                game.toggleFlag(i, j);
            }
        }
        revealed_cells = game.makeMove(8, 8);
        int number_revealed_flagged_cells = 0;
        for (RevealedCell cell : revealed_cells) {
            if (cell.cell.flagged && !cell.cell.hidden) {
                number_revealed_flagged_cells++;
            }
        }
        check(number_revealed_flagged_cells == 0, "flagged cells are not revealed after clicking on neutral cell (revealed " + std::to_string(number_revealed_flagged_cells) + " flagged cells instead of 0) for a setup with one mine in (0,0) and all other cells flagged except (8,8). Revealed (8,8).");

        // Test 7: flagged cell cannot be revealed
        game = Game(layoutWith({ { 4, 4 } }));
        game.toggleFlag(4,4);
        revealed_cells = game.makeMove(4, 4);
        check(revealed_cells.size() == 0, "flagged cells are not revealed by clicking on it (revealed " + std::to_string(revealed_cells.size()) + " cells instead of 0) for a setup with one flagged cell in (4,4). Tried to revealed (4,4).");

        // Test 8: Hitting a number 6 cell only reveals this cell
        game = Game(layoutWith({
            { 3, 3 }, { 3, 4 }, { 3, 5 },
            { 4, 3 },            { 4, 5 },
            { 5, 3 },
        }));
        revealed_cells = game.makeMove(4, 4);
        check(revealed_cells.size() == 1, "Hitting a number 6 cell only reveals this cell");
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
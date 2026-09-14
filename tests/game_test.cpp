#include "src/game/game.hpp"
#include "src/structs/board_coord.hpp"

#include <algorithm>
#include <set>
#include <gtest/gtest.h>

namespace {

    Game::Board getEmptyRevealedBoard() {
        Game::Board board;
        for (auto& row : board) std::fill(std::begin(row), std::end(row), Playerield{ false, false, FieldType::NEUTRAL });
        return board;
    }

    Game::MineLayout layoutWith(std::initializer_list<BoardCoord> mines) {
        Game::MineLayout test_layout{};
        for (BoardCoord mine : mines) {
            test_layout[mine.row][mine.column] = true;
        }
        return test_layout;
    }

    void expectBoardMatches(const Game::Board& actual, const Game::Board& expected) {
        for (size_t i = 0; i < Game::kFieldSize; i++) {
            for (size_t j = 0; j < Game::kFieldSize; j++) {
                EXPECT_EQ(actual[i][j].fieldType, expected[i][j].fieldType) << "at (" << i << ", " << j << ")";
            }
        }
    }

}  // namespace

TEST(GameTest, IsCopyAssignable) {
    static_assert(std::is_copy_assignable_v<Game>);
}

TEST(GameTest, MineInCornerIsHandledCorrectly) {
    Game::Board expected = getEmptyRevealedBoard();
    expected[0][0] = { false, false, FieldType::MINE };
    expected[1][0] = { false, false, FieldType::ONE };
    expected[1][1] = { false, false, FieldType::ONE };
    expected[0][1] = { false, false, FieldType::ONE };

    Game game = Game(layoutWith({ { 0, 0 } }));
    game.revealAll();
    expectBoardMatches(game.getPlayerfield(), expected);
}

TEST(GameTest, EightMinesAroundFieldIsHandledCorrectly) {
    Game::Board expected = getEmptyRevealedBoard();
    for (size_t i = 3; i <= 5; i++) {
        for (size_t j = 3; j <= 5; j++) {
            if (i == j && i == 4) continue;
            expected[i][j] = { false, false, FieldType::MINE };
        }
    }
    expected[4][4] = { false, false, FieldType::EIGHT };
    expected[2][2] = { false, false, FieldType::ONE };
    expected[2][6] = { false, false, FieldType::ONE };
    expected[6][2] = { false, false, FieldType::ONE };
    expected[6][6] = { false, false, FieldType::ONE };

    expected[2][3] = { false, false, FieldType::TWO };
    expected[2][5] = { false, false, FieldType::TWO };
    expected[3][2] = { false, false, FieldType::TWO };
    expected[3][6] = { false, false, FieldType::TWO };
    expected[5][2] = { false, false, FieldType::TWO };
    expected[5][6] = { false, false, FieldType::TWO };
    expected[6][3] = { false, false, FieldType::TWO };
    expected[6][5] = { false, false, FieldType::TWO };

    expected[2][4] = { false, false, FieldType::THREE };
    expected[4][2] = { false, false, FieldType::THREE };
    expected[4][6] = { false, false, FieldType::THREE };
    expected[6][4] = { false, false, FieldType::THREE };

    Game game = Game(layoutWith({
        { 3, 3 }, { 3, 4 }, { 3, 5 },
        { 4, 3 },           { 4, 5 },
        { 5, 3 }, { 5, 4 }, { 5, 5 },
    }));
    game.revealAll();
    expectBoardMatches(game.getPlayerfield(), expected);
}

TEST(GameTest, RandomGeneratedFieldHasCorrectNumberOfMines) {
    Game game = Game();
    game.revealAll();
    Game::Board board = game.getPlayerfield();

    size_t num_mines = 0;
    for (size_t i = 0; i < Game::kFieldSize; i++) {
        for (size_t j = 0; j < Game::kFieldSize; j++) {
            if (board[i][j].fieldType == FieldType::MINE) ++num_mines;
        }
    }
    EXPECT_EQ(num_mines, Game::kNumMines);
}

TEST(GameTest, HittingANumberCellOnlyRevealsThisCell) {
    Game game = Game(layoutWith({ { 0, 0 } }));
    std::vector<RevealedCell> revealed_cells = game.makeMove(0, 1);
    EXPECT_EQ(revealed_cells.size(), 1u);
}

TEST(GameTest, MakeMoveDoesNotRevealDuplicateCells) {
    Game game = Game(layoutWith({ { 0, 0 } }));
    std::vector<RevealedCell> revealed_cells = game.makeMove(8, 8);

    std::set<std::pair<size_t, size_t>> revealed_set;
    for (const RevealedCell& cell : revealed_cells) {
        auto coord_pair = std::pair(cell.coordinates.row, cell.coordinates.column);
        EXPECT_TRUE(revealed_set.emplace(coord_pair).second) << "duplicate cell (" << coord_pair.first << ", " << coord_pair.second << ")";
    }
}

TEST(GameTest, FlaggedCellsAreNotRevealedByNeighboringMove) {
    Game game = Game(layoutWith({ { 0, 0 } }));
    for (size_t i = 0; i < Game::kFieldSize; i++) {
        for (size_t j = 0; j < Game::kFieldSize; j++) {
            if ((i == 0 && j == 0) || (i == 8 && j == 8)) continue;
            game.toggleFlag(i, j);
        }
    }
    std::vector<RevealedCell> revealed_cells = game.makeMove(8, 8);

    for (const RevealedCell& cell : revealed_cells) {
        EXPECT_FALSE(cell.cell.flagged && !cell.cell.hidden);
    }
}

TEST(GameTest, FlaggedCellCannotBeRevealed) {
    Game game = Game(layoutWith({ { 4, 4 } }));
    game.toggleFlag(4, 4);
    std::vector<RevealedCell> revealed_cells = game.makeMove(4, 4);
    EXPECT_EQ(revealed_cells.size(), 0u);
}

TEST(GameTest, HittingANumberSixCellOnlyRevealsThisCell) {
    Game game = Game(layoutWith({
        { 3, 3 }, { 3, 4 }, { 3, 5 },
        { 4, 3 },           { 4, 5 },
        { 5, 3 },
    }));
    std::vector<RevealedCell> revealed_cells = game.makeMove(4, 4);
    EXPECT_EQ(revealed_cells.size(), 1u);
}

TEST(GameTest, MakeMoveOnMineEveryOtherCellStaysHiddenExceptMineCell) {
    Game game = Game(layoutWith({ { 4, 4 } }));
    std::vector<RevealedCell> revealed_cells = game.makeMove(4, 4);
    EXPECT_EQ(revealed_cells.size(), 1u);
    Game::Board board = game.getPlayerfield();
    for (size_t i = 0; i < Game::kFieldSize; i++) {
        for (size_t j = 0; j < Game::kFieldSize; j++) {
            if (board[i][j].hidden) {
                EXPECT_EQ(board[i][j].fieldType, FieldType::HIDDEN) << "at (" << i << "," << j << ")";
            }
        }
    }
}

TEST(GameTest, ToggleFlagOnMineCellThatMakeMoveConsumedReturnsNullopt) {
    Game game = Game(layoutWith({ { 4, 4 } }));
    game.makeMove(4, 4);
    EXPECT_FALSE(game.toggleFlag(4, 4).has_value());
}

TEST(GameTest, ToggleFlagOnNonMineCellThatMakeMoveConsumedReturnsNullopt) {
    Game game = Game(layoutWith({ { 4, 4 } }));
    game.makeMove(3, 3);
    EXPECT_FALSE(game.toggleFlag(3, 3).has_value());
}

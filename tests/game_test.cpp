#include "src/game/game.hpp"
#include "src/game/random_layout.hpp"
#include "src/enums/field_type.hpp"
#include "src/structs/board_coord.hpp"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <random>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

static_assert(std::is_copy_assignable_v<Game>);

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

TEST(GameTest, RandomLayoutProducesRequestedMineCount) {
    std::mt19937 rng{1};
    Game::MineLayout layout = randomLayout<Game::kFieldSize>(Game::kFieldSize * Game::kFieldSize, rng);

    size_t num_mines = 0;
    for (size_t i = 0; i < Game::kFieldSize; i++) {
        for (size_t j = 0; j < Game::kFieldSize; j++) {
            if (layout[i][j]) ++num_mines;
        }
    }
    EXPECT_EQ(num_mines, Game::kFieldSize * Game::kFieldSize);
}

TEST(GameTest, RandomLayoutClampsMineCountToBoardSize) {
    std::mt19937 rng{1};
    Game::MineLayout layout = randomLayout<Game::kFieldSize>(Game::kFieldSize * Game::kFieldSize + 1, rng);

    size_t num_mines = 0;
    for (size_t i = 0; i < Game::kFieldSize; i++) {
        for (size_t j = 0; j < Game::kFieldSize; j++) {
            if (layout[i][j]) ++num_mines;
        }
    }
    EXPECT_EQ(num_mines, Game::kFieldSize * Game::kFieldSize);
}

TEST(GameTest, RandomLayoutIsDeterministicForAFixedSeed) {
    std::mt19937 rng_a{42};
    std::mt19937 rng_b{42};
    Game::MineLayout layout_a = randomLayout<Game::kFieldSize>(Game::kNumMines, rng_a);
    Game::MineLayout layout_b = randomLayout<Game::kFieldSize>(Game::kNumMines, rng_b);

    EXPECT_EQ(layout_a, layout_b);
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

TEST(GameTest, RestartSetsNewBoardWithAllCellsHidden) {
    Game game = Game(layoutWith({ {4,4} }));
    game.makeMove(4, 4);
    game.restart();
    Game::Board board = game.getPlayerfield();
    for (size_t i = 0; i < Game::kFieldSize; i++) {
        for (size_t j = 0; j < Game::kFieldSize; j++) {
            EXPECT_EQ(board[i][j].fieldType, FieldType::HIDDEN) << "at (" << i << "," << j << ")";
            EXPECT_TRUE(board[i][j].hidden);
        }
    }
}

TEST(GameTest, RestartClearsFlaggedCells) {
    Game game = Game(layoutWith({ { 4, 4 } }));
    ASSERT_TRUE(game.toggleFlag(0, 0).has_value());
    game.restart();
    Game::Board board = game.getPlayerfield();
    for (size_t i = 0; i < Game::kFieldSize; i++) {
        for (size_t j = 0; j < Game::kFieldSize; j++) {
            EXPECT_FALSE(board[i][j].flagged) << "at (" << i << "," << j << ")";
        }
    }
}

TEST(GameTest, MakeMoveWithOutOfRangeRowReturnsNoRevealedCells) {
    Game game = Game(layoutWith({ { 4, 4 } }));
    EXPECT_TRUE(game.makeMove(Game::kFieldSize, 0).empty());
}

TEST(GameTest, MakeMoveWithOutOfRangeColumnReturnsNoRevealedCells) {
    Game game = Game(layoutWith({ { 4, 4 } }));
    EXPECT_TRUE(game.makeMove(0, Game::kFieldSize).empty());
}

TEST(GameTest, MakeMoveOnAlreadyRevealedCellReturnsNoRevealedCells) {
    Game game = Game(layoutWith({ { 4, 4 } }));
    ASSERT_EQ(game.makeMove(3, 3).size(), 1u);
    EXPECT_TRUE(game.makeMove(3, 3).empty());
}

TEST(GameTest, RevealAllOnlyRevealsStillHiddenCells) {
    Game game = Game(layoutWith({ { 4, 4 } }));
    ASSERT_EQ(game.makeMove(3, 3).size(), 1u);

    std::vector<RevealedCell> revealed_by_reveal_all = game.revealAll();
    EXPECT_EQ(revealed_by_reveal_all.size(), Game::kFieldSize * Game::kFieldSize - 1);

    bool contains_already_revealed_cell = std::any_of(
        revealed_by_reveal_all.begin(), revealed_by_reveal_all.end(),
        [](const RevealedCell& cell) { return cell.coordinates.row == 3 && cell.coordinates.column == 3; });
    EXPECT_FALSE(contains_already_revealed_cell);
}

TEST(GameTest, ToggleFlagTwiceReturnsCellToUnflagged) {
    Game game = Game(layoutWith({ { 4, 4 } }));

    std::optional<Playerield> first_toggle = game.toggleFlag(0, 0);
    ASSERT_TRUE(first_toggle.has_value());
    EXPECT_TRUE(first_toggle->flagged);

    std::optional<Playerield> second_toggle = game.toggleFlag(0, 0);
    ASSERT_TRUE(second_toggle.has_value());
    EXPECT_FALSE(second_toggle->flagged);
}

TEST(GameTest, GameWonIsFalseOnAFreshBoard) {
    Game game = Game(layoutWith({ { 8, 8 } }));
    EXPECT_FALSE(game.game_won());
}

TEST(GameTest, GameWonIsFalseWhileNonMineCellsRemainHidden) {
    Game game = Game(layoutWith({ { 4, 4 } }));
    ASSERT_EQ(game.makeMove(3, 3).size(), 1u);
    EXPECT_FALSE(game.game_won());
}

TEST(GameTest, GameWonIsTrueOnceEveryNonMineCellIsRevealed) {
    Game game = Game(layoutWith({ { 8, 8 } }));
    game.makeMove(0, 0);
    EXPECT_TRUE(game.game_won());
}

TEST(GameTest, MakeMoveFloodFillStopsAtNumberedBoundaryLeavingSurroundedCellsHidden) {
    Game game = Game(layoutWith({
        { 3, 3 }, { 3, 4 }, { 3, 5 },
        { 4, 3 },           { 4, 5 },
        { 5, 3 }, { 5, 4 }, { 5, 5 },
    }));
    game.makeMove(0, 0);
    Game::Board board = game.getPlayerfield();

    // The 8 mines plus the fully mine-surrounded centre cell are never
    // adjacent to a NEUTRAL cell, so flood fill from a distant corner
    // cannot reach them; everything else on the board is connected to
    // that corner and gets revealed.
    std::set<std::pair<size_t, size_t>> expected_still_hidden = {
        { 3, 3 }, { 3, 4 }, { 3, 5 },
        { 4, 3 }, { 4, 4 }, { 4, 5 },
        { 5, 3 }, { 5, 4 }, { 5, 5 },
    };
    for (size_t i = 0; i < Game::kFieldSize; i++) {
        for (size_t j = 0; j < Game::kFieldSize; j++) {
            bool should_be_hidden = expected_still_hidden.count({ i, j }) > 0;
            EXPECT_EQ(board[i][j].hidden, should_be_hidden) << "at (" << i << ", " << j << ")";
        }
    }
}

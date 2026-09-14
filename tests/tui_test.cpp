#include "src/tui/tui.hpp"
#include "src/structs/board_coord.hpp"

#include <gtest/gtest.h>

using namespace std::string_view_literals;

namespace {

struct RejectCase {
    int screen_row;
    int screen_col;
    const char* what;
};

struct AcceptCase {
    int screen_row;
    int screen_col;
    BoardCoord coord;
    const char* what;
};

struct Origin {
    int row;
    int col;
};

class TuiBoardCellAtTest : public testing::TestWithParam<Origin> {};

TEST_P(TuiBoardCellAtTest, RejectsCoordinatesOutsideTheGrid) {
    const Origin origin = GetParam();
    const int kCellWidth = 3;
    const int first_row = origin.row + 1;
    const int last_row  = origin.row + static_cast<int>(Game::kFieldSize);
    const int first_col = origin.col + 1;
    const int last_col  = origin.col + static_cast<int>(Game::kFieldSize) * kCellWidth;

    Tui tui(origin.row, origin.col);

    const RejectCase rejects[] = {
        { first_row - 1, first_col - 1, "origin is not in the grid" },
        { first_row - 1, first_col,     "one row above the grid" },
        { last_row + 1,  first_col,     "one row below the grid" },
        { first_row,     first_col - 1, "one column left of the grid" },
        { first_row,     last_col + 1,  "one column right of the grid" },
    };

    for (const RejectCase& t : rejects) {
        SCOPED_TRACE(t.what);
        EXPECT_FALSE(tui.boardCellAt(t.screen_row, t.screen_col).has_value());
    }
}

TEST_P(TuiBoardCellAtTest, AcceptsCoordinatesInsideTheGrid) {
    const Origin origin = GetParam();
    const int first_row = origin.row + 1;
    const int last_row  = origin.row + static_cast<int>(Game::kFieldSize);
    const int first_col = origin.col + 1;
    const int last_col  = origin.col + static_cast<int>(Game::kFieldSize) * 3;

    Tui tui(origin.row, origin.col);

    const AcceptCase accepts[] = {
        { first_row, first_col,     BoardCoord{0, 0}, "first cell" },
        { last_row,  first_col,     BoardCoord{8, 0}, "last row in the first column" },
        { first_row, last_col,      BoardCoord{0, 8}, "first row in the last column" },
        { first_row, first_col + 2, BoardCoord{0, 0}, "last column of the first cell" },
        { first_row, first_col + 3, BoardCoord{0, 1}, "first column of the second cell" },
    };

    for (const AcceptCase& t : accepts) {
        SCOPED_TRACE(t.what);
        auto cell = tui.boardCellAt(t.screen_row, t.screen_col);
        ASSERT_TRUE(cell.has_value());
        EXPECT_EQ(cell->row, t.coord.row);
        EXPECT_EQ(cell->column, t.coord.column);
    }
}

TEST(TuiTest, GlyphFunctionReturnsHiddenSymbolForEveryFieldType) {
    Playerield pf;
    pf.hidden = true;
    pf.flagged = false;
    EXPECT_EQ(Tui::glyph(pf), "\u25A2"sv);
    pf.fieldType = FieldType::MINE;
    EXPECT_EQ(Tui::glyph(pf), "\u25A2"sv);
    pf.fieldType = FieldType::ONE;
    EXPECT_EQ(Tui::glyph(pf), "\u25A2"sv);
    pf.fieldType = FieldType::TWO;
    EXPECT_EQ(Tui::glyph(pf), "\u25A2"sv);
    pf.fieldType = FieldType::THREE;
    EXPECT_EQ(Tui::glyph(pf), "\u25A2"sv);
    pf.fieldType = FieldType::FOUR;
    EXPECT_EQ(Tui::glyph(pf), "\u25A2"sv);
    pf.fieldType = FieldType::FIVE;
    EXPECT_EQ(Tui::glyph(pf), "\u25A2"sv);
    pf.fieldType = FieldType::SIX;
    EXPECT_EQ(Tui::glyph(pf), "\u25A2"sv);
    pf.fieldType = FieldType::SEVEN;
    EXPECT_EQ(Tui::glyph(pf), "\u25A2"sv);
    pf.fieldType = FieldType::EIGHT;
    EXPECT_EQ(Tui::glyph(pf), "\u25A2"sv);
    pf.fieldType = FieldType::NEUTRAL;
    EXPECT_EQ(Tui::glyph(pf), "\u25A2"sv);
    pf.fieldType = FieldType::MARKED_MINE;
    EXPECT_EQ(Tui::glyph(pf), "\u25A2"sv);

}

INSTANTIATE_TEST_SUITE_P(
    Origins,
    TuiBoardCellAtTest,
    testing::Values(Origin{0, 0}, Origin{2, 5}));

}  // namespace

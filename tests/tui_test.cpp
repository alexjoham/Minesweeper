#include "src/tui/tui.hpp"
#include "src/enums/field_type.hpp"
#include "src/game/game.hpp"
#include "src/structs/board_coord.hpp"

#include <string>
#include <string_view>

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

std::string originName(const testing::TestParamInfo<Origin>& info) {
    return "Row" + std::to_string(info.param.row) + "Col" + std::to_string(info.param.col);
}

std::string fieldTypeName(const testing::TestParamInfo<FieldType>& info) {
    switch (info.param) {
        case FieldType::HIDDEN:      return "Hidden";
        case FieldType::NEUTRAL:     return "Neutral";
        case FieldType::MINE:        return "Mine";
        case FieldType::MARKED_MINE: return "MarkedMine";
        case FieldType::ONE:         return "One";
        case FieldType::TWO:         return "Two";
        case FieldType::THREE:       return "Three";
        case FieldType::FOUR:        return "Four";
        case FieldType::FIVE:        return "Five";
        case FieldType::SIX:         return "Six";
        case FieldType::SEVEN:       return "Seven";
        case FieldType::EIGHT:       return "Eight";
    }
    return "Unknown";
}

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

class TuiGlyphHiddenTest : public testing::TestWithParam<FieldType> {};

TEST_P(TuiGlyphHiddenTest, ReturnsHiddenSymbol) {
    Playerield pf;
    pf.hidden = true;
    pf.flagged = false;
    pf.fieldType = GetParam();
    EXPECT_EQ(Tui::glyph(pf), "\u25A2"sv);
}

INSTANTIATE_TEST_SUITE_P(
    FieldTypes,
    TuiGlyphHiddenTest,
    testing::Values(
        FieldType::MINE,
        FieldType::ONE,
        FieldType::TWO,
        FieldType::THREE,
        FieldType::FOUR,
        FieldType::FIVE,
        FieldType::SIX,
        FieldType::SEVEN,
        FieldType::EIGHT,
        FieldType::NEUTRAL,
        FieldType::MARKED_MINE,
        FieldType::HIDDEN),
    fieldTypeName);

INSTANTIATE_TEST_SUITE_P(
    Origins,
    TuiBoardCellAtTest,
    testing::Values(Origin{0, 0}, Origin{2, 5}),
    originName);

}  // namespace

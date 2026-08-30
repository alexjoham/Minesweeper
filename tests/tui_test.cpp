#include "src/tui/tui.hpp"
#include "src/structs/board_coord.hpp"

#include <cstdio>
#include <string>
#include <cstddef>


#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BOLDBLACK   "\033[1m\033[30m"

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

namespace {

    int failures = 0;

    void check(bool condition, const char* what) {
        if (!condition) {
            std::string text = std::string(RED) + "FAIL: " + std::string(RESET) + what + "\n";
            std::printf("%s", text.c_str());
            ++failures;
        }
    }

}

int main() {

    int origin_row = 0;
    int origin_column = 0;
    int kCellWidth = 3;
    const int first_row = origin_row + 1;
    const int last_row  = origin_row + static_cast<int>(Game::kFieldSize);
    const int first_col = origin_column + 1;
    const int last_col  = origin_column + static_cast<int>(Game::kFieldSize) * kCellWidth;

    Tui tui = Tui(origin_row, origin_column);

    const RejectCase rejects[] = {
        { first_row - 1, first_col + 1, "origin is not in the grid"},
        { first_row - 1, first_col,  "one row above the grid" },
        { last_row + 1, first_col,  "one row below the grid" },
        { first_row, first_col - 1, "one column left of the grid" },
        { first_row, last_col + 1, "one column right of the grid"},
    };

    const AcceptCase accepts[] = {
        { first_row, first_col, BoardCoord{0,0}, "first cell"},
        { last_row, first_col,  BoardCoord{8,0}, "last row in the first column" },
        { first_row, last_col,  BoardCoord{0,8}, "first row in the last column" },
        { first_row, first_col + 2, BoardCoord{0,0}, "last column of the first cell" },
        { first_row, first_col + 3, BoardCoord{0,1}, "first column of the second cell"},
    };

    for (const RejectCase& t : rejects) {
        auto cell = tui.boardCellAt(t.screen_row, t.screen_col);
        check(!cell.has_value(), t.what);
    }

    for (const AcceptCase& t : accepts) {
        auto cell = tui.boardCellAt(t.screen_row, t.screen_col);
        check(cell.has_value() && cell.value().row == t.coord.row && cell.value().column == t.coord.column, t.what);
    }

    if (failures == 0) {
        std::string text = std::string(GREEN) + "all checks passed" + std::string(RESET) + "\n";
        std::printf("%s", text.c_str());
        return 0;
    }
    std::string text = std::string(RED) + std::to_string(failures) + " check(s) failed" + std::string(RESET) + "\n";
    std::printf("%s", text.c_str());
    return 1;
}
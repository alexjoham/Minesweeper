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

    Tui tui = Tui(origin_row, origin_column);

    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            int row_pos = origin_row + i + 1;
            int column_pos = origin_column + j * kCellWidth + 1;
            auto cell = tui.boardCellAt(row_pos, column_pos);
            std::string msg = "No value received for row " + std::to_string(i) + ", column " + std::to_string(j);
            bool cell_has_value = cell.has_value();
            check(cell_has_value, msg.c_str());
            if (cell_has_value) {
                msg = "Received wrong row for row: " + std::to_string(i) + " and column " + std::to_string(j);
                check(cell.value().row == static_cast<size_t>(i), msg.c_str());
                msg = "Received wrong column for row: " + std::to_string(i) + " and column " + std::to_string(j);
                check(cell.value().column == static_cast<size_t>(j), msg.c_str());
            }
        }
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
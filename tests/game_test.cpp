#include "src/tui/tui.hpp"
#include "src/structs/board_coord.hpp"

#include <cstdio>
#include <string>
#include <cstddef>


constexpr std::string_view RESET = "\033[0m";
constexpr std::string_view RED = "\033[31m";
constexpr std::string_view GREEN = "\033[32m";

namespace {

    int failures = 0;

    // void check(bool condition, const char* what) {
    //     if (!condition) {
    //         std::printf("%s FAIL: %s %s\n", RED.data(), RESET.data(), what);
    //         ++failures;
    //     }
    // }
}

int main() {
    static_assert(std::is_copy_assignable_v<Game>);
    //check(, "Game is copy assignable");

    if (failures == 0) {
        std::printf("%sall checks passed%s\n", GREEN.data(), RESET.data());
        return 0;
    }
    std::string text = std::string(RED) + std::to_string(failures) + " check(s) failed" + std::string(RESET) + "\n";
    std::printf("%s %d %s\n", RED.data(), failures, RESET.data());
    return 1;
}
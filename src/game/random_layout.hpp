#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <numeric>
#include <random>

// randomLayout has no relationship to Game's state: it is a pure function of
// (board size, mine count, rng). It is total for every mine_count -- values
// above board_size*board_size are clamped rather than asserted on, so the
// same behaviour holds in Debug and Release builds; there is no build
// configuration in which it can read out of bounds or loop forever.
template <std::size_t N>
std::array<std::array<bool, N>, N> randomLayout(std::size_t mine_count, std::mt19937& rng) {
    constexpr std::size_t kCellCount = N * N;
    const std::size_t clamped_count = std::min(mine_count, kCellCount);

    std::array<std::array<bool, N>, N> layout{};

    std::array<std::size_t, kCellCount> cells{};
    std::iota(cells.begin(), cells.end(), std::size_t{0});
    std::shuffle(cells.begin(), cells.end(), rng);

    for (std::size_t i = 0; i < clamped_count; i++) {
        const std::size_t cell = cells[i];
        layout[cell / N][cell % N] = true;
    }
    return layout;
}

inline std::mt19937& defaultRng() {
    static std::mt19937 rng{std::random_device{}()};
    return rng;
}

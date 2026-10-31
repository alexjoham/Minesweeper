#pragma once
#include <cstdint>

// How a game ends, decided by Game. Unlike GameState it has no menu: the
// menu is a screen of the UI, not a state of the game.
enum class GameOutcome : std::uint8_t { Playing, Won, Lost };

#include "button/button.hpp"
#include "enums/game_outcome.hpp"
#include "enums/game_state.hpp"
#include "game/game.hpp"
#include "helpers.hpp"
#include "input/sgr_parser.hpp"
#include "tui/tui.hpp"

#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <signal.h>
#include <string>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <variant>
#include <vector>

constexpr int PLAYING_FIELD_X = 2;
constexpr int PLAYING_FIELD_Y = 2;

static termios term;
static bool g_raw_active = false;
GameState state = GameState::MENU;
Game game;
Tui tui = Tui(PLAYING_FIELD_X, PLAYING_FIELD_Y);
static bool set_flag = false;
UnicodeButton flag_button = UnicodeButton(1, 3, 3, "\u2691", "\x1b[93m");

static void restore_terminal() {
    if (!g_raw_active)
        return;
    g_raw_active = false;
    emit_terminal_command("\x1b[?1006l"   // disable SGR mouse mode
                          "\x1b[?1002l"   // drag reporting off
                          "\x1b[?1000l"   // mouse reporting off
                          "\x1b[?25h"     // show the cursor
                          "\x1b[?1049l"); // leave alternate screen
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

static void on_signal(int) {
    restore_terminal();
    _exit(0);
}

static void setup_terminal() {
    if (tcgetattr(STDIN_FILENO, &term) == -1) {
        std::fprintf(stderr, "not a terminal\n");
        std::exit(1);
    }

    termios raw = term;

    raw.c_lflag &= ~(tcflag_t)(ECHO | ICANON | ISIG);
    raw.c_iflag &= ~(tcflag_t)(IXON | ICRNL);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    g_raw_active = true;

    emit_terminal_command("\x1b[?1049h"   // alternative screen bugger
                          "\x1b[?25l"     // hide the cursor
                          "\x1b[?1000h"   // mouse reporting
                          "\x1b[?1002h"   // drag reporting
                          "\x1b[?1006h"); // enable SGR mouse mode

    std::atexit(restore_terminal);
    std::signal(SIGTERM, on_signal);
    std::signal(SIGHUP, on_signal);
}

static void draw_all(std::vector<std::unique_ptr<Button>>& buttons) {
    clear_screen();
    emit_terminal_command(at(3, 3) + "\x1b[2mWelcome to Minesweeper. Press q to quit.\x1b[0m");
    for (const auto& b : buttons)
        b->draw();
}

static void drawGameInfo() {
    flag_button.draw();
    emit_terminal_command(at(0, 7) + "\x1b[2mPress q to quit.\x1b[0m");
}

static void drawGameLost() {
    emit_terminal_command(at(0, 0) +
                          "\x1b[91mGAME LOST\x1b[0m\x1b[2m. Press \x1b[0m\x1b[1mr\x1b[0m\x1b[2m to retry.\x1b[0m");
}

static void drawGameWon() {
    emit_terminal_command(at(0, 0) +
                          "\x1b[92mGAME WON!\x1b[0m\x1b[2m. Press \x1b[0m\x1b[1mr\x1b[0m\x1b[2m to replay.\x1b[0m");
}

static void drawGameRules(int x, int y) {
    emit_terminal_command(at(x, y) + "\x1b[2mLeft-click to open a square.\x1b[0m");
    emit_terminal_command(at(x + 1, y) +
                          "\x1b[2mSelect the flag to place a flag with left click where you think a mine is.\x1b[0m");
    emit_terminal_command(at(x + 2, y) + "\x1b[2mThe numbers show you how many mines are around this square "
                                         "(vertically, horizontally and diagonally.\x1b[0m");
}

static void handle_key(std::uint8_t key, bool& running) {
    if (key == 'q' || key == 3 /* Ctrl+C */) {
        running = false;
    } else if (key == 'r') {
        state = GameState::GAME;
        game.restart();
        clear_screen();
        drawGameInfo();
        tui.drawGame(game.getPlayerfield());
    }
}

static void handle_mouse(const MouseEvent& mouse, std::vector<std::unique_ptr<Button>>& buttons, int start_id,
                         int quit_id, bool& running) {
    // Only left press and release matter; drag, wheel and the other buttons are ignored.
    if (mouse.button != MouseButton::Left || mouse.action == MouseAction::Motion) {
        return;
    }
    // Button and Tui take the screen row first, then the column.
    const int row = mouse.row;
    const int column = mouse.column;

    if (mouse.action == MouseAction::Press) {
        for (auto& b : buttons)
            b->press(row, column);
        flag_button.press(row, column);
        return;
    }

    switch (state) {
        case GameState::MENU: {
            bool switch_to_game = false;
            bool quit_clicked = false;
            for (auto& b : buttons) {
                if (b->release(row, column)) {
                    if (b->getID() == start_id) {
                        switch_to_game = true;
                    } else if (b->getID() == quit_id) {
                        quit_clicked = true;
                    }
                }
            }
            if (quit_clicked) {
                running = false;
                return;
            }
            if (switch_to_game) {
                state = GameState::GAME;
                clear_screen();
                drawGameInfo();
                int height = tui.drawGame(game.getPlayerfield());
                drawGameRules(PLAYING_FIELD_X + height, PLAYING_FIELD_Y);
                return;
            }
            draw_all(buttons);
            break;
        }
        case GameState::GAME: {
            if (flag_button.release(row, column)) {
                set_flag = !set_flag;
                if (set_flag) {
                    flag_button.setColor("\x1b[7m");
                } else {
                    flag_button.setColor("\x1b[93m");
                }
                drawGameInfo();
                tui.drawGame(game.getPlayerfield());
                return;
            }
            if (!set_flag) {
                if (auto cell = tui.boardCellAt(row, column)) {
                    game.makeMove(cell->row, cell->column);
                }
            } else {
                if (auto cell = tui.boardCellAt(row, column)) {
                    game.toggleFlag(cell->row, cell->column);
                }
            }
            if (game.outcome() == GameOutcome::Lost) {
                game.revealAll();
                state = GameState::LOST;
                drawGameLost();
                tui.drawGame(game.getPlayerfield());
                return;
            }
            if (game.outcome() == GameOutcome::Won) {
                game.revealAll();
                drawGameWon();
                tui.drawGame(game.getPlayerfield());
                state = GameState::WON;
                return;
            }
            drawGameInfo();
            int height = tui.drawGame(game.getPlayerfield());
            drawGameRules(PLAYING_FIELD_X + height, PLAYING_FIELD_Y);
            break;
        }
        case GameState::LOST:
            drawGameLost();
            tui.drawGame(game.getPlayerfield());
            break;
        case GameState::WON:
            drawGameWon();
            tui.drawGame(game.getPlayerfield());
            break;
        default:
            break;
    }
}

int main() {
    setup_terminal();

    std::vector<std::unique_ptr<Button>> buttons;
    buttons.push_back(std::make_unique<LabelButton>(6, 3, 14, "Start", "\x1b[48;5;24m\x1b[97m"));
    const int start_id = buttons.back()->getID();
    buttons.push_back(std::make_unique<LabelButton>(6, 35, 14, "Quit", "\x1b[48;5;52m\x1b[97m"));
    const int quit_id = buttons.back()->getID();
    draw_all(buttons);

    bool running = true;
    // One parser for the whole run: a sequence split across two reads must continue.
    SgrParser parser;

    while (running) {
        std::uint8_t byte = 0;
        const ssize_t n = ::read(STDIN_FILENO, &byte, 1);
        if (n <= 0)
            break;

        const auto event = parser.feed(byte);
        if (!event)
            continue;
        if (const auto* key = std::get_if<KeyEvent>(&*event)) {
            handle_key(key->byte, running);
        } else if (const auto* mouse = std::get_if<MouseEvent>(&*event)) {
            handle_mouse(*mouse, buttons, start_id, quit_id, running);
        }
    }

    restore_terminal();
    return 0;
}

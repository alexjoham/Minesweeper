#include <termios.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <csignal>
#include <memory>
#include <vector>
#include <algorithm>

#include "enums/game_state.hpp"
#include "game/game.hpp"
#include "tui/tui.hpp"
#include "button/button.hpp"

constexpr int PLAYING_FIELD_X = 2;
constexpr int PLAYING_FIELD_Y = 2;

static termios term;
static bool g_raw_active = false;
GameState state = GameState::MENU;
Game game;
Tui tui = Tui();
std::vector<std::unique_ptr<Button>> game_buttons;
static bool set_flag = false;
UnicodeButton flag_button = UnicodeButton(1, 3, 3, "\u2691", "\x1b[93m");


static void restore_terminal() {
    if (!g_raw_active) return;
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
    if(tcgetattr(STDIN_FILENO, &term) == -1) {
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

    emit_terminal_command("\x1b[?1049h" // alternative screen bugger
                          "\x1b[?25l"   // hide the cursor
                          "\x1b[?1000h" // mouse reporting
                          "\x1b[?1002h" // drag reporting
                          "\x1b[?1006h"); // enable SGR mouse mode

    std::atexit(restore_terminal);
    std::signal(SIGTERM, on_signal);
    std::signal(SIGHUP, on_signal);
}

static void draw_all(std::vector<std::unique_ptr<Button>> &buttons) {
    clear_screen();
    emit_terminal_command(at(3, 3) + "\x1b[2mWelcome to Minesweeper. Press q to quit.\x1b[0m");
    for (const auto &b : buttons) b->draw();
}

static void init_game() {
    game.startGame();
    for (size_t i = 1; i <= game.getFieldSize(); i++) {
        for (size_t j = 0; j < game.getFieldSize(); j++) {
            auto b = std::make_unique<UnicodeButton>(PLAYING_FIELD_X+i, PLAYING_FIELD_Y + j*3 + 1, 3, "\u25A2", "\x1b[97m");
            game_buttons.push_back(std::move(b));
        }
    }
}

static void drawGameInfo() {
    flag_button.draw();
    emit_terminal_command(at(0, 7) + "\x1b[2mPress q to quit.\x1b[0m");
}

static void drawGameLost() {
    emit_terminal_command(at(0, 0) + "\x1b[91mGAME LOST\x1b[0m\x1b[2m. Press \x1b[0m\x1b[1mr\x1b[0m\x1b[2m to retry.\x1b[0m");
}

static void drawGameWon() {
    emit_terminal_command(at(0, 0) + "\x1b[92mGAME WON!\x1b[0m\x1b[2m. Press \x1b[0m\x1b[1mr\x1b[0m\x1b[2m to replay.\x1b[0m");
}

static void drawGameRules(int x, int y) {
    emit_terminal_command(at(x, y) + "\x1b[2mLeft-click to open a square.\x1b[0m");
    emit_terminal_command(at(x+1, y) + "\x1b[2mSelect the flag to place a flag with left click where you think a mine is.\x1b[0m");
    emit_terminal_command(at(x+2, y) + "\x1b[2mThe numbers show you how many mines are around this square (vertically, horizontally and diagonally.\x1b[0m");
    
}

int main() {
    setup_terminal();

    std::vector<std::unique_ptr<Button>> buttons;
    buttons.push_back(std::make_unique<LabelButton>(6, 3, 14, "Start", "\x1b[48;5;24m\x1b[97m"));
    const int start_id = buttons.back()->getID();
    buttons.push_back(std::make_unique<LabelButton>(6, 35, 14, "Quit",  "\x1b[48;5;52m\x1b[97m"));
    const int guit_id = buttons.back()->getID();
    draw_all(buttons);

    bool running = true;
    std::string buf;

    while(running) {
        char c;
        ssize_t n = ::read(STDIN_FILENO, &c, 1);
        if (n <= 0) break;
        buf.push_back(c);

        if (buf.size() == 1 && c != '\x1b') {
            if (c == 'q' || c == 3 /* Ctrl+C */) running = false;
            if (c == 'r') { state = GameState::GAME; init_game(); clear_screen(); buttons = std::move(game_buttons); drawGameInfo(); tui.drawGame(PLAYING_FIELD_X, PLAYING_FIELD_Y, game.getPlayerfield()); continue; }
            buf.clear();
            continue;
        }

        if ((buf.size() == 2 && c != '[') || (buf.size() == 3 && c != '<')) {
            buf.clear();
            continue;
        }
        if (buf.size() < 4) continue;

        if (c == 'M' || c == 'm') {
            int btn = 0, mx = 0, my = 0;
            bool parsed = std::sscanf(buf.c_str(), "\x1b[<%d;%d;%d", &btn, &my, &mx) == 3;
            buf.clear();
            if (parsed) {
                bool press   = (c == 'M');
                bool motion  = (btn & 0x20) != 0;   // drag, not a fresh click
                bool wheel   = (btn & 0x40) != 0;
                bool is_left = (btn & 0x03) == 0;

                if (!motion && !wheel && is_left) {
                    if (press) {
                        for (auto &b : buttons) b->press(mx, my);
                        flag_button.press(mx, my);
                    } else {
                        switch (state) {
                            case GameState::MENU: {
                                bool switch_to_game = false;
                                bool quit_clicked = false;
                                for (auto &b : buttons) {
                                    if (b->release(mx, my)) {
                                        if (b->getID() == start_id) { switch_to_game = true; }
                                        else if (b->getID() == guit_id) { quit_clicked = true; }
                                    }
                                }
                                if (quit_clicked) { running = false; buf.clear(); continue; }
                                if (switch_to_game) {
                                    state = GameState::GAME; 
                                    init_game(); 
                                    clear_screen(); 
                                    buttons = std::move(game_buttons); 
                                    drawGameInfo(); 
                                    int height = tui.drawGame(PLAYING_FIELD_X, PLAYING_FIELD_Y, game.getPlayerfield());
                                    drawGameRules(PLAYING_FIELD_X+height, PLAYING_FIELD_Y); continue;
                                }
                                draw_all(buttons);
                                break;
                            }
                            case GameState::GAME: {
                                if (flag_button.release(mx, my)) {
                                    set_flag = !set_flag;
                                    if (set_flag) {
                                        flag_button.setColor("\x1b[7m");
                                    } else {
                                        flag_button.setColor("\x1b[93m");
                                    }
                                    drawGameInfo();
                                    tui.drawGame(PLAYING_FIELD_X, PLAYING_FIELD_Y, game.getPlayerfield());
                                    continue;
                                }
                                bool game_lost = false;
                                for (auto &b : buttons) {
                                    if (b->release(mx, my)) {
                                        if (!set_flag) {
                                            std::vector<RevealedCell> result = game.makeMove(static_cast<size_t>(b->getX()), static_cast<size_t>(b->getY()));
                                            if(result.size() > 0) {
                                                if (result.size() == 1 && result.front().cell.fieldType == FieldType::MINE) {
                                                    result = game.revealAll();
                                                    game_lost = true;
                                                }
                                                for (auto &resultField: result) {
                                                    size_t pos = resultField.row * static_cast<size_t>(game.getFieldSize()) + resultField.column;
                                                    auto &button = buttons.at(pos);
                                                    UnicodeButton* ub = dynamic_cast<UnicodeButton*>(button.get());
                                                    if (ub) {
                                                        switch (resultField.cell.fieldType) {
                                                            case FieldType::MINE:        ub->setCode("★"); ub->setColor("\x1b[91m"); break;
                                                            case FieldType::NEUTRAL:     ub->setCode(" ");      ub->setColor("\x1b[97m"); break;
                                                            case FieldType::ONE:         ub->setCode("1");      ub->setColor("\x1b[94m"); break;
                                                            case FieldType::TWO:         ub->setCode("2");      ub->setColor("\x1b[92m"); break;
                                                            case FieldType::THREE:       ub->setCode("3");      ub->setColor("\x1b[91m"); break;
                                                            case FieldType::FOUR:        ub->setCode("4");      ub->setColor("\x1b[95m"); break;
                                                            case FieldType::FIVE:        ub->setCode("5");      ub->setColor("\x1b[93m"); break;
                                                            default: break;
                                                        }
                                                        ub->activated = false;
                                                    }
                                                }
                                            }
                                        } else {
                                            std::optional<Playerield> field = game.toggleFlag(static_cast<size_t>(b->getX()), static_cast<size_t>(b->getY()));
                                            if (field) {
                                                UnicodeButton* ub = dynamic_cast<UnicodeButton*>(b.get());
                                                if (ub) {
                                                    if (field->flagged) {
                                                        ub->setCode("\u2691");
                                                        ub->setColor("\x1b[93m");
                                                    } else {
                                                        ub->setCode("\u25A2");
                                                        ub->setColor("\x1b[97m");
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                if (game_lost) {
                                    state = GameState::LOST;
                                    drawGameLost();
                                    tui.drawGame(PLAYING_FIELD_X, PLAYING_FIELD_Y, game.getPlayerfield());
                                    continue;
                                } else if (game.game_won()) {
                                    game.revealAll();
                                    drawGameWon();
                                    tui.drawGame(PLAYING_FIELD_X, PLAYING_FIELD_Y, game.getPlayerfield());
                                    state = GameState::WON;
                                    continue;
                                }
                                drawGameInfo();
                                int height = tui.drawGame(PLAYING_FIELD_X, PLAYING_FIELD_Y, game.getPlayerfield());
                                drawGameRules(PLAYING_FIELD_X+height, PLAYING_FIELD_Y);
                                break;
                            }
                            case GameState::LOST:
                                drawGameLost();
                                tui.drawGame(PLAYING_FIELD_X, PLAYING_FIELD_Y, game.getPlayerfield());
                                break;
                            case GameState::WON:
                                drawGameWon();
                                tui.drawGame(PLAYING_FIELD_X, PLAYING_FIELD_Y, game.getPlayerfield());
                                break;
                            default:
                            break;
                        }
                    }
                }
            }
        }

        if (buf.size() > 32) buf.clear(); // give up on a malformed sequence
    }

    restore_terminal();
    return 0;
}
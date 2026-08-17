#include <termios.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <csignal>
#include <vector>

#include "enums/game_state.hpp"
#include "game/game.hpp"


static termios term;
static bool g_raw_active = false;
GameState state = GameState::MENU;
Game game;
std::vector<std::unique_ptr<Button>> game_buttons;
static bool set_flag = false;
UnicodeButton flag_button = UnicodeButton(3, 1, 3, "\u2691", "\x1b[93m");

static int PLAYING_FIELD_X = 2;
static int PLAYING_FIELD_Y = 2;

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
    std::vector<int> buttonIDs;
    for (int i = 1; i <= game.FIELD_SIZE; i++) {
        for (int j = 0; j < game.FIELD_SIZE; j++) {
            UnicodeButton b = UnicodeButton(10+i*3, 5+j, 3, "\u25A2", "\x1b[97m");
            game_buttons.push_back(std::make_unique<UnicodeButton>(b));
            buttonIDs.push_back(b.getID());
        }
    }
    game.setPlayerfield(buttonIDs);
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
    LabelButton start = LabelButton(3,  6, 14, "Start", "\x1b[48;5;24m\x1b[97m");
    LabelButton quit = LabelButton(35, 6, 14, "Quit",  "\x1b[48;5;52m\x1b[97m");
    buttons.push_back(std::make_unique<LabelButton>(start));
    buttons.push_back(std::make_unique<LabelButton>(quit));
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
            if (c == 'r') { state = GameState::GAME; init_game(); clear_screen(); buttons = std::move(game_buttons); drawGameInfo(); game.drawGame(PLAYING_FIELD_X, PLAYING_FIELD_Y, buttons); continue; }
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
            bool parsed = std::sscanf(buf.c_str(), "\x1b[<%d;%d;%d", &btn, &mx, &my) == 3;
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
                                        if (*b == start) { switch_to_game = true; }
                                        else if (*b == quit) { quit_clicked = true; }
                                    }
                                }
                                if (quit_clicked) { running = false; buf.clear(); continue; }
                                if (switch_to_game) {
                                    state = GameState::GAME; 
                                    init_game(); 
                                    clear_screen(); 
                                    buttons = std::move(game_buttons); 
                                    drawGameInfo(); 
                                    int height = game.drawGame(PLAYING_FIELD_X, PLAYING_FIELD_Y, buttons);
                                    drawGameRules(PLAYING_FIELD_X+height, PLAYING_FIELD_Y); continue;
                                }
                                draw_all(buttons);
                                break;
                            }
                            case GameState::GAME: {
                                if (flag_button.release(my, my)) {
                                    set_flag = !set_flag;
                                    if (set_flag) {
                                        flag_button.color = "\x1b[7m";
                                    } else {
                                        flag_button.color = "\x1b[93m";
                                    }
                                    drawGameInfo();
                                    game.drawGame(PLAYING_FIELD_X, PLAYING_FIELD_Y, buttons);
                                    continue;
                                }
                                bool game_lost = false;
                                bool game_won = false;
                                for (auto &b : buttons) {
                                    if (b->release(mx, my)) {
                                        if (!set_flag) {
                                            std::vector<Playerield> result = game.makeMove(b->getID());
                                            if(result.size() > 0) {
                                                if (result.size() == 1 && result.front().fieldType == FieldType::MINE) {
                                                    result = game.revealAll();
                                                    game_lost = true;
                                                }
                                                for (auto &resultField: result) {
                                                    int buttonID = resultField.buttonID;
                                                    auto button = std::find_if(buttons.begin(), buttons.end(), [buttonID](const std::unique_ptr<Button>& i) { return i->getID() == buttonID; });
                                                    if (button != buttons.end()) {
                                                        UnicodeButton* ub = dynamic_cast<UnicodeButton*>(button->get());
                                                        if (ub) {
                                                            switch (resultField.fieldType) {
                                                                case FieldType::MINE:        ub->code = "★"; ub->color = "\x1b[91m"; break;
                                                                case FieldType::NEUTRAL:     ub->code = " ";      ub->color = "\x1b[97m"; break;
                                                                case FieldType::ONE:         ub->code = "1";      ub->color = "\x1b[94m"; break;
                                                                case FieldType::TWO:         ub->code = "2";      ub->color = "\x1b[92m"; break;
                                                                case FieldType::THREE:       ub->code = "3";      ub->color = "\x1b[91m"; break;
                                                                case FieldType::FOUR:        ub->code = "4";      ub->color = "\x1b[95m"; break;
                                                                case FieldType::FIVE:        ub->code = "5";      ub->color = "\x1b[93m"; break;
                                                                default: break;
                                                            }
                                                            ub->activated = false;
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            Playerield field = game.toggleFlag(b->getID());
                                            if (field.buttonID != -1) {
                                                UnicodeButton* ub = dynamic_cast<UnicodeButton*>(b.get());
                                                if (ub) {
                                                    if (field.flagged) {
                                                        ub->code = "\u2691";
                                                        ub->color = "\x1b[93m";
                                                    } else {
                                                        ub->code = "\u25A2";
                                                        ub->color = "\x1b[97m";
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                if (game_lost) {
                                    state = GameState::LOST;
                                    drawGameLost();
                                    game.drawGame(PLAYING_FIELD_X, PLAYING_FIELD_Y, buttons);
                                    continue;
                                } else if (game.game_won()) {
                                    game.revealAll();
                                    drawGameWon();
                                    game.drawGame(PLAYING_FIELD_X, PLAYING_FIELD_Y, buttons);
                                    state = GameState::WON;
                                    continue;
                                }
                                drawGameInfo();
                                int height = game.drawGame(PLAYING_FIELD_X, PLAYING_FIELD_Y, buttons);
                                drawGameRules(PLAYING_FIELD_X+height, PLAYING_FIELD_Y);
                                break;
                            }
                            case GameState::LOST:
                                drawGameLost();
                                game.drawGame(PLAYING_FIELD_X, PLAYING_FIELD_Y, buttons);
                                break;
                            case GameState::WON:
                                drawGameWon();
                                game.drawGame(PLAYING_FIELD_X, PLAYING_FIELD_Y, buttons);
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
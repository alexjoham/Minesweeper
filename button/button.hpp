#include <string>
#include "../helpers.hpp"

class Button {
    protected:
        static int ID;
    private:
        int id;
    public:
        int x, y, w;
        std::string color;

        bool pressed = false;
        bool activated = true;

        virtual void draw() {};

        int getID() {
            return id;
        }

        bool contains(int cx, int cy) const {
            return cy == y && cx >= x && cx < x + w;
        }

        void press(int mx, int my) {
            if (activated) {
                pressed = contains(mx, my);
            }
        }

        bool release(int mx, int my) {
            if (activated && pressed && contains(mx, my)) {
                pressed = false;
                return true;
            }
            return false;
        }

        Button(int x, int y, int w, std::string color) : x(x), y(y), w(w), color(color) {
            id = ++ID;
        }

        bool operator==(const Button& rhs) { return (id == rhs.id); }
        bool operator!=(const Button& rhs) { return !operator==(rhs); }

        virtual ~Button() = default;
};

class LabelButton : public Button {
    public:
        std::string label;
        LabelButton(int x, int y, int w, std::string label, std::string color) : Button(x, y, w, color), label(label) {}
        void draw() {
            if ((int)label.size() > w - 2) label.resize(w - 2);
            int pad = w - 2 - (int)label.size();
            int left = pad / 2;
            int right = pad - left;
            std::string face = " ";
            face += std::string(left, ' ') + label + std::string(right, ' ');
            face += " ";
            std::string style = pressed ? "\x1b[7m" : color;  // reverse color when held
            emit_terminal_command(at(y, x) + style + face + "\x1b[0m"); //TODO: Switch x and y
        }
};

class UnicodeButton : public Button {
    public:
        std::string code;
        UnicodeButton(int x, int y, int w, std::string code, std::string color) : Button(x, y, w, color), code(code) {}
        void draw() {
            std::string face = std::string(1, ' ') + code + std::string(1, ' ');
            std::string style = pressed ? "\x1b[7m" : color;  // reverse color when held
            emit_terminal_command(at(y, x) + style + face + "\x1b[0m"); //TODO: Switch x and y
        }
};
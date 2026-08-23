#pragma once
#include <string>
#include <utility>
#include "../helpers.hpp"

class Button {
    protected:
        static int ID;
        int x_, y_, w_;
        std::string color_;
    private:
        int id;
    public:

        bool pressed = false;
        bool activated = true;

        virtual void draw() const = 0;

        int getID() const {
            return id;
        }

        bool contains(int cx, int cy) const {
            return cy == y_ && cx >= x_ && cx < x_ + w_;
        }

        void setX(int new_x) {
            x_ = new_x;
        }

        void setY(int new_y) {
            y_ = new_y;
        }

        void setColor(std::string new_color) {
            color_ = std::move(new_color);
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

        Button(int x, int y, int w, std::string color) : x_(x), y_(y), w_(w), color_(std::move(color)) {
            id = ++ID;
        }

        bool operator==(const Button& rhs) const { return (id == rhs.id); }
        bool operator!=(const Button& rhs) const { return !operator==(rhs); }

        virtual ~Button() = default;
};

class LabelButton : public Button {
    private:
        std::string label_;
    public:
        LabelButton(int x, int y, int w, std::string label, std::string color) : Button(x, y, w, color), label_(std::move(label)) {}        
        void draw() const override {
            const std::size_t visible = static_cast<std::size_t>(std::max(0, w_ - 2));
            const std::string_view text = std::string_view(label_).substr(0, visible);
            const std::size_t pad   = visible - text.size();
            const std::size_t left  = pad / 2;
            const std::size_t right = pad - left;
            std::string face = " ";
            face += std::string(left, ' ');
            face += text;
            face += std::string(right, ' ');
            face += " ";
            std::string style = pressed ? "\x1b[7m" : color_;  // reverse color when held
            emit_terminal_command(at(y_, x_) + style + face + "\x1b[0m"); //TODO: Switch x and y
        }
};

class UnicodeButton : public Button {
    private:
        std::string code_;
    public:
        UnicodeButton(int x, int y, int w, std::string code, std::string color) : Button(x, y, w, color), code_(std::move(code)) {}
        void draw() const override {
            std::string face = std::string(1, ' ') + code_ + std::string(1, ' ');
            std::string style = pressed ? "\x1b[7m" : color_;  // reverse color when held
            emit_terminal_command(at(y_, x_) + style + face + "\x1b[0m"); //TODO: Switch x and y
        }

        void setCode(std::string new_code) {
            code_ = std::move(new_code);
        }
};
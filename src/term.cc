#include "term.hh"

#include "ctrl.hh"

#include <fmt/format.h>
#include <stack>

namespace {
termios saved;
[[gnu::constructor]] void save() { saved = sh::term::mode(); }
[[gnu::destructor]] void restore() { sh::term::set_mode(saved); }

std::string prompt_string;
std::string line;
size_t prompt_size;
sh::term::cursor::lcur cur;
} // namespace

termios sh::term::mode() {
    termios trm{};
    tcgetattr(STDIN_FILENO, &trm);
    return trm;
}

void sh::term::set_mode(const termios& termios) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios);
}

void sh::term::set_raw() {
    termios trm = mode();
    cfmakeraw(&trm);
    trm.c_cc[VMIN] = 0;
    trm.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &trm);
}

void sh::term::clear_to_end() {
    write("\033[0K");
}

void sh::term::delete_left() {
    if (cur == cursor::lcur::start) return;
    auto raw = size_t(cur);

    /// Erase more than two chars if its a control char.
    /// TODO: multi-byte utf-8 chars.
    if (std::iscntrl(line[raw - 1])) line.erase(raw - 2, 2);
    else line.erase(raw - 1, 1);

    redraw();

    /// Move the cursor back.
    move_left();
}

void sh::term::delete_right() {
    auto raw = size_t(cur);
    if (raw == line.size()) return;

    /// Erase more than two chars if its a control char.
    /// TODO: multi-byte utf-8 chars.
    if (std::iscntrl(line[raw])) line.erase(raw, 2);
    else line.erase(raw, 1);

    redraw();
}

void sh::term::move_left() {
    if (cur == cursor::lcur::start) return;
    auto raw = size_t(cur);
    cur = cursor::lcur(raw - 1);
    write("\033[1D");
}

void sh::term::move_right() {
    if (size_t(cur) == line.size()) return;
    auto raw = size_t(cur);
    cur = cursor::lcur(raw + 1);
    write("\033[1C");
}

void sh::term::new_line() {
    write("\r");
    write(prompt_string);
    cur = cursor::lcur::start;
    line.clear();
}

char sh::term::readc() {
    char c;
    auto n = read(STDIN_FILENO, &c, 1);
    if (n == -1) throw std::runtime_error("read() failed");
    if (n == 0) return -1;

    /// Handle special characters.
    switch (c) {
            /// Ctrl+C.
        case CTRL('C'):
            write("^C\n");
            new_line();
            return -1;

        /// Ctrl+D.
        case CTRL('D'):
            write("\r\n");
            sh::exit();

        /// Escape sequences.
        case '\033':
            n = read(STDIN_FILENO, &c, 1);
            if (n == -1) throw std::runtime_error("read() failed");
            if (n == 0) return -1;

            switch (c) {
                    /// Arrow keys.
                case '[': {
                    n = read(STDIN_FILENO, &c, 1);
                    if (n == -1) throw std::runtime_error("read() failed");
                    if (n == 0) return -1;

                    switch (c) {
                        /// Up arrow.
                        case 'A':
                            return -1;
                        /// Down arrow.
                        case 'B':
                            return -1;
                        /// Right arrow.
                        case 'C':
                            move_right();
                            return -1;
                        /// Left arrow.
                        case 'D':
                            move_left();
                            return -1;

                        /// Delete.
                        case '3':
                            n = read(STDIN_FILENO, &c, 1);
                            if (n == -1) throw std::runtime_error("read() failed");
                            if (n == 0) return -1;
                            if (c == '~') delete_right();
                            else write(fmt::format("^033[3{}", c));
                            return -1;

                        default:
                            write("^033[");
                            write(c);
                            return -1;
                    }
                }

                default:
                    write("^033");
                    write(c);
                    return -1;
            }

        /// Backspace.
        case 0x7f:
            delete_left();
            return -1;
    }

    cur = cursor::lcur(size_t(cur) + 1);
    line += c;
    return c;
}

void sh::term::redraw() {
    write("\r");
    clear_to_end();
    write(prompt_string);
    write(line);
}

void sh::term::set_prompt(std::string_view prompt) {
    prompt_string = prompt;
    prompt_size = prompt.size();
}

std::string_view sh::term::text() { return line; }

void sh::term::write(char c) { ::write(STDOUT_FILENO, &c, 1); }
void sh::term::write(std::string_view str) { ::write(STDOUT_FILENO, str.data(), str.size()); }

void sh::term::cursor::up(size_t n) { write(fmt::format("\033[{}A", n)); }
void sh::term::cursor::down(size_t n) { write(fmt::format("\033[{}B", n)); }
void sh::term::cursor::right(size_t n) { write(fmt::format("\033[{}C", n)); }
void sh::term::cursor::left(size_t n) { write(fmt::format("\033[{}D", n)); }

auto sh::term::cursor::save() -> pos {
    return pos{size_t(cur), 0};
}

void sh::term::cursor::restore(pos pos) {
    cur = lcur(pos.x);
    to(lcur(pos.x));
}

void sh::term::cursor::to(lcur n) {
    write(fmt::format("\033[{}G", size_t(n) + prompt_size + 1));
}

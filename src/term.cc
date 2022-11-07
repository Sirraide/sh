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
}

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
    cur = cursor::lcur(line.size());
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

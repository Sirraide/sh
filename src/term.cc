#include "term.hh"

#include "cmd.hh"
#include "ctrl.hh"

#include <filesystem>
#include <fmt/format.h>
#include <stack>

namespace {
termios saved;
[[gnu::constructor]] void save() { saved = sh::term::mode(); }
[[gnu::destructor]] void restore() { sh::term::set_mode(saved); }

std::string prompt_string_template;
std::string git_prompt_template;
std::string line;
std::string saved_prompt;
size_t prompt_size;
sh::term::cursor::lcur cur;
bool line_continued = false;

std::string prompt() {
    /// Get the current path.
    auto path = std::filesystem::current_path().string();
    auto home = std::getenv("HOME");
    if (path.starts_with(home)) path.replace(0, std::strlen(home), "~");

    /// Get the current git branch.
    auto [status, branch] = sh::cmd::popen("git rev-parse --abbrev-ref HEAD", true);

    /// Format the prompt.
    std::string str;
    if (status == 0) {
        /// Remove the newline.
        branch.pop_back();

        /// Check if the branch is dirty.
        auto [st, output] = sh::cmd::popen("git status --porcelain", true);
        auto dirty = st == 0 && !output.empty();

        /// Format the prompt.
        str = fmt::vformat(git_prompt_template,
            fmt::make_format_args(path,
                dirty ? "\033[1;31m" : "\033[1;32m",
                branch,
                sh::last_exit_code == 0 ? "\033[32m" : "\033[31m",
                sh::last_exit_code));
    } else {
        str = fmt::vformat(prompt_string_template,
            fmt::make_format_args(path,
                sh::last_exit_code == 0 ? "\033[32m" : "\033[31m",
                sh::last_exit_code));
    }

    /// Strip color codes.
    prompt_size = 0;
    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] == '\033')
            while (str[i] != 'm') i++;
        else prompt_size++;
    }

    return str;
}

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

void sh::term::reset() { set_mode(saved); }

void sh::term::clear_line_and_prompt() {
    write("\r");
    write(prompt());
    cur = cursor::lcur::start;
    line.clear();
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

    /// Adjust the logical cursor.
    cur = cursor::lcur(raw - 1);

    redraw();
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

void sh::term::echo(std::string_view str) {
    /// Insert text at the current cursor position.
    auto raw = size_t(cur);
    line.insert(raw, str);

    /// Move the cursor forward.
    cur = cursor::lcur(raw + str.size());

    /// Redraw the line.
    redraw();
}

void sh::term::echo(char c) { echo(std::string_view(&c, 1)); }

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

void sh::term::new_line() { write("\r\n"); }

std::string sh::term::read_line() {
    /// Save the current prompt.
    saved_prompt = prompt();

    while (not sh::term::readc())
        ;
    write("\r");
    auto ret = line;
    line.clear();
    return ret;
}

bool sh::term::readc() {
    char c;
    auto n = read(STDIN_FILENO, &c, 1);
    if (n == -1) throw std::runtime_error("read() failed");
    if (n == 0) return false;

    /// Handle special characters.
    switch (c) {
        /// Ctrl+C.
        case CTRL('C'):
            new_line();
            clear_line_and_prompt();
            return false;

        /// Ctrl+D.
        case CTRL('D'):
            write("\r\n");
            sh::exit();

        /// Enter.
        case '\r':
        case '\n':
            /// Continue the line if the last char is a backslash.
            if (not line.empty() and line.back() == '\\') {
                line.pop_back();
                cur = cursor::lcur(line.size());
                line_continued = true;
                new_line();
                return false;
            }

            /// Otherwise, return the line.
            line_continued = false;
            new_line();
            return true;

        /// Escape sequences.
        case '\033':
            n = read(STDIN_FILENO, &c, 1);
            if (n == -1) throw std::runtime_error("read() failed");
            if (n == 0) return false;

            switch (c) {
                case '[': {
                    n = read(STDIN_FILENO, &c, 1);
                    if (n == -1) throw std::runtime_error("read() failed");
                    if (n == 0) return false;

                    switch (c) {
                        /// Up arrow.
                        case 'A':
                            return false;
                        /// Down arrow.
                        case 'B':
                            return false;
                        /// Right arrow.
                        case 'C':
                            move_right();
                            return false;
                        /// Left arrow.
                        case 'D':
                            move_left();
                            return false;

                        /// Insert.
                        case '2':
                            n = read(STDIN_FILENO, &c, 1);
                            if (n == -1) throw std::runtime_error("read() failed");
                            if (n == 0) return false;
                            if (c != '~') echo(fmt::format("033[2{}", c));
                            return false;

                        /// Delete.
                        case '3':
                            n = read(STDIN_FILENO, &c, 1);
                            if (n == -1) throw std::runtime_error("read() failed");
                            if (n == 0) return false;
                            if (c == '~') delete_right();
                            else echo(fmt::format("033[3{}", c));
                            return false;

                        /// Home.
                        case 'H':
                            lmove_to(cursor::lcur::start);
                            return false;

                        /// End.
                        case 'F':
                            lmove_to(cursor::lcur(line.size()));
                            return false;

                        default:
                            echo("033[");
                            echo(c);
                            return false;
                    }
                }

                default:
                    echo("033");
                    echo(c);
                    return false;
            }

        /// Backspace.
        case 0x7f:
            delete_left();
            return false;
    }

    if (std::iscntrl(c)) {
        echo('^');
        echo(char(c + '@'));
        return false;
    }

    echo(c);
    return false;
}

void sh::term::redraw() {
    write("\r");
    clear_to_end();
    if (line_continued) write("...>");
    else write(saved_prompt);
    write(line);
    to(cur);
}

void sh::term::set_prompt(std::string_view prompt, std::string_view git_prompt) {
    prompt_string_template = prompt;
    git_prompt_template = git_prompt;
}

std::string_view sh::term::text() { return line; }

void sh::term::write(char c) { ::write(STDOUT_FILENO, &c, 1); }
void sh::term::write(std::string_view str) { ::write(STDOUT_FILENO, str.data(), str.size()); }

void sh::term::cursor::up(size_t n) { write(fmt::format("\033[{}A", n)); }
void sh::term::cursor::down(size_t n) { write(fmt::format("\033[{}B", n)); }
void sh::term::cursor::right(size_t n) { write(fmt::format("\033[{}C", n)); }
void sh::term::cursor::left(size_t n) { write(fmt::format("\033[{}D", n)); }

void sh::term::cursor::lmove_to(cursor::lcur pos) {
    auto raw = size_t(pos);
    auto cur_raw = size_t(cur);

    if (raw > cur_raw) right(raw - cur_raw);
    else if (raw < cur_raw) left(cur_raw - raw);

    cur = pos;
}

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

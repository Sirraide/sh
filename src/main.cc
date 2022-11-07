#include "cmd.hh"
#include "term.hh"

#include <filesystem>
#include <fmt/format.h>

int main() {
    sh::term::set_raw();

    auto path = std::filesystem::current_path().string();
    auto home = std::getenv("HOME");
    if (path.starts_with(home)) path.replace(0, std::strlen(home), "~");
    sh::term::set_prompt(fmt::format("\033[33m[sh++] \033[38;2;79;151;215m{} \033[1;38;2;79;151;215m$ \033[m", path));

    /*auto debug = [&] {
        auto pos = sh::term::cursor::save();
        sh::term::write(fmt::format("\033[50G\033[37m{}\033[m", sh::term::text()));
        sh::term::cursor::restore(pos);
    };*/

    /// Shell main loop.
    for (;;) {
        /// Print the prompt.
        sh::term::clear_line_and_prompt();

        /// Read a line.
        auto line = sh::term::read_line();
        sh::cmd::exec(line);
    }
}
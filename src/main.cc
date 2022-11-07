#include "term.hh"
#include <fmt/format.h>
#include <filesystem>

int main() {
    sh::term::set_raw();

    auto path = std::filesystem::current_path().string();
    auto home = std::getenv("HOME");
    if (path.starts_with(home)) path.replace(0, std::strlen(home), "~");
    sh::term::set_prompt(fmt::format("\033[33msh++ \033[38;2;79;151;215m{} \033[1;38;2;79;151;215m$ \033[m", path));

    auto debug = [&] {
        auto pos = sh::term::cursor::save();
        sh::term::write(fmt::format("\033[50G\033[37m{}\033[m", sh::term::text()));
        sh::term::cursor::restore(pos);
    };

    /// Shell main loop.
    sh::term::clear_line_and_prompt();
    for (;;) {
        auto c = sh::term::readc();

        /// Write the current line in grey at offset 50 for debugging.
        if (c == -1) {
            debug();
            continue;
        }

        debug();
    }
}
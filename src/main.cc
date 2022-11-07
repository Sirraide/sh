#include "term.hh"
#include <fmt/format.h>

int main() {
    sh::term::set_raw();
    sh::term::set_prompt("sh++> ");

    auto debug = [&] {
        auto pos = sh::term::cursor::save();
        sh::term::write(fmt::format("\033[50G\033[37m{}\033[m", sh::term::text()));
        sh::term::cursor::restore(pos);
    };

    /// Shell main loop.
    sh::term::new_line();
    for (;;) {
        auto c = sh::term::readc();

        /// Write the current line in grey at offset 50 for debugging.
        if (c == -1) {
            debug();
            continue;
        }

        switch (c) {
            default:
                if (std::iscntrl(c)) sh::term::write(fmt::format("^{}", c + '@'));
                else sh::term::write(c);
                break;
        }

        debug();
    }
}
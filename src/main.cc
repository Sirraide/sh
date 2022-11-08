#include "cmd.hh"
#include "ctrl.hh"
#include "term.hh"

#include <filesystem>
#include <fmt/format.h>

int main() {
    sh::term::set_raw();
    sh::term::set_prompt("\033[33m[sh++] \033[38;2;79;151;215m{} {}{} \033[1;38;2;79;151;215m$ \033[m",
                         "\033[33m[sh++] \033[38;2;79;151;215m{}{} @ \033[m\033[34m{}\033[38;2;79;151;215m {}{} \033[1;38;2;79;151;215m$ \033[m");

    /// Shell main loop.
    for (;;) {
        /// Print the prompt.
        sh::term::clear_line_and_prompt();

        /// Read a line.
        auto line = sh::term::read_line();
        sh::last_exit_code = sh::cmd::exec(line);
    }
}
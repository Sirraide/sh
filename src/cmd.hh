#ifndef SH_CMD_HH
#define SH_CMD_HH

#include <string>
#include <string_view>
#include <vector>

namespace sh::cmd {
/// Shell command token.
struct token {
    std::string str;
};

/// Shell command token list.
using toks = std::vector<token>;

/// Execute a command.
int exec(std::string_view cmd);

/// Parse a shell command.
toks parse(std::string_view cmd);

/// Execute a command and get its output.
std::pair<int, std::string> popen(std::string_view cmd, bool ignore_stderr = false);

}

#endif // SH_CMD_HH

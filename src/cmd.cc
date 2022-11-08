#include "cmd.hh"

#include "ctrl.hh"
#include "term.hh"
#include "utils.hh"

#include <fcntl.h>
#include <fmt/format.h>
#include <stdexcept>
#include <sys/wait.h>

namespace {
int wait_for_child(pid_t pid) {
    int status;
    do {
        if (waitpid(pid, &status, 0) == -1) throw std::runtime_error("waitpid failed");
    } while (not WIFEXITED(status) and not WIFSIGNALED(status));

    if (WIFSIGNALED(status)) {
        if (WTERMSIG(status) == SIGSEGV) {
            fmt::print(stderr, "Segmentation fault (core dumped)\r\033[1A");
        } else {
            fmt::print(stderr, "Terminated by signal {}\r\033[1A", WTERMSIG(status));
        }
    }

    return WEXITSTATUS(status);
}
}

int sh::cmd::exec(std::string_view cmd) {
    if (cmd.empty()) return 0;
    if (cmd == "exit") sh::exit();

    auto toks = parse(cmd);
    if (toks.empty()) return 0;

    /// Reset the terminal
    sh::term::reset();
    defer { sh::term::set_raw(); };

    /// If the command doesn’t exist, print an error.
    if (auto path = sh::utils::which(toks[0].str); path.empty()) {
        fmt::print(stderr, "sh++: command not found: {}\n", toks[0].str);
        return 127;
    }

    /// Run the command.
    auto pid = fork();
    if (pid == -1) throw std::runtime_error("fork failed");

    /// Child process.
    if (pid == 0) {
        std::vector<char*> args;
        for (auto& tok : toks) args.push_back(tok.str.data());
        args.push_back(nullptr);
        execvp(args[0], args.data());
        fmt::print(stderr, "execvp failed: {}", args[0]);
        sh::exit(1);
    }

    /// Parent process.
    else return wait_for_child(pid);
}

auto sh::cmd::popen(std::string_view cmd, bool ignore_stderr) -> std::pair<int, std::string> {
    if (cmd.empty()) return {0, ""};
    if (cmd == "exit") sh::exit();

    auto toks = parse(cmd);
    if (toks.empty()) return {0, ""};

    /// Reset the terminal
    sh::term::reset();
    defer { sh::term::set_raw(); };

    /// If the command doesn’t exist, print an error.
    if (auto path = sh::utils::which(toks[0].str); path.empty()) {
        fmt::print(stderr, "sh++: command not found: {}\n", toks[0].str);
        return {127, ""};
    }

    /// Open a pipe to the command.
    int pipefd[2];
    if (pipe(pipefd) == -1) throw std::runtime_error("pipe failed");

    /// Run the command.
    auto pid = fork();
    if (pid == -1) throw std::runtime_error("fork failed");

    /// Child process.
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);

        if (ignore_stderr) {
            close(STDERR_FILENO);
            open("/dev/null", O_WRONLY);
        }

        std::vector<char*> args;
        for (auto& tok : toks) args.push_back(tok.str.data());
        args.push_back(nullptr);
        execvp(args[0], args.data());
        fmt::print(stderr, "execvp failed: {}", args[0]);
        sh::exit(1);
    }

    /// Parent process.
    else {
        close(pipefd[1]);
        std::string result;
        char buffer[128];

        while (true) {
            auto n = read(pipefd[0], buffer, 128);
            if (n == -1) throw std::runtime_error("read failed");
            if (n == 0) break;
            result.append(buffer, size_t(n));
        }

        return {wait_for_child(pid), result};
    }
}

auto sh::cmd::parse(std::string_view cmd) -> toks {
    const char* data = cmd.data();
    const char* const end = data + cmd.size();
    toks tokens;
    std::string curr;
    bool in_double_quotes = false;
    bool in_operator = false;

    for (; data < end; data++) {
        switch (*data) {
            /// A backslash escapes the next character.
            case '\\': {
                ++data;
                if (data == end) throw std::runtime_error("unexpected end of command");
                if (not in_double_quotes and *data != '\n') curr += *data;
                else {
                    switch (*data) {
                        case '\n': break;
                        case '$': curr += '$'; break;
                        case '`': curr += '`'; break;
                        case '"': curr += '"'; break;
                        case '\\': curr += '\\'; break;
                        default:
                            curr += '\\';
                            curr += *data;
                            break;
                    }
                }
                break;

                /// Single quotes quote everything.
                case '\'': {
                    if (in_double_quotes) goto append;
                    do {
                        ++data;
                        if (data < end) curr += *data;
                        else throw std::runtime_error("Unterminated single quote");
                    } while (*data != '\'');
                } break;

                /// Double quotes quote everything except backquotes,
                /// dollar signs, and double quotes.
                case '"': {
                    in_double_quotes = not in_double_quotes;
                } break;

                /// A backquote or dollar sign starts a substitution.
                case '`':
                case '$': {
                    /// TODO
                } break;

                /// A space or tab ends a token.
                case ' ':
                case '\t': {
                    if (in_double_quotes) goto append;
                    if (not curr.empty()) {
                        tokens.push_back({curr});
                        curr.clear();
                    }
                } break;

                /// A hash starts a comment.
                case '#': {
                    if (in_double_quotes) goto append;
                    return tokens;
                }

                /// Append the character.
                default:
                append : {
                    curr += *data;
                } break;
            }
        }
    }

    if (in_double_quotes) throw std::runtime_error("Unterminated double quote");
    if (not curr.empty()) tokens.push_back({curr});
    return tokens;
}
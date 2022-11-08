#include "utils.hh"

#include <algorithm>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

namespace sh::utils {
std::string popen(std::string_view cmd) {
    std::string result;
    FILE* pipe = ::popen(cmd.data(), "r");
    if (!pipe) {
        return result;
    }
    defer { ::pclose(pipe); };
    char buffer[128];
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != nullptr) {
            result += buffer;
        }
    }
    return result;
}

std::vector<std::string> split(std::string_view str, char delim) {
    std::vector<std::string> ret;
    std::string::size_type start = 0;
    std::string::size_type end = str.find(delim);
    while (end != std::string::npos) {
        ret.emplace_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find(delim, start);
    }
    ret.emplace_back(str.substr(start));
    return ret;
}

std::string which(std::string_view cmd) {
    /// Search for the command in the PATH.
    auto PATH = std::getenv("PATH");
    auto paths = sh::utils::split(PATH, ':');

    for (auto&& path : paths) {
        auto file = fs::path(path) / cmd;
        if (fs::exists(file)) return file;
    }

    return "";
}
}
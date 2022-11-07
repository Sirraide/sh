#ifndef SH_UTILS_HH
#define SH_UTILS_HH

#include <algorithm>
#include <string>
#include <vector>

#define CAT_(x, y) x##y
#define CAT(x, y)  CAT_(x, y)

template <typename callable>
struct $$defer_instance {
    callable f;
    explicit $$defer_instance(callable&& f) : f(std::move(f)) {}
    ~$$defer_instance() { f(); }
};

struct $$defer {
    template <typename callable>
    $$defer_instance<callable> operator%(callable&& f) {
        return $$defer_instance<callable>(std::forward<callable>(f));
    }
};

#define defer auto&& CAT($$defer_instance_, __COUNTER__) = $$defer() % [&]()

namespace sh::utils {
/// Split a string into a vector of strings.
std::vector<std::string> split(std::string_view str, char delim);

/// Get the real path of a command.
std::string which(std::string_view cmd);
} // namespace sh::utils

#endif // SH_UTILS_HH

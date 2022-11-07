#ifndef SH_UTILS_HH
#define SH_UTILS_HH

#include <algorithm>

#define CAT_(x, y) x##y
#define CAT(x, y) CAT_(x, y)

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

#endif // SH_UTILS_HH

#ifndef SH_CTRL_HH
#define SH_CTRL_HH

#include <cstdlib>

namespace sh {
[[noreturn]] void exit() { std::exit(0); }
}

#endif//SH_CTRL_HH

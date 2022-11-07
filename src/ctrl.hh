#ifndef SH_CTRL_HH
#define SH_CTRL_HH

#include <cstdlib>

namespace sh {
[[noreturn]] inline void exit(int code = 0) { std::exit(code); }
extern int last_exit_code = 0;
}

#endif//SH_CTRL_HH

#ifndef SH_TERM_HH
#define SH_TERM_HH

#include <termios.h>
#include <string_view>

/// ===========================================================================
///  sh::term — Terminal handling.
/// ===========================================================================
namespace sh::term {
/// ===========================================================================
///  Terminal settings.
/// ===========================================================================
/// Get terminal settings.
termios mode();

/// Restore terminal settings.
void set_mode(const termios& termios);

/// Set terminal to raw mode.
void set_raw();

/// ===========================================================================
///  Terminal I/O.
/// ===========================================================================
/// Clear from the cursor to the end of the line.
void clear_to_end();

/// Delete the character to the left of the cursor.
void delete_left();

/// Delete the character to the right of the cursor.
void delete_right();

/// Print the prompt.
void new_line();

/// Read a character from the terminal.
char readc();

/// Redraw the current line.
void redraw();

/// Set the terminal prompt.
void set_prompt(std::string_view prompt);

/// Get the current line.
std::string_view text();

/// Write to the terminal.
void write(char c);
void write(std::string_view str);

/// ===========================================================================
///  Terminal – Move cursor.
/// ===========================================================================
namespace cursor {
enum struct lcur : size_t { start = 0 };
enum struct phys : size_t {};
struct pos { size_t x, y; };

void up(size_t n = 1);
void down(size_t n = 1);
void right(size_t n = 1);
void left(size_t n = 1);

pos save();
void restore(pos p);

void to(lcur n);
} // namespace cursor
}

#endif//SH_TERM_HH

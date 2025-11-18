#ifndef DISPLAY_H
#define DISPLAY_H

#define DISPLAY_COLS 30
#define DISPLAY_ROWS 8

// Initialize the display
void display_init();

// Write a single character to the display
// Handles newlines, wrapping, and scrolling automatically
void display_write_char(char c);

// Write a string to the display
void display_write(const char *str);

// Write a string followed by a newline
void display_write_line(const char *str);

// Clear the display and reset cursor position
void display_clear();

#endif // DISPLAY_H

#ifndef DISPLAY_H
#define DISPLAY_H

#define DISPLAY_COLS 40
#define DISPLAY_ROWS 17

#ifdef __cplusplus
extern "C"
{
#endif

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

    // Update the blinking cursor (call from main loop)
    void display_update_cursor();

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H

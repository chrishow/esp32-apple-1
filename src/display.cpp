#include "display.h"
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

static const int LINE_HEIGHT = 16; // Height of font 2
static int currentRow = 0;
static int currentCol = 0;

void display_init()
{
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    // Turn on backlight
    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);

    // Setup text display
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextFont(2);
    tft.setCursor(0, 0);

    currentRow = 0;
    currentCol = 0;
}

void display_clear()
{
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
    currentRow = 0;
    currentCol = 0;
}

void display_write_char(char c)
{
    // Skip carriage return to avoid double newlines
    if (c == '\r')
    {
        return;
    }

    // Check if we need to wrap to next line (auto word wrap)
    if (c != '\n' && currentCol >= DISPLAY_COLS)
    {
        currentRow++;
        currentCol = 0;

        // Check if we need to clear screen before wrapping
        if (currentRow >= DISPLAY_ROWS)
        {
            display_clear();
        }
    }

    // Print the character
    tft.print(c);

    // Handle newline
    if (c == '\n')
    {
        currentRow++;
        currentCol = 0;

        // Clear screen when reaching max rows
        if (currentRow >= DISPLAY_ROWS)
        {
            display_clear();
        }
    }
    else
    {
        currentCol++;
    }
}

void display_write(const char *str)
{
    while (*str)
    {
        display_write_char(*str++);
    }
}

void display_write_line(const char *str)
{
    display_write(str);
    display_write_char('\n');
}

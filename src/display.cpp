#include "display.h"
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

static const int LINE_HEIGHT = 16; // Height of font 2
static int currentRow = 0;
static int currentCol = 0;

// Buffer to store screen content for scrolling
static char screenBuffer[DISPLAY_ROWS][DISPLAY_COLS + 1];

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

    display_clear();
}

void display_clear()
{
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
    currentRow = 0;
    currentCol = 0;

    // Clear the screen buffer
    for (int i = 0; i < DISPLAY_ROWS; i++)
    {
        for (int j = 0; j < DISPLAY_COLS; j++)
        {
            screenBuffer[i][j] = ' ';
        }
        screenBuffer[i][DISPLAY_COLS] = '\0';
    }
}

static void scroll_screen()
{
    // Shift buffer contents up by one line
    for (int i = 0; i < DISPLAY_ROWS - 1; i++)
    {
        for (int j = 0; j < DISPLAY_COLS; j++)
        {
            screenBuffer[i][j] = screenBuffer[i + 1][j];
        }
    }

    // Clear the last line in buffer
    for (int j = 0; j < DISPLAY_COLS; j++)
    {
        screenBuffer[DISPLAY_ROWS - 1][j] = ' ';
    }

    // Redraw the entire screen
    tft.fillScreen(TFT_BLACK);
    for (int i = 0; i < DISPLAY_ROWS; i++)
    {
        tft.setCursor(0, i * LINE_HEIGHT);
        for (int j = 0; j < DISPLAY_COLS; j++)
        {
            if (screenBuffer[i][j] != '\0')
            {
                tft.print(screenBuffer[i][j]);
            }
        }
    }

    // Position cursor at start of last line
    currentRow = DISPLAY_ROWS - 1;
    currentCol = 0;
    tft.setCursor(0, currentRow * LINE_HEIGHT);
}

void display_write_char(char c)
{
    // Convert CR to newline for both serial and display
    if (c == '\r')
    {
        c = '\n';
    }

    // Echo to serial port (after conversion)
    Serial.write(c);

    // Check if we need to wrap to next line (auto word wrap)
    if (c != '\n' && currentCol >= DISPLAY_COLS)
    {
        currentRow++;
        currentCol = 0;

        // Scroll if we're past the last row
        if (currentRow >= DISPLAY_ROWS)
        {
            scroll_screen();
        }
        else
        {
            tft.setCursor(0, currentRow * LINE_HEIGHT);
        }
    }

    // Handle newline
    if (c == '\n')
    {
        // Fill rest of current line with spaces in buffer
        while (currentCol < DISPLAY_COLS)
        {
            screenBuffer[currentRow][currentCol] = ' ';
            currentCol++;
        }

        currentRow++;
        currentCol = 0;

        // Scroll if we're past the last row
        if (currentRow >= DISPLAY_ROWS)
        {
            scroll_screen();
        }
        else
        {
            tft.setCursor(0, currentRow * LINE_HEIGHT);
        }
    }
    else
    {
        // Store character in buffer and print it
        screenBuffer[currentRow][currentCol] = c;
        tft.print(c);
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

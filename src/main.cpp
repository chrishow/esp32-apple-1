#include <Arduino.h>
#include "display.h"
#include "emulator.h"

void setup()
{
    Serial.begin(115200);

    display_init();

    reset_emulator();

    display_write_line("Serial Echo Ready");
    display_write_line("Type something...");

    Serial.println("Serial Echo Ready - Type something!");
}

void loop()
{
    // Check if data is available on serial
    if (Serial.available() > 0)
    {
        char incomingChar = Serial.read();

        // Echo back to serial
        Serial.print(incomingChar);

        // Display on screen
        display_write_char(incomingChar);
    }
}

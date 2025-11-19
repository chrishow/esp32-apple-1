#include <Arduino.h>
#include "display.h"
#include "emulator.h"

void setup()
{
    Serial.begin(115200);

    display_init();

    display_write_line("Apple-1 Emulator");
    display_write_line("Loading Wozmon...");

    Serial.println("Apple-1 Emulator");
    Serial.println("Loading Wozmon...");

    reset_emulator();

    Serial.println("Ready");
}

void loop()
{
    // Check if data is available on serial
    if (Serial.available() > 0)
    {
        char incomingChar = Serial.read();

        // Queue key for emulator
        emulator_queue_key(incomingChar);
    }

    // Run CPU instructions (multiple per loop for better performance)
    for (int i = 0; i < 1000; i++)
    {
        step_emulator();
    }

    // Update the blinking cursor
    display_update_cursor();

    // Small delay to prevent watchdog issues
    delay(1);
}

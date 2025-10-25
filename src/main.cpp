#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to enable it.
#endif

BluetoothSerial SerialBT;

// IMPORTANT: Replace with your ODBII adapter's name or MAC address
// You can use a Bluetooth scanner app on your phone to find this.
const char *deviceName = "OBDII"; // Or "V-LINK" or similar
// const char* deviceAddress = "XX:XX:XX:XX:XX:XX"; // If you prefer MAC address, uncomment and use this.
const char *pin = "1234"; // PIN for ODBII adapter authentication

// Function to send command and get response
String sendObdCommand(String command, long timeout = 3000)
{
    // Clear any existing data in the buffer
    while (SerialBT.available())
    {
        SerialBT.read();
    }

    // Send command
    SerialBT.println(command);

    String response = "";
    unsigned long startTime = millis();
    bool foundPrompt = false;

    // Keep reading until we get a '>' prompt or timeout
    while (!foundPrompt && (millis() - startTime < timeout))
    {
        if (SerialBT.available())
        {
            char c = SerialBT.read();
            response += c;

            // Check if we've received the prompt character
            if (c == '>')
            {
                foundPrompt = true;
            }
        }
        else
        {
            delay(10); // Small delay to prevent busy waiting
        }
    }

    // Remove the prompt character and clean up the response
    response.replace(">", "");
    response.trim(); // Remove leading/trailing whitespace and newlines

    Serial.print("TX: ");
    Serial.println(command);
    Serial.print("RX: ");
    Serial.println(response);

    return response;
}

void setup()
{
    Serial.begin(115200);
    Serial.println("ESP32 ODBII Reader - Bare Bones");

    // Initialize Bluetooth in Master mode (required for connecting to other devices)
    SerialBT.begin("ESP32_OBD_Scanner", true); // Name of your ESP32 device, Master mode enabled
    Serial.println("Bluetooth initialized in Master mode. Trying to connect to ODBII adapter...");

    // Set PIN for ODBII adapter authentication
    SerialBT.setPin(pin);
    Serial.print("PIN set for authentication: ");
    Serial.println(pin);

    // Connect to the ODBII adapter
    if (!SerialBT.connect(deviceName))
    {
        Serial.println("Failed to connect to ODBII adapter by name. Retrying...");
    }

    // A small delay to allow connection to establish
    delay(2000);

    if (SerialBT.connected())
    {
        Serial.println("Successfully connected to ODBII adapter!");

        // Initialize ELM327
        sendObdCommand("ATZ");
        delay(1000);            // Give it more time to reset
        sendObdCommand("ATE0"); // Echo off
        delay(500);
        sendObdCommand("ATL0"); // Linefeeds off
        delay(500);
        sendObdCommand("ATS0"); // Spaces off
        delay(500);

        Serial.println("\nELM327 Initialized. Ready to read data.");
    }
    else
    {
        Serial.println("Could not establish Bluetooth connection. Check device name/address and power.");
        while (true)
        {
            delay(100);
        } // Halt for this example
    }
}

void loop()
{
    if (SerialBT.connected())
    {
        // --- Read Battery Voltage (ELM327 Internal) ---
        String voltageResponse = sendObdCommand("ATRV");
        if (voltageResponse.endsWith("V"))
        {
            Serial.print("Battery Voltage: ");
            Serial.println(voltageResponse);
        }
        else
        {
            Serial.print("Failed to read battery voltage: ");
            Serial.println(voltageResponse);
        }
        delay(3000);

        // --- Read Coolant Temperature (ODBII PID 0105) ---
        String tempResponse = sendObdCommand("0105");
        if (tempResponse.startsWith("4105") && tempResponse.length() >= 6)
        {
            String hexValue = tempResponse.substring(4, 6);
            long intValue = strtol(hexValue.c_str(), NULL, 16);
            int coolantTemp = intValue - 40;
            Serial.print("Coolant Temperature: ");
            Serial.print(coolantTemp);
            Serial.println(" C");
        }
        else
        {
            Serial.print("Failed to read coolant temperature: ");
            Serial.println(tempResponse);
        }
        delay(3000);

        // --- Read Intake Air Temperature (ODBII PID 010F) ---
        String intakeAirTempResponse = sendObdCommand("010F");
        if (intakeAirTempResponse.startsWith("410F") && intakeAirTempResponse.length() >= 6)
        {
            String hexValue = intakeAirTempResponse.substring(4, 6);
            long intValue = strtol(hexValue.c_str(), NULL, 16);
            int intakeAirTemp = intValue - 40;
            Serial.print("Intake Air Temperature: ");
            Serial.print(intakeAirTemp);
            Serial.println(" C");
        }
        else
        {
            Serial.print("Failed to read intake air temperature: ");
            Serial.println(intakeAirTempResponse);
        }
        delay(3000);
    }
    else
    {
        Serial.println("Bluetooth disconnected. Attempting to reconnect...");
        if (!SerialBT.connect(deviceName))
        {
            Serial.println("Reconnection failed.");
            delay(5000);
        }
        delay(1000);
    }
}
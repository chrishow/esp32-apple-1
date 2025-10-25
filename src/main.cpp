#include "BluetoothSerial.h"
#include <TFT_eSPI.h>
#include <SPI.h>

// Initialize display
TFT_eSPI tft = TFT_eSPI();

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to enable it.
#endif

BluetoothSerial SerialBT;

// IMPORTANT: Replace with your ODBII adapter's name or MAC address
// You can use a Bluetooth scanner app on your phone to find this.
const char *deviceName = "OBDII"; // Or "V-LINK" or similar
// const char* deviceAddress = "XX:XX:XX:XX:XX:XX"; // If you prefer MAC address, uncomment and use this.
const char *pin = "1234"; // PIN for ODBII adapter authentication

// Display variables
String batteryVoltage = "-.--V";
int coolantTemp = -99;
int intakeAirTemp = -99;
bool obdConnected = false;

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

// Function to update the display
void updateDisplay()
{
    tft.fillScreen(TFT_BLACK);

    // Connection status (top line)
    tft.setTextSize(1);
    tft.setCursor(5, 5);
    tft.setTextColor(obdConnected ? TFT_GREEN : TFT_RED, TFT_BLACK);
    tft.print("Status: ");
    tft.print(obdConnected ? "Connected" : "Disconnected");

    // Update time (top right)
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(180, 5);
    tft.print(millis() / 1000);
    tft.print("s");

    // Battery Voltage
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(5, 25);
    tft.print("Battery: ");
    tft.setTextSize(2);
    tft.setCursor(5, 40);
    tft.println(batteryVoltage);

    // Coolant Temperature
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(5, 65);
    tft.print("Coolant: ");
    tft.setTextSize(2);
    tft.setCursor(5, 80);
    if (coolantTemp == -99)
    {
        tft.println("--.-C");
    }
    else
    {
        tft.print(coolantTemp);
        tft.println("C");
    }

    // Intake Air Temperature
    tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(5, 105);
    tft.print("Intake: ");
    tft.setTextSize(2);
    tft.setCursor(5, 120);
    if (intakeAirTemp == -99)
    {
        tft.println("--.-C");
    }
    else
    {
        tft.print(intakeAirTemp);
        tft.println("C");
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("ESP32 ODBII Reader - Bare Bones");

    // Initialize display
    tft.init();
    tft.setRotation(1); // Landscape orientation
    tft.setTextSize(1);
    tft.fillScreen(TFT_BLACK);

    // Show initial display with placeholders
    updateDisplay();

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
        obdConnected = true;
        updateDisplay();

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
            batteryVoltage = voltageResponse;
            Serial.print("Battery Voltage: ");
            Serial.println(voltageResponse);
        }
        else
        {
            batteryVoltage = "ERR";
            Serial.print("Failed to read battery voltage: ");
            Serial.println(voltageResponse);
        }
        updateDisplay();
        delay(3000);

        // --- Read Coolant Temperature (ODBII PID 0105) ---
        String tempResponse = sendObdCommand("0105");
        if (tempResponse.startsWith("4105") && tempResponse.length() >= 6)
        {
            String hexValue = tempResponse.substring(4, 6);
            long intValue = strtol(hexValue.c_str(), NULL, 16);
            coolantTemp = intValue - 40;
            Serial.print("Coolant Temperature: ");
            Serial.print(coolantTemp);
            Serial.println(" C");
        }
        else
        {
            coolantTemp = -99; // Error value
            Serial.print("Failed to read coolant temperature: ");
            Serial.println(tempResponse);
        }
        updateDisplay();
        delay(3000);

        // --- Read Intake Air Temperature (ODBII PID 010F) ---
        String intakeAirTempResponse = sendObdCommand("010F");
        if (intakeAirTempResponse.startsWith("410F") && intakeAirTempResponse.length() >= 6)
        {
            String hexValue = intakeAirTempResponse.substring(4, 6);
            long intValue = strtol(hexValue.c_str(), NULL, 16);
            intakeAirTemp = intValue - 40;
            Serial.print("Intake Air Temperature: ");
            Serial.print(intakeAirTemp);
            Serial.println(" C");
        }
        else
        {
            intakeAirTemp = -99; // Error value
            Serial.print("Failed to read intake air temperature: ");
            Serial.println(intakeAirTempResponse);
        }
        updateDisplay();
        delay(3000);
    }
    else
    {
        obdConnected = false;
        updateDisplay();
        Serial.println("Bluetooth disconnected. Attempting to reconnect...");
        if (!SerialBT.connect(deviceName))
        {
            Serial.println("Reconnection failed.");
            delay(5000);
        }
        else
        {
            obdConnected = true;
            updateDisplay();
        }
        delay(1000);
    }
}
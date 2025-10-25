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
int coolantTemp = -99;
int ambientTemp = -99;
bool obdConnected = false;

// Fuel consumption tracking
struct FuelData
{
    float instantConsumption;     // Current L/100km
    float totalFuelUsed;          // Total liters since connection
    float averageConsumption;     // Average L/100km since connection
    float totalDistance;          // Total km traveled
    unsigned long startTime;      // When tracking started
    unsigned long lastUpdateTime; // For time-based calculations
    float lastSpeed;              // Previous speed reading
} fuelData = {0.0, 0.0, 0.0, 0.0, 0, 0, 0.0};

// Current sensor readings for fuel calculation
float currentMAF = -1.0;   // Mass Air Flow (g/s)
float currentSpeed = -1.0; // Vehicle speed (km/h)

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

// Calculate fuel consumption from MAF and speed (diesel-specific)
void calculateFuelConsumption()
{
    if (currentMAF > 0 && currentSpeed > 0)
    {
        // Diesel-specific constants
        const float DIESEL_AFR = 14.5;      // Air-fuel ratio for diesel
        const float DIESEL_DENSITY = 832.0; // Diesel density (g/L) at 15°C

        // Calculate fuel rate (L/h)
        float fuelRate = (currentMAF * 3600.0) / (DIESEL_AFR * DIESEL_DENSITY);

        // Calculate instant consumption (L/100km)
        fuelData.instantConsumption = (fuelRate * 100.0) / currentSpeed;

        // Update trip totals if we have valid time data
        unsigned long currentTime = millis();
        if (fuelData.lastUpdateTime > 0)
        {
            float timeHours = (currentTime - fuelData.lastUpdateTime) / 3600000.0; // Convert ms to hours
            float distanceKm = (currentSpeed * timeHours);                         // Distance in this interval

            fuelData.totalFuelUsed += (fuelRate * timeHours);
            fuelData.totalDistance += distanceKm;

            // Calculate average consumption
            if (fuelData.totalDistance > 0.01)
            { // Avoid division by zero
                fuelData.averageConsumption = (fuelData.totalFuelUsed * 100.0) / fuelData.totalDistance;
            }
        }

        fuelData.lastUpdateTime = currentTime;
    }
    else
    {
        // No valid data
        fuelData.instantConsumption = -1.0;
    }
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

    // Total fuel used (top right)
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(160, 5);
    tft.print(fuelData.totalFuelUsed, 1);
    tft.print("L");

    // Current Fuel Consumption (prominent display)
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(5, 25);
    tft.print("Current:");
    tft.setTextSize(2);
    tft.setCursor(5, 40);
    if (fuelData.instantConsumption > 0)
    {
        tft.print(fuelData.instantConsumption, 1);
        tft.println("L/100km");
    }
    else
    {
        tft.println("--.- L/100km");
    }

    // Average Fuel Consumption
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(5, 70);
    tft.print("Average:");
    tft.setTextSize(2);
    tft.setCursor(5, 85);
    if (fuelData.averageConsumption > 0)
    {
        tft.print(fuelData.averageConsumption, 1);
        tft.println("L/100km");
    }
    else
    {
        tft.println("--.- L/100km");
    }

    // Temperature readings (line 1)
    tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(5, 115);
    tft.print("Cool:");
    if (coolantTemp != -99)
    {
        tft.print(coolantTemp);
    }
    else
    {
        tft.print("--");
    }
    tft.print("C");

    // Ambient temperature (same line, middle)
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(80, 115);
    tft.print("Amb:");
    if (ambientTemp != -99)
    {
        tft.print(ambientTemp);
    }
    else
    {
        tft.print("--");
    }
    tft.print("C");

    // Speed (same line, right side)
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(150, 115);
    if (currentSpeed >= 0)
    {
        tft.print(currentSpeed, 0);
    }
    else
    {
        tft.print("--");
    }
    tft.print("kmh");

    // Trip distance (bottom line)
    tft.setCursor(5, 128);
    tft.print("Trip: ");
    tft.print(fuelData.totalDistance, 1);
    tft.print("km");
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

        // Initialize fuel tracking
        fuelData.startTime = millis();
        fuelData.lastUpdateTime = 0;
        fuelData.totalFuelUsed = 0.0;
        fuelData.totalDistance = 0.0;
        fuelData.averageConsumption = 0.0;

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
        delay(1000);

        // --- Read Mass Air Flow (ODBII PID 0110) ---
        String mafResponse = sendObdCommand("0110");
        if (mafResponse.startsWith("4110") && mafResponse.length() >= 8)
        {
            // MAF is 2 bytes: ((A*256)+B)/100 grams/sec
            String hexA = mafResponse.substring(4, 6);
            String hexB = mafResponse.substring(6, 8);
            long valueA = strtol(hexA.c_str(), NULL, 16);
            long valueB = strtol(hexB.c_str(), NULL, 16);
            currentMAF = ((valueA * 256) + valueB) / 100.0;
            Serial.print("Mass Air Flow: ");
            Serial.print(currentMAF);
            Serial.println(" g/s");
        }
        else
        {
            currentMAF = -1.0; // Error value
            Serial.print("Failed to read MAF: ");
            Serial.println(mafResponse);
        }
        delay(1000);

        // --- Read Vehicle Speed (ODBII PID 010D) ---
        String speedResponse = sendObdCommand("010D");
        if (speedResponse.startsWith("410D") && speedResponse.length() >= 6)
        {
            String hexValue = speedResponse.substring(4, 6);
            long intValue = strtol(hexValue.c_str(), NULL, 16);
            currentSpeed = intValue; // km/h
            Serial.print("Vehicle Speed: ");
            Serial.print(currentSpeed);
            Serial.println(" km/h");
        }
        else
        {
            currentSpeed = -1.0; // Error value
            Serial.print("Failed to read speed: ");
            Serial.println(speedResponse);
        }
        delay(1000);

        // --- Read Ambient Air Temperature (ODBII PID 0146) ---
        String ambientResponse = sendObdCommand("0146");
        if (ambientResponse.startsWith("4146") && ambientResponse.length() >= 6)
        {
            String hexValue = ambientResponse.substring(4, 6);
            long intValue = strtol(hexValue.c_str(), NULL, 16);
            ambientTemp = intValue - 40;
            Serial.print("Ambient Temperature: ");
            Serial.print(ambientTemp);
            Serial.println(" C");
        }
        else
        {
            ambientTemp = -99; // Error value
            Serial.print("Failed to read ambient temperature: ");
            Serial.println(ambientResponse);
        }

        // Calculate fuel consumption
        calculateFuelConsumption();

        updateDisplay();
        delay(2000);
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
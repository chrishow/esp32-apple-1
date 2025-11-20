# Apple-1 Emulator

6502 emulator running on ESP32 (TTGO T-Display) with Apple-1 ROMs including Wozmon and Integer BASIC.

## Build & Upload

```bash
platformio run --target upload
platformio device monitor
```

## Hardware

- TTGO T-Display (ESP32)
- ST7789 135x240 TFT display

## ROM contents

### Wozmon  
Stored at 0xFF00, runs on boot

### BASIC 
Stored at 0xE000, run with `E000R`

### Cellular Automaton
Stored at 0x0300  
Run with `300R` 
A good setting is: INITIAL: 1, RADIUS: 2, RULE: 15
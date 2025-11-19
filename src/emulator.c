#include "emulator.h"
#include "display.h"
#include "wozmon_rom.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>

// Apple-1 Memory Map
#define KBD 0xD010   // Keyboard data register
#define KBDCR 0xD011 // Keyboard control register
#define DSP 0xD012   // Display data register
#define DSPCR 0xD013 // Display control register

#define RAM_SIZE 0x2000     // 8KB RAM (0x0000-0x1FFF)
#define ROM_START 0xFF00    // ROM starts at 0xFF00
#define ROM_SIZE 256        // 256 bytes
#define RESET_VECTOR 0xFFFC // Reset vector location

uint8_t memory[65536];

// Keyboard input buffer
static uint8_t kbd_data = 0;
static uint8_t kbd_strobe = 0; // Separate strobe flag
static char last_char = 0; // To prevent duplicate chars

// Queue a character from keyboard (serial input)
void emulator_queue_key(char c)
{
    // Ignore duplicate characters (happens when terminal sends both \r and \n)
    if (c == last_char && (c == '\r' || c == '\n'))
    {
        return;
    }
    last_char = c;
    
    // Convert lowercase to uppercase (Apple-1 style)
    if (c >= 'a' && c <= 'z')
    {
        c = c - 'a' + 'A';
    }

    // Convert newline to CR (Apple-1 line terminator)
    if (c == '\n' || c == '\r')
    {
        c = '\r'; // 0x0D
    }

    // Store character and set strobe
    kbd_data = c;
    kbd_strobe = 1;
}

// These functions are required by fake6502
uint8_t read6502(uint16_t address)
{
    static uint32_t dsp_read_count = 0;
    
    // Handle memory-mapped I/O
    switch (address)
    {
    case KBD: // Keyboard data - return with high bit set if strobe is active
    {
        uint8_t value = kbd_data | (kbd_strobe ? 0x80 : 0x00);
        kbd_strobe = 0; // Clear strobe after reading KBD
        return value;
    }

    case KBDCR: // Keyboard control - just return status, don't clear strobe
    {
        // Return high bit set only if strobe is active
        // This is critical: kbd_data might have bit 7 set, but we only
        // want to indicate "key ready" when strobe is active
        uint8_t status = kbd_strobe ? 0x80 : 0x00;
        return status;
    }

    case DSP: // Display data - reads undefined, return 0
        return 0x00;

    case DSPCR: // Display control - return ready status
        return 0x00; // Bit 7 clear = ready (opposite of keyboard!)

    default:
        return memory[address];
    }
}

void write6502(uint16_t address, uint8_t value)
{
    // Handle memory-mapped I/O
    switch (address)
    {
    case DSP: // Display output
    {
        char c = value & 0x7F; // Strip high bit

        // Output to display (display will handle CR conversion)
        display_write_char(c);
        break;
    }

    case DSPCR: // Display control (usually not written)
        break;

    case KBD:   // Keyboard registers are read-only
    case KBDCR:
        break;

    default:
        // Protect ROM area from writes
        if (address < ROM_START)
        {
            memory[address] = value;
        }
        break;
    }
}

void setup_emulator()
{
    // Clear all memory including ROM area
    for (uint32_t i = 0; i < 65536; i++)
    {
        memory[i] = 0;
    }

    // Load Wozmon ROM from embedded data
    size_t copy_size = (wozmon_rom_len < ROM_SIZE) ? wozmon_rom_len : ROM_SIZE;
    memcpy(&memory[ROM_START], wozmon_rom, copy_size);
    
    printf("Wozmon loaded from embedded ROM (%u bytes)\n", copy_size);
    
    // Dump ROM sections for debugging
    printf("ROM $FF00-$FF0F: ");
    for (int i = 0; i < 16; i++) {
        printf("%02X ", memory[ROM_START + i]);
    }
    printf("\n");
    
    printf("ROM $FF40-$FF4F: ");
    for (int i = 0x40; i < 0x50; i++) {
        printf("%02X ", memory[ROM_START + i]);
    }
    printf("\n");

    // Check reset vector
    printf("Reset vector at $FFFC: %02X%02X (points to $%02X%02X)\n", 
           memory[RESET_VECTOR+1], memory[RESET_VECTOR],
           memory[RESET_VECTOR+1], memory[RESET_VECTOR]);
    
    // If reset vector is invalid (0xFFFF), set it to ROM_START
    if (memory[RESET_VECTOR] == 0xFF && memory[RESET_VECTOR+1] == 0xFF)
    {
        memory[RESET_VECTOR] = 0x00;     // Low byte
        memory[RESET_VECTOR + 1] = 0xFF; // High byte  
        printf("Fixed reset vector to point to $FF00\n");
    }

    // Initialize keyboard
    kbd_data = 0;
    kbd_strobe = 0;
}

void reset_emulator()
{
    setup_emulator();
    reset6502();
}

void step_emulator()
{
    static uint16_t last_pc = 0;
    static uint32_t stuck_count = 0;
    
    step6502();
    
    // Detect if CPU is stuck in a loop
    if (PC == last_pc)
    {
        stuck_count++;
        if (stuck_count == 50000)
        {
            printf("CPU stuck at PC=%04X A=%02X X=%02X Y=%02X SP=%02X\n", PC, A, X, Y, SP);
            printf("Memory at PC: %02X %02X %02X\n", memory[PC], memory[PC+1], memory[PC+2]);
            stuck_count = 0;
        }
    }
    else
    {
        stuck_count = 0;
    }
    
    last_pc = PC;
}
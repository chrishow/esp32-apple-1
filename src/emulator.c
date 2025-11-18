#include "emulator.h"

uint8_t memory[65536];

// These functions are required by fake6502
uint8_t read6502(uint16_t address)
{
    return memory[address];
}

void write6502(uint16_t address, uint8_t value)
{
    memory[address] = value;
}

void setup_emulator() {

}

void reset_emulator() {
    reset6502();
}

void step_emulator() {
    uint16_t save = PC;
    step6502();
    if (save == PC) { // No change in PC, likely a BRK instruction
        printf("PC=%04x ", PC);
        printf("A=%02x X=%02x Y=%02x SP=%02x ", A, X, Y, SP);
        printf("P=%02x\n", getP());
        return;
    }
}
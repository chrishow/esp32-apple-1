#ifndef EMULATOR_H
#define EMULATOR_H

#include "fake6502.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void setup_emulator();
    void reset_emulator();
    void step_emulator();
    void emulator_queue_key(char c);
    uint8_t read_memory(uint16_t address);
    void write_memory(uint16_t address, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif // EMULATOR_H
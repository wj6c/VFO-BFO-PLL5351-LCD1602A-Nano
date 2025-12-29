#include <EEPROM.h>
#include "eeprom_storage.h"
#include "definitions.h"
#include "vfo-screen.h"

// keep 4 for avrdude's flash programming count
#define EEPROM_TOP (EEPROM.length() - 4)

// Testing value
//#define EEPROM_TOP (26)

static constexpr uint16_t VFO_START_ADDR = 14;
static uint16_t current_vfo_offset       = VFO_START_ADDR;
static int_fast32_t current_vfo_freq     = 0;
static bool current_sentinel             = false;

void findLatestVFO(int_fast32_t &vfo, uint16_t &next);
static void storeVFO(int_fast32_t vfo_freq);

template<typename T> constexpr bool getSentinel(T value)
{
    return 0x01 & (value >> (8*sizeof(T) - 1));
}

int_fast32_t setSentinel(int_fast32_t value, bool sentinel)
{
    uint_fast32_t mask = ((uint_fast32_t)1 << (8*sizeof(int_fast32_t) - 1));
    if(sentinel)
        return value | mask;
    else
        return value & (~mask);
}

void loadState(TVFOState &state)
{
    uint16_t addr     = 0;
    int_fast32_t freq = 0;
    uint8_t tmp       = 0;

    EEPROM.get(addr, freq); addr += sizeof(int_fast32_t);
    // check that the EEPROM is actually programmed
    if(freq < 0)
    {
        Serial.println("EEPROM is not programmed yet, using defaults!");
        return;
    }
    else
    {
        state.bfo_center = freq;
        state.bfo_dirty  = true;
    }

    // IF
    EEPROM.get(addr, state.if_freq); addr += sizeof(int_fast32_t);

    // Si5351 frequency correction
    EEPROM.get(addr, state.si3531_correction); addr += sizeof(int_fast32_t);

    // Tuning steps
    EEPROM.get(addr++, tmp);
    state.rit_step_idx = tmp & 0x0F;
    state.vfo_step_idx = (tmp & 0xF0) >> 4;

    // Opmodes
    EEPROM.get(addr++, tmp);
    state.opmode = (Defs::OperatingMode) (0x03 & tmp);
    state.ifmode = (Defs::IFMode) (0x03 & (tmp >> 2));
    state.sidebands_swap = ((1 << 5) & tmp);

    // VFO
    EEPROM.get(VFO_START_ADDR, current_vfo_freq);
    current_sentinel = getSentinel(current_vfo_freq);

    findLatestVFO(current_vfo_freq, current_vfo_offset);
    state.vfo_freq = current_vfo_freq;

    state.bfo_dirty = true;
    state.vfo_dirty = true;
    state.rit_dirty = true;
    state.if_dirty  = true;
}

void storeState(const TVFOState &state)
{
    // Serial.println("storeState");
    uint16_t addr = 0;

    // We are using put() and update() which do EEPROM
    // write only if writing an actually different value.
    // So no need to check if the values differ to save
    // EEPROM wear. The only exception is VFO freq bcs.
    // of the wear leveling used.

    // BFO freq
    EEPROM.put(addr, state.bfo_center);
    addr += sizeof(int_fast32_t);

    // IF freq
    EEPROM.put(addr, state.if_freq);
    addr += sizeof(int_fast32_t);

    // Si5351 frequency correction
    EEPROM.put(addr, state.si3531_correction); addr += sizeof(int_fast32_t);

    // Tuning steps
    uint8_t steps = ((0x0F & state.vfo_step_idx) << 4) | (0x0F & state.rit_step_idx);
    EEPROM.update(addr++, steps);

    // Opmodes
    uint8_t modes = ((0x03 & (uint8_t) state.opmode) |
                     ((0x03 & (uint8_t) state.ifmode) << 2) |
                     ((0x01 & ((state.sidebands_swap)?1:0)) << 5));
    EEPROM.update(addr++, modes);

    if(current_vfo_freq != state.vfo_freq)
        storeVFO(state.vfo_freq);
}

void storeVFO(int_fast32_t vfo_freq)
{
    // Serial.print("storeVFO: ");
    // Serial.print(vfo_freq);
    // Serial.print(" addr ");
    // Serial.print(current_vfo_offset);
    // Serial.print(" sentinel ");
    // Serial.println(current_sentinel);

    int_fast32_t sentinel_val = setSentinel(vfo_freq, current_sentinel);
    EEPROM.put(current_vfo_offset, sentinel_val);

    if(current_vfo_offset + sizeof(int_fast32_t) < EEPROM_TOP) // 4 for avrdude flash counter
        current_vfo_offset += sizeof(int_fast32_t);
    else
    {
        current_vfo_offset = VFO_START_ADDR;
        current_sentinel = !current_sentinel;
    }
}

void findLatestVFO(int_fast32_t &vfo, uint16_t &next)
{
    int_fast32_t freq, tmp;
    uint16_t addr = VFO_START_ADDR;

    EEPROM.get(addr, freq);
    bool sentinel = getSentinel(freq);

    addr += sizeof(int_fast32_t);
    while(addr < EEPROM_TOP)
    {
        EEPROM.get(addr, tmp);
        if(getSentinel(tmp) != sentinel)
            break;
        else
            freq = tmp;

        addr += sizeof(int_fast32_t);
    }

    vfo  = freq & 0x7FFFFFFF;
    next = (addr < EEPROM_TOP) ? addr : VFO_START_ADDR;

    // Serial.print("findLatestVFO: found ");
    // Serial.print(vfo);
    // Serial.print(" next addr ");
    // Serial.println(next);
}

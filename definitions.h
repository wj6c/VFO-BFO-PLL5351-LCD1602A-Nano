#pragma once

#include <stdint.h>
#include <Arduino.h>

#include "lcdui/utilities.h"

namespace Defs
{
// Radio config ///////////////////////////////////////////////////////////////
enum class OperatingMode
{
    AM  = 0,
    LSB = 1,
    USB = 2,
    CW  = 3
};

enum class IFMode
{
    OFF        = 0,
    F_MINUS_IF = 1,
    F_PLUS_IF  = 2
};

constexpr auto OperatingModeStr = Utilities::make_array("AM", "LSB", "USB", "CW");
constexpr auto IFModeStr        = Utilities::make_array("OFF", "F-IF", "F+IF");



// Yaesu filter
constexpr auto IFFreq           = 8987500; // IF starting frequency
constexpr auto BFOFreq          = 8987500; // BFO starting frequency

// Crystal ladder filter
// constexpr auto IFFreq           = 11997450; // IF starting frequency
// constexpr auto BFOFreq          = 11997450; // BFO starting frequency

constexpr auto BFOSSBOffset     = 1250;
constexpr auto BFOCWOffset      = 600;

constexpr auto TuningIncrements = Utilities::make_array(100000, 10, 50,
                                                        100, 500, 1000,
                                                        2500, 5000, 10000); // need the 100000 first to set the type
constexpr auto RitIncrements = Utilities::make_array(10, 100, 1000, 1);

constexpr auto TuningIncrementIdx = 4;
constexpr auto RitIncrementIdx    = 0;
constexpr auto Rit                = 0;

constexpr auto SidebandsSwap      = false;

constexpr auto VFOFreq          = 7150000UL;  // <--- UL importante para tipo long

constexpr int_fast32_t FreqLimitUp   = 30000000UL;  // 30 MHz
constexpr int_fast32_t FreqLimitDown = 300000UL;   // 300 kHz
const int_fast32_t Si5351CorrUp   =  100000;
const int_fast32_t Si5351CorrDown = -100000;

// HW config //////////////////////////////////////////////////////////////////
constexpr auto BtnA       = A0;
constexpr auto BtnB       = A1;
constexpr auto BtnC       = A2;
constexpr auto BtnTX      = 4;
constexpr auto BtnEncoder = A3;

constexpr auto BtnDebounceInterval = 50;

// Base I2C address for the Si5351
// 0x60 is according to the datasheet but there are chips
// with other addresses out there
// 0x62, 0x6f have been seen
constexpr auto SI5351Addr       = 0x62;
constexpr auto SI5351Correction = 2004;

// globals
constexpr uint8_t LCDWidth  = 16;
constexpr uint8_t LCDHeight =  2;
};

// Include the library code
#include <avr/interrupt.h>

#include <LiquidCrystal.h>
#include <Bounce2.h>
#include <Rotary.h>
#include <si5351.h>
#include "Wire.h"

#include "definitions.h"
#include "lcdui/lcdui.h"
#include "vfo-screen.h"
#include "ArduinoLCDApi.h"
#include "eeprom_storage.h"

Rotary encoder = Rotary(3, 2);           // Sets the pins the rotary encoder uses.  Must be interrupt pins.
LiquidCrystal lcd(13, 12, 8, 9, 10, 11); // LCD pins (rs, enable, d4,d5,d6,d7)
Si5351 si5351(Defs::SI5351Addr);
Bounce btnA, btnB, btnC, btnTX, btnEnc;
int8_t encoder_delta = 0;
TVFOState VFOState;

bool memory_dirty = false;
uint_fast32_t last_memory_save = millis();

void clampFreq(TVFOState &VFOState);
void setFrequencies(TVFOState &VFOState);
void update_output(TVFOState &VFOState);

// Main code //////////////////////////////////////////////////////////////////
void setup()
{
    // Init hw //////////////////////////////////////////////////////////////////
    Serial.begin(38400);
    Serial.println("Si5351 VFO v5.2 (c) Jan Ciger 2024");
    Serial.println(">");

    // button pin setup
    pinMode(Defs::BtnEncoder, INPUT_PULLUP); // Connect to a button that goes to GND on push
    pinMode(Defs::BtnA, INPUT_PULLUP);
    pinMode(Defs::BtnB, INPUT_PULLUP);
    pinMode(Defs::BtnC, INPUT_PULLUP);
    pinMode(Defs::BtnTX, INPUT_PULLUP);

    digitalWrite(Defs::BtnEncoder, HIGH); // enable pull-ups
    digitalWrite(Defs::BtnA, HIGH);
    digitalWrite(Defs::BtnB, HIGH);
    digitalWrite(Defs::BtnC, HIGH);
    digitalWrite(Defs::BtnTX, HIGH);

    // encoder
    encoder.begin(true);        // enable encoder pull-ups

    // configure debouncers
    btnA.attach(Defs::BtnA);
    btnB.attach(Defs::BtnB);
    btnC.attach(Defs::BtnC);
    btnTX.attach(Defs::BtnTX);
    btnEnc.attach(Defs::BtnEncoder);

    btnA.interval(Defs::BtnDebounceInterval);
    btnB.interval(Defs::BtnDebounceInterval);
    btnC.interval(Defs::BtnDebounceInterval);
    btnTX.interval(Defs::BtnDebounceInterval);
    btnEnc.interval(Defs::BtnDebounceInterval);

    // lcd setup
    lcd.begin(Defs::LCDWidth, Defs::LCDHeight);

    // Si5351
    si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, Defs::SI5351Correction * 100ULL);
    si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);
    si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_8MA);

    // encoder interrupts
    PCICR |= (1 << PCIE2);
    PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
    sei();

    // Load state
    loadState(VFOState);

    // Init menu //////////////////////////////////////////////////////////////
    TArduinoLCDApi lcd_api(lcd, Defs::LCDWidth, Defs::LCDHeight);

    auto mode_list_box = LCDUI::make_listbox(lcd_api, "Op mode", Defs::OperatingModeStr,
                                             [](uint8_t selected, const char *) {
                                                 VFOState.bfo_dirty = true;
                                                 VFOState.opmode = static_cast<Defs::OperatingMode>(selected);
                                             },
                                             static_cast<uint8_t>(VFOState.opmode));

    auto bfo_center_item = LCDUI::make_number(lcd_api, "BFO Freq. [Hz]", 9, VFOState.bfo_center,
                                              [](int32_t x) {
                                                  VFOState.bfo_dirty  = true;
                                                  VFOState.bfo_center = x;
                                              },
                                              Defs::FreqLimitDown, Defs::FreqLimitUp);

    auto if_list_box = LCDUI::make_listbox(lcd_api, "IF mode", Defs::IFModeStr,
                                           [](uint8_t selected, const char *) {
                                               VFOState.vfo_dirty = true;
                                               VFOState.ifmode    = static_cast<Defs::IFMode>(selected);
                                           },
                                           static_cast<uint8_t>(VFOState.ifmode));

    auto if_item = LCDUI::make_number(lcd_api, "IF Freq. [Hz]", 9, VFOState.if_freq,
                                      [](int32_t x) {
                                          VFOState.if_dirty = true;
                                          VFOState.if_freq  = x;
                                      },
                                      Defs::FreqLimitDown, Defs::FreqLimitUp);

    auto si5351corr_item = LCDUI::make_number(lcd_api, "Si5351 Cal [Hz]", 9, VFOState.si3531_correction,
                                      [](int32_t x) {
                                          VFOState.cal_dirty = true;
                                          VFOState.si3531_correction  = x;
                                      },
                                      Defs::Si5351CorrDown, Defs::Si5351CorrUp);

    auto swap_sidebands_box
        = LCDUI::make_checkbox(lcd_api, "Invert sidebands", VFOState.sidebands_swap, [](bool checked) {
              VFOState.vfo_dirty      = true;
              VFOState.sidebands_swap = checked;
          });

    auto vfo_screen = make_vfo_screen(lcd_api, "VFO", VFOState);
    vfo_screen.set_focus(true);

    auto menu = LCDUI::make_menu(vfo_screen, mode_list_box, if_list_box, if_item, bfo_center_item,
                                 swap_sidebands_box, si5351corr_item);

    menu.render(true);

    // Main loop //////////////////////////////////////////////////////////////
    while (1)
    {
        btnA.update();
        btnB.update();
        btnC.update();
        btnTX.update();
        btnEnc.update();

        LCDUI::InputEvent ev;
        memset(&ev, 0, sizeof(LCDUI::InputEvent));

        ev.buttons = (!btnEnc.read()) | (!btnA.read() << 1) | (!btnB.read() << 2) | (!btnC.read() << 3)
                     | (!btnTX.read() << 4);

        if (btnC.fell())
        {
            // jump in/out of the settings
            if (menu.current_idx() == 0)
            {
                menu.set_current_idx(1);
                vfo_screen.set_focus(false);
            } else
            {
                menu.set_current_idx(0);
                vfo_screen.set_focus(true);
            }

            menu.render(true); // force redraw
        }

        // hack - force update on TX change
        // because we may have RIT on
        if (btnTX.rose() || btnTX.fell())
        {
            VFOState.tx_active = !btnTX.read();
            VFOState.vfo_dirty = true;
        }

        if (encoder_delta != 0)
        {
            ev.ax         = 0;
            ev.ay         = encoder_delta;
            encoder_delta = 0;
        }

        if (ev.ax != 0 || ev.ay != 0 || btnA.rose() || btnA.fell() || btnB.rose() || btnB.fell()
            || btnC.rose() || btnC.fell() || btnTX.rose() || btnTX.fell() || btnEnc.rose() || btnEnc.fell())
        {
            menu.handle_input(ev);
        }

        update_output(VFOState);
        menu.render();

        // save to eeprom
        if(memory_dirty && last_memory_save + 10000 < millis())
        {
            memory_dirty = false;
            storeState(VFOState);
        }
    }
}

void loop() {}

void update_output(TVFOState &VFOState)
{
    if (VFOState.bfo_dirty || VFOState.vfo_dirty || VFOState.rit_dirty || VFOState.if_dirty)
    {
        clampFreq(VFOState);
        memory_dirty = true;
        last_memory_save = millis();

        VFOState.bfo_dirty = false;
        VFOState.vfo_dirty = false;
        VFOState.rit_dirty = false;
        VFOState.if_dirty  = false;

        setFrequencies(VFOState);
    }

    if(VFOState.cal_dirty)
    {
        memory_dirty = true;
        last_memory_save = millis();
        VFOState.cal_dirty = false;

        si5351.set_correction(VFOState.si3531_correction*100LL, SI5351_PLL_INPUT_XO);
    }
}

void clampFreq(TVFOState &VFOState)
{
    // Clamp VFO to sensible range
    if (VFOState.vfo_freq >= Defs::FreqLimitUp)
        VFOState.vfo_freq = Defs::FreqLimitUp;

    if (VFOState.vfo_freq <= Defs::FreqLimitDown)
        VFOState.vfo_freq = Defs::FreqLimitDown;

    // Clamp after applying IF
    // Handle only +IF case
    // In the -IF case if the user tunes too close to the IF frequency,
    // the VFO output may fail - to be expected, it is an operator error.
    // Clamping it in some meaningful value is tricky because of the way the UI
    // works. It is a lesser evil to lose the VFO output than to have mysterious
    // frequency changes.
    if(VFOState.ifmode == Defs::IFMode::F_PLUS_IF &&
       VFOState.vfo_freq + VFOState.if_freq > Defs::FreqLimitUp)
        VFOState.vfo_freq = Defs::FreqLimitUp - VFOState.if_freq;

    // Correct RIT
    if (VFOState.vfo_freq + VFOState.rit > Defs::FreqLimitUp)
        VFOState.rit = Defs::FreqLimitUp - VFOState.vfo_freq;

    if (VFOState.vfo_freq + VFOState.rit < Defs::FreqLimitDown)
        VFOState.rit = Defs::FreqLimitDown - VFOState.vfo_freq;

    // Clamp BFO
    if (VFOState.bfo_freq > Defs::FreqLimitUp)
        VFOState.bfo_freq = Defs::FreqLimitUp;

    if (VFOState.bfo_freq < Defs::FreqLimitDown)
        VFOState.bfo_freq = Defs::FreqLimitDown;
}

void setFrequencies(TVFOState &VFOState)
{
    int32_t frequency = VFOState.vfo_freq;

    // VFO
    switch (VFOState.ifmode)
    {
        case Defs::IFMode::F_PLUS_IF: frequency += VFOState.if_freq; break;

        case Defs::IFMode::F_MINUS_IF: frequency = abs(frequency - VFOState.if_freq); break;

        default: break;
    }

    if (!VFOState.tx_active)
        frequency += VFOState.rit;

    // frequency calc from datasheet page 8 = <sys clock> * <frequency tuning word>/2^32
    si5351.set_freq(frequency * 100ULL, SI5351_CLK0);

    // BFO
    switch (VFOState.opmode)
    {
        case Defs::OperatingMode::AM: si5351.output_enable(SI5351_CLK2, 0); break;

        case Defs::OperatingMode::LSB:
            if (VFOState.sidebands_swap)
                VFOState.bfo_freq = VFOState.bfo_center - Defs::BFOSSBOffset;
            else
                VFOState.bfo_freq = VFOState.bfo_center + Defs::BFOSSBOffset;

            si5351.output_enable(SI5351_CLK2, 1);
            si5351.set_freq(VFOState.bfo_freq * 100ULL, SI5351_CLK2);
            break;

        case Defs::OperatingMode::USB:
            if (VFOState.sidebands_swap)
                VFOState.bfo_freq = VFOState.bfo_center + Defs::BFOSSBOffset;
            else
                VFOState.bfo_freq = VFOState.bfo_center - Defs::BFOSSBOffset;

            si5351.output_enable(SI5351_CLK2, 1);
            si5351.set_freq(VFOState.bfo_freq * 100ULL, SI5351_CLK2);
            break;

        case Defs::OperatingMode::CW:
            VFOState.bfo_freq = VFOState.bfo_center - Defs::BFOCWOffset;
            si5351.output_enable(SI5351_CLK2, 1);
            si5351.set_freq(VFOState.bfo_freq * 100ULL, SI5351_CLK2);
            break;
    }

    // Serial.println(frequency);
}


ISR(PCINT2_vect)
{
    unsigned char dir = encoder.process();

    switch (dir)
    {
        case DIR_NONE:
            // encoder_delta = 0;
            break;

        case DIR_CCW: --encoder_delta; break;

        case DIR_CW: ++encoder_delta; break;
    }
}

int main()
{
    init();
    setup();

    for (;;)
        loop();
}

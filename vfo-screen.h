#pragma once

#include "Arduino.h"
#include "definitions.h"
#include "lcdui/lcdui.h"

struct TVFOState
{
    Defs::OperatingMode opmode = Defs::OperatingMode::LSB;
    Defs::IFMode ifmode        = Defs::IFMode::F_MINUS_IF;

    int_fast32_t vfo_freq = Defs::VFOFreq;

    int_fast32_t if_freq    = Defs::IFFreq;
    int_fast32_t bfo_freq   = Defs::BFOFreq;
    int_fast32_t bfo_center = Defs::BFOFreq;

    int_fast16_t rit = Defs::Rit;

    uint_fast8_t vfo_step_idx = Defs::TuningIncrementIdx;
    uint_fast8_t rit_step_idx = Defs::RitIncrementIdx;

    int_fast32_t si3531_correction = Defs::SI5351Correction;

    bool sidebands_swap = Defs::SidebandsSwap;
    bool tx_active      = false;

    // ensures the first update
    bool vfo_dirty = true;
    bool bfo_dirty = true;
    bool if_dirty  = true;
    bool rit_dirty = true;
    bool cal_dirty = true;
};

template <typename LCDApi> class VFOScreen : public LCDUI::MenuItemBase<LCDApi, VFOScreen<LCDApi>>
{
    using base = LCDUI::MenuItemBase<LCDApi, VFOScreen<LCDApi>>;

public:
    enum VFOScreenMode
    {
        StepTuning,
        ChangingStep,
        DecadeTuning,
        ChangingDecade,
        RitTuning,
        ChangingRitStep
    };

public:
    VFOScreen(LCDApi &api, const char *title, TVFOState &state)
        : base(api, title), m_state(state), m_mode(StepTuning), m_selected_decade(0)
    {
    }

    void render_impl()
    {
        if (!this->is_dirty())
            return;

        constexpr int BUF_LEN = 11;
        char buf[BUF_LEN];
        char disp_buf[BUF_LEN];

        memset(buf, 0, BUF_LEN);
        memset(disp_buf, 0, BUF_LEN);

        this->m_lcd_api.int2string(buf, BUF_LEN, m_state.vfo_freq);

        auto s         = buf + BUF_LEN - 2;
        auto t         = disp_buf + BUF_LEN - 2;
        int8_t d_count = 0;

        while (t >= disp_buf)
        {
            if (d_count == 3)
            {
                *(t--)  = '.';
                d_count = 0;
            }

            *(t--) = *(s--);
            ++d_count;
        }

        // first line
        this->m_lcd_api.clear();
        this->m_lcd_api.setCursorPosition(0, 0);

        this->m_lcd_api.drawText(disp_buf);
        this->m_lcd_api.drawText("Hz");

        if (m_state.tx_active)
            this->m_lcd_api.drawText(" TX");
        else
        {
            switch (m_state.ifmode)
            {
                case Defs::IFMode::F_MINUS_IF: this->m_lcd_api.drawText("-IF"); break;

                case Defs::IFMode::F_PLUS_IF: this->m_lcd_api.drawText("+IF"); break;

                case Defs::IFMode::OFF: break;
            };
        }

        // second line
        uint8_t p = this->m_lcd_api.width() - 4;
        this->m_lcd_api.setCursorPosition(p, 1);
        this->m_lcd_api.drawText(Defs::OperatingModeStr[static_cast<uint8_t>(m_state.opmode)]);

        this->m_lcd_api.setCursorPosition(0, 1);
        this->m_lcd_api.setCursorVisible(false);

        switch (m_mode)
        {
            case ChangingStep:
            {
                this->m_lcd_api.setCursorVisible(true);
                this->m_lcd_api.drawText("STP: ");
                this->m_lcd_api.drawText(Defs::TuningIncrements[m_state.vfo_step_idx]);
                break;
            }

            case StepTuning:
            {
                this->m_lcd_api.drawText("VFO: ");
                this->m_lcd_api.drawText(Defs::TuningIncrements[m_state.vfo_step_idx]);
                break;
            }

            case DecadeTuning:
            case ChangingDecade:
            {
                this->m_lcd_api.drawText("CUR: ");

                uint8_t c = 9 - m_selected_decade;
                if (m_selected_decade >= 3)
                    --c;

                if (m_selected_decade >= 6)
                    --c;

                this->m_lcd_api.setCursorPosition(c, 0);
                this->m_lcd_api.setCursorVisible(true);

                break;
            }

            case RitTuning:
            {
                this->m_lcd_api.drawText("RIT: ");
                this->m_lcd_api.drawText(m_state.rit);
                break;
            }

            case ChangingRitStep:
            {
                this->m_lcd_api.setCursorVisible(true);
                this->m_lcd_api.drawText("RST: ");
                this->m_lcd_api.drawText(Defs::RitIncrements[m_state.rit_step_idx]);
                break;
            }
        }
    }

    void handle_input_impl(const LCDUI::InputEvent &ev)
    {
        switch (m_mode)
        {
            case StepTuning:
            {
                if (ev.ay > 0)
                {
                    m_state.vfo_freq += Defs::TuningIncrements[m_state.vfo_step_idx];
                    m_state.vfo_dirty = true;
                }

                else if (ev.ay < 0)
                {
                    m_state.vfo_freq -= Defs::TuningIncrements[m_state.vfo_step_idx];
                    m_state.vfo_dirty = true;
                }

                if (ev.b_OK)
                    m_mode = ChangingStep;

                else if (ev.b_A)
                    m_mode = ChangingDecade;

                else if (ev.b_B)
                    m_mode = RitTuning;

                break;
            }

            case ChangingStep:
            {
                if (ev.ay > 0 && m_state.vfo_step_idx < Defs::TuningIncrements.size() - 1)
                    ++m_state.vfo_step_idx;

                else if (ev.ay < 0 && m_state.vfo_step_idx > 0)
                    --m_state.vfo_step_idx;

                if (ev.b_OK)
                    m_mode = StepTuning;

                break;
            }

            case DecadeTuning:
            {
                int_fast32_t delta = 1;
                for (uint8_t i = 0; i < m_selected_decade; ++i)
                    delta *= 10;

                if (ev.ay > 0)
                {
                    m_state.vfo_freq += delta;
                    m_state.vfo_dirty = true;
                } else if (ev.ay < 0)
                {
                    m_state.vfo_freq -= delta;
                    m_state.vfo_dirty = true;
                }

                if (ev.b_A)
                    m_mode = ChangingDecade;
                else if (ev.b_OK)
                    m_mode = StepTuning;

                break;
            }

            case ChangingDecade:
            {
                if (ev.ay < 0 && m_selected_decade < m_max_decade)
                    ++m_selected_decade;
                else if (ev.ay > 0 && m_selected_decade > 0)
                    --m_selected_decade;

                if (!ev.b_A)
                    m_mode = DecadeTuning;

                break;
            }

            case RitTuning:
            {
                if (ev.ay > 0)
                {
                    m_state.rit += Defs::RitIncrements[m_state.rit_step_idx];
                    m_state.rit_dirty = true;
                }

                else if (ev.ay < 0)
                {
                    m_state.rit -= Defs::RitIncrements[m_state.rit_step_idx];
                    m_state.rit_dirty = true;
                }

                if (ev.b_OK)
                    m_mode = ChangingRitStep;

                else if (ev.b_B)
                {
                    m_state.rit       = 0;
                    m_state.rit_dirty = true;
                    m_mode            = StepTuning;
                }

                break;
            }

            case ChangingRitStep:
            {
                if (ev.ay > 0 && m_state.rit_step_idx < Defs::RitIncrements.size() - 1)
                    ++m_state.rit_step_idx;

                else if (ev.ay < 0 && m_state.rit_step_idx > 0)
                    --m_state.rit_step_idx;

                if (ev.b_OK)
                    m_mode = RitTuning;

                break;
            }
        }

        this->repaint();
    }

protected:
    TVFOState &m_state;
    VFOScreenMode m_mode;
    uint8_t m_selected_decade;
    static constexpr uint8_t m_max_decade = 6;
};

template <typename LCDApi>
constexpr VFOScreen<LCDApi> make_vfo_screen(LCDApi &api, const char *title, TVFOState &state)
{
    return VFOScreen<LCDApi>(api, title, state);
}

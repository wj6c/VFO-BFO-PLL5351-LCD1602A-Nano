#ifndef ARDUINOLCDAPI_H
#define ARDUINOLCDAPI_H

#include <LiquidCrystal.h>
#include "lcdui/lcdui.h"

class TArduinoLCDApi : public LCDUI::LCDApiBase<TArduinoLCDApi>
{
    using base = LCDUI::LCDApiBase<TArduinoLCDApi>;
public:
    TArduinoLCDApi(LiquidCrystal &display, uint8_t width, uint8_t height)
        : base(width, height)
        , m_lcd(display)
    {}

    virtual void clear_impl();
    virtual void drawText_impl(const char *text);
    virtual void drawText_impl(int_fast32_t val);
    virtual void setCursorVisible_impl(bool visible);
    virtual void setBlink_impl(bool blink);
    virtual void setCursorPosition_impl(uint8_t x, uint8_t y);

    virtual void int2string_impl(char *output, uint8_t max_len,
                                 int32_t num, bool pad = false );

private:
    LiquidCrystal &m_lcd;
};


#endif /* ARDUINOLCDAPI_H */

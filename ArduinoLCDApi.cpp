#include "ArduinoLCDApi.h"

void TArduinoLCDApi::clear_impl()
{
    m_lcd.clear();
}

void TArduinoLCDApi::drawText_impl(const char *text)
{
    m_lcd.print(text);
}

void TArduinoLCDApi::drawText_impl(int_fast32_t val)
{
    m_lcd.print(val);
}

void TArduinoLCDApi::setCursorVisible_impl(bool visible)
{
    if(visible)
        m_lcd.cursor();
    else
        m_lcd.noCursor();
}

void TArduinoLCDApi::setBlink_impl(bool blink)
{
    if(blink)
        m_lcd.blink();
    else
        m_lcd.noBlink();
}

void TArduinoLCDApi::setCursorPosition_impl(uint8_t x, uint8_t y)
{
    m_lcd.setCursor(x,y);
}

void TArduinoLCDApi::int2string_impl(char *output, uint8_t max_len,
                                     int32_t num, bool pad)
{
    (void) pad;

    String result(num);
    uint8_t l = result.length();
    if(l < max_len - 1)
    {
        for(uint8_t i = 0; i < max_len - 1 - l; i++)
            result = " " + result;
    }
    result.toCharArray(output, max_len);
};

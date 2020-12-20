#ifndef lcd_h
#define lcd_h

#include "terrarium.h"

#ifdef _NATIVE_
#include <stdint.h>
#else
#include <LiquidCrystal_I2C.h>

extern LiquidCrystal_I2C lcd;
#endif

void setup_lcd();
void lcd_clear_line(int8_t nr);
void lcd_printf(char *format, ...);

#endif /* lcd_h */
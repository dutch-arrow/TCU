#include "lcd.h"
#include <stdarg.h>

LiquidCrystal_I2C
    lcd(0x27, 16,
        2); // set the LCD address to 0x27 for a 16 chars and 2 line display

uint8_t degrees[8] = {0x6, 0x9, 0x9, 0x6, 0x0, 0x0, 0x0};
uint8_t sun[8] = {0x0, 0x15, 0x0e, 0x1f, 0x0e, 0x15, 0x0};
uint8_t moon[8] = {0x8, 0x6, 0x3, 0x3, 0x3, 0x6, 0x8};

void setup_lcd() {
    lcd.init(); // initialize the lcd
    lcd.createChar(0, degrees);
    lcd.createChar(1, sun);
    lcd.createChar(2, moon);
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Initializing..."));
}

void lcd_clear_line(int8_t nr) {
    lcd.setCursor(0, nr);
    lcd.print(F("                "));
    lcd.setCursor(0, nr);
}

void lcd_printf(const char *format, ...) {
    va_list arg;
    char tmp[50];
    va_start (arg, format);
    sprintf(tmp, format, arg);
    va_end(arg);
    if (strlen(tmp) > 16) {
        tmp[16] = '/0';
    }
    lcd.print(tmp);
}

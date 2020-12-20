#ifndef timer_h
#define timer_h

#ifdef _NATIVE_
#include <stdint.h>
#else
#include <RTClib.h>
#endif

#include "terrarium.h"

typedef struct {
    enum_device device;     // max 15
    int8_t index;           // max 7
    enum_on_off on_off;
    int8_t hour;            // max 23
    int8_t minute;          // max 59
    int8_t repeat_in_days;  // max 7 (=weekly)
    int8_t on_period;       // 1-59 seconds
} timer_t;

extern RTC_Millis rtc;
extern timer_t timers[NR_OF_TIMERS];

// Implemented in eeprom.c
int8_t read_timers();
int8_t update_timer(int8_t index);
void read_fan_period(int16_t *start_period, int16_t *period);
void update_fan_period(int16_t start_period, int16_t period);

void setup_all_timers();
void check_timers();
int8_t getTimerIndex(const char *device, const int ix, const char *on_off);
timer_t set_timer(enum_device device, const int ix, enum_on_off on_off, int repeat, int period);
void reset_timer(const char *device, const int ix, const char *on_off, int8_t h, int8_t m, int8_t repeat, int8_t period);
void reset_fan_period(int16_t start_period, int16_t period);
int8_t get_timer(const char *device, const int ix, const char *on_off, timer_t *t);
int8_t get_nr_of_timers_per_device(enum_device device);
int8_t get_timers_as_json(const enum_device device, char **json);
char *timer_to_json(timer_t t);
void check_timers(DateTime *now);

#endif /* timer_h */

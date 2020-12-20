#ifndef eeprom_h
#define eeprom_h

// Arduino Uno Wifi Rev2 has only 256 bytes EEPROM (0 - 255)
// Each timer is compressed into 4 bytes.
// 46 timers = 184 bytes : byte 1 - 184
// fan_period: byte 185-186 (2 bytes)
// free: 187-189 (3 bytes)
// Each program is 33 bytes.
// 2 programs = 66 bytes : byte 190 - 255
// Total: 210 bytes. Free: 46 bytes
#define START_INDEX_TIMERS      1
#define START_INDEX_REST      185
#define START_INDEX_PROGRAMS  190

#ifdef _NATIVE_
#include <stdint.h>
#else
#include <EEPROM.h>
#endif

#include "terrarium.h"
#include "timer.h"
#include "sensors.h"

int8_t update_timer(int8_t index);

void write_timer_int(uint32_t tbits, int ix);
uint32_t read_timer_int(int ix);
void update_timer_int(uint32_t tbits, int ix);
void cvt_timer_to_uint32(const timer_t t, uint32_t *tbits);
void cvt_uint32_to_timer(const uint32_t tbits, timer_t *t);
void read_fan_period(int16_t *start_period, int16_t *period);
void update_fan_period(int16_t start_period, int16_t period);

int8_t update_program(int8_t index);

void write_rule_set_int(rule_set_t p, int ix);
rule_set_t read_rule_set_int(int ix);
void update_program_int(rule_set_t p, int ix);

#endif /* eeprom_h */
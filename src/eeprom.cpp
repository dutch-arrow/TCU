#include "eeprom.h"

/*
* TIMERS
*
* device : bits  0 -  3 (4 bits)
* index:   bits  4 -  6 (3 bits, >> 4 & 0x7) 
* on_off:  bits  7 -  8 (1 bit , >> 7 & 0x1)
* hour:    bits  9 - 13 (5 bits, >> 8 & 0x1F)
* minutes: bits 14 - 19 (6 bits, >> 13 & 0x3F)
* repeat:  bits 20 - 22 (3 bits, >> 19 & 0x7)
* onperiod:bits 23 - 28 (6 bits, >> 22 & 0x3F)
* notused: bits 29 - 31 (3 bits)
*/
void write_timer_int(uint32_t tbits, int bix) {
    EEPROM.write(bix, (byte)tbits);
    EEPROM.write(bix + 1, (byte)(tbits >> 8 & 0xFF));
    EEPROM.write(bix + 2, (byte)(tbits >> 16 & 0xFF));
    EEPROM.write(bix + 3, (byte)(tbits >> 24 & 0xFF));
}

uint32_t read_timer_int(int bix) {
    uint32_t tint = EEPROM.read(bix);
    tint += ((uint32_t)EEPROM.read(bix + 1)) << 8;
    tint += ((uint32_t)EEPROM.read(bix + 2)) << 16;
    tint += ((uint32_t)EEPROM.read(bix + 3)) << 24;
    return tint;
}

void update_timer_int(uint32_t tbits, int bix) {
    EEPROM.update(bix, (byte)tbits);
    EEPROM.update(bix + 1, (byte)(tbits >> 8 & 0xFF));
    EEPROM.update(bix + 2, (byte)(tbits >> 16 & 0xFF));
    EEPROM.update(bix + 3, (byte)(tbits >> 24 & 0xFF));
}

void cvt_timer_to_uint32(const timer_t t, uint32_t *tbits) {
    *tbits = t.device;
    *tbits += ((uint32_t)t.index) << 4;
    *tbits += ((uint32_t)t.on_off) << 7;
    *tbits += ((uint32_t)t.hour) << 8;
    *tbits += ((uint32_t)t.minute) << 13;
    *tbits += ((uint32_t)t.repeat_in_days) << 19;
    *tbits += ((uint32_t)t.on_period) << 22;
 }

void cvt_uint32_to_timer(const uint32_t tbits, timer_t *t) {
    t->device = (enum_device)(tbits & 0x0F);
    t->index = tbits >> 4 & 0x7;
    t->on_off = (enum_on_off)(tbits >> 7 & 0x1);
    t->hour = tbits >> 8 & 0x1F;
    t->minute = tbits >> 13 & 0x3F;
    t->repeat_in_days = tbits >> 19 & 0x7;
    t->on_period = tbits >> 22 & 0x3F;
}

int8_t read_timers() {
    // Read all timer settings from EEPROM
    // after checking if it was written before
    int8_t rc = 0;
    if (EEPROM.read(0) == 0xAA) {
        int bix = START_INDEX_TIMERS;
        for (int i = 0; i < NR_OF_TIMERS; i++) {
            uint32_t tbits = read_timer_int(bix);
            bix += 4;
            timer_t t;
            cvt_uint32_to_timer(tbits, &t);
            timers[i] = t;
        }
    } else {
        rc = 1;
    }
    return rc;
}

void read_fan_period(int16_t *start_period, int16_t *period) {
    int bix = START_INDEX_REST;
    *start_period = ((int16_t)EEPROM.read(bix) << 8) + EEPROM.read(bix+1);
    bix += 2;
    *period = ((int16_t)EEPROM.read(bix) << 8) + EEPROM.read(bix + 1);
}

void update_fan_period(int16_t start_period, int16_t period) {
    int bix = START_INDEX_REST;
    EEPROM.update(bix, start_period >> 8);
    EEPROM.update(bix + 1, start_period & 0xFF);
    bix += 2;
    EEPROM.update(bix, period >> 8);
    EEPROM.update(bix + 1, period & 0xFF);
}

int8_t update_timer(int8_t index) {
    int bix = START_INDEX_TIMERS + (index * 4);
    uint32_t tbits;
    cvt_timer_to_uint32(timers[index], &tbits);
    update_timer_int(tbits, bix);
    return 0;
}

/*
* PROGRAMS
*/
void write_rule_set_int(rule_set_t p, int bix) {
    EEPROM.write(bix, p.active);
    EEPROM.write(bix + 1, p.time_of_day >> 8 & 0xFF);
    EEPROM.write(bix + 2, p.time_of_day & 0xFF);
    EEPROM.write(bix + 3, p.temp_ideal);
    EEPROM.write(bix + 4, p.hum_ideal);
    int ix = 5;
    for (int i = 0; i < 4; i++) {
        EEPROM.write(bix + ix, p.program_data[i].value);
        ix++;
        for (int a = 0; a < 2; a++) {
            EEPROM.write(bix + ix, p.program_data[i].actions[a].device);
            ix++;
            EEPROM.write(bix + ix, p.program_data[i].actions[a].on_period >> 8 & 0xFF);
            ix++;
            EEPROM.write(bix + ix, p.program_data[i].actions[a].on_period & 0xFF);
            ix++;
        }
    }
}

rule_set_t read_program_int(int bix) {
    rule_set_t p;
    p.active = EEPROM.read(bix);
    p.time_of_day = (((int16_t)EEPROM.read(bix + 1)) << 8) +  EEPROM.read(bix + 2);
    p.temp_ideal = EEPROM.read(bix + 3);
    p.hum_ideal = EEPROM.read(bix + 4);
    int ix = 5;
    for (int i = 0; i < 4; i++) {
        p.program_data[i].value = EEPROM.read(bix + ix);
        ix++;
        for (int a = 0; a < 2; a++) {
            p.program_data[i].actions[a].device = (enum_device)EEPROM.read(bix + ix);
            ix++;
            p.program_data[i].actions[a].on_period = (((int16_t)EEPROM.read(bix + ix)) << 8) + EEPROM.read(bix + ix + 1);
            ix += 2;
        }
    }
    return p;
}

void update_program_int(rule_set_t p, int bix) {
    EEPROM.update(bix, p.active);
    EEPROM.update(bix + 1, p.time_of_day >> 8 & 0xFF);
    EEPROM.update(bix + 2, p.time_of_day & 0xFF);
    EEPROM.update(bix + 3, p.temp_ideal);
    EEPROM.update(bix + 4, p.hum_ideal);
    int ix = 5;
    for (int i = 0; i < 4; i++) {
        EEPROM.update(bix + ix, p.program_data[i].value);
        ix++;
        for (int a = 0; a < 2; a++) {
            EEPROM.update(bix + ix, p.program_data[i].actions[a].device);
            ix++;
            EEPROM.update(bix + ix, p.program_data[i].actions[a].on_period >> 8 & 0xFF);
            ix++;
            EEPROM.update(bix + ix, p.program_data[i].actions[a].on_period & 0xFF);
            ix++;
        }
    }
}

int8_t read_programs() {
    // Read all timer settings from EEPROM
    // after checking if it was written before
    int8_t rc = 0;
    if (EEPROM.read(0) == 0xAA) {
        int bix = START_INDEX_PROGRAMS;
        for (int i = 0; i < NR_OF_PROGRAMS; i++) {
            rule_set_t p = read_program_int(bix);
            bix += 33;
            programs[i] = p;
        }
    } else {
        rc = 1;
    }
    return rc;
}

int8_t update_program(int8_t index) {
    update_program_int(programs[index], START_INDEX_PROGRAMS + (index * 33));
    return 0;
}

#include "timer.h"

RTC_Millis rtc;
extern int8_t trace_on;

timer_t timers[NR_OF_TIMERS];
extern int16_t fan_period;
extern int16_t fan_start_period;

void check_timers() {
    DateTime now = rtc.now();
    int now_time = now.hour() * 60 + now.minute(); // the minute of the day (0 - 1440)
    enum_device prev_dev = no_device; // timer always has a device !
    int8_t prev_ix = 0; // timer index start with 1 !
    for (int i = 0; i < NR_OF_TIMERS; i++) {
        timer_t t = timers[i];
        enum_device dev = t.device;
        int ix = t.index;
        if (dev != prev_dev) {
            prev_dev = dev;
            prev_ix = 0;
        }
        if (t.index > prev_ix) { // next on-timer index
            prev_ix = ix;
            int on_timer_time = t.hour * 60 + t.minute;
            int off_timer_time = timers[i + 1].hour * 60 + timers[i + 1].minute;
            int end_time = (t.on_period == 0 ? 0 : rtc.now().unixtime() + t.on_period);
            if (now_time >= on_timer_time && now_time <= off_timer_time) {
                set_device_state(t.device, t.on_off, end_time);
            } else if (now_time < on_timer_time && now_time < off_timer_time && off_timer_time < on_timer_time) { // timer on-off interval passed midnight
                set_device_state(t.device, t.on_off, end_time);
            }
        }
        i++; // lets skip the off-timer
    }
}

void setup_all_timers() {
    read_timers();
    if (trace_on == 1) {
        Serial1.println(F("All timers read from EEPROM"));
    }
    check_timers();
    read_fan_period(&fan_start_period, &fan_period);
}

void check_timers(DateTime *now) {
    for (int i = 0; i < NR_OF_TIMERS; i++) {
        timer_t t = timers[i];
        if (t.device != no_device) {
            if (now->hour() == t.hour  && now->minute() == t.minute ) {
                if (t.on_period == 0) {
                    set_device_state(t.device, t.on_off, 0);
                } else {
                    set_device_state(t.device, t.on_off, rtc.now().unixtime() + t.on_period);
                }
            }
        }
    }
}

int8_t getTimerIndex(const char *device, const int ix, const char *on_off) {
    int8_t index = 0;
    if (strcmp(device, "light1") == 0) {
        index =  0 + (ix - 1) * 2 + (strcmp(on_off, "on") == 0 ? 0 : 1);
    } else if (strcmp(device, "light2") == 0) {
        index =  4 + (ix - 1) * 2 + (strcmp(on_off, "on") == 0 ? 0 : 1);
    } else if (strcmp(device, "light3") == 0) {
        index =  8 + (ix - 1) * 2 + (strcmp(on_off, "on") == 0 ? 0 : 1);
    } else if (strcmp(device, "light4") == 0) {
        index = 12 + (ix - 1) * 2 + (strcmp(on_off, "on") == 0 ? 0 : 1);
    } else if (strcmp(device, "light5") == 0) {
        index = 16 + (ix - 1) * 2 + (strcmp(on_off, "on") == 0 ? 0 : 1);
    } else if (strcmp(device, "light6") == 0) {
        index = 18 + (ix - 1) * 2 + (strcmp(on_off, "on") == 0 ? 0 : 1);
    } else if (strcmp(device, "pump") == 0) {
        index = 20 + (ix - 1) * 2 + (strcmp(on_off, "on") == 0 ? 0 : 1);
    } else if (strcmp(device, "sprayer") == 0) {
        index = 24 + (ix - 1) * 2 + (strcmp(on_off, "on") == 0 ? 0 : 1);
    } else if (strcmp(device, "mist") == 0) {
        index = 28 + (ix - 1) * 2 + (strcmp(on_off, "on") == 0 ? 0 : 1);
    } else if (strcmp(device, "fan_in") == 0) {
        index = 32 + (ix - 1) * 2 + (strcmp(on_off, "on") == 0 ? 0 : 1);
    } else if (strcmp(device, "fan_out") == 0) {
        index = 38 + (ix - 1) * 2 + (strcmp(on_off, "on") == 0 ? 0 : 1);
    }
    return index;
}

int8_t getTimerIndex(enum_device device, const int ix, enum_on_off on_off) {
    int8_t index = 0;
    switch (device) {
        case device_light1: {
            index = (ix - 1) * 2 + (on_off == on ? 0 : 1);
            break;
        }
        case device_light2: {
            index = 4 + (ix - 1) * 2 + (on_off == on ? 0 : 1);
            break;
        }
        case device_light3: {
            index = 8 + (ix - 1) * 2 + (on_off == on ? 0 : 1);
            break;
        }
        case device_light4: {
            index = 12 + (ix - 1) * 2 + (on_off == on ? 0 : 1);
            break;
        }
        case device_light5: {
            index = 16 + (ix - 1) * 2 + (on_off == on ? 0 : 1);
            break;
        }
        case device_light6: {
            index = 18 + (ix - 1) * 2 + (on_off == on ? 0 : 1);
            break;
        }
        case device_pump: {
            index = 20 + (ix - 1) * 2 + (on_off == on ? 0 : 1);
            break;
        }
        case device_sprayer: {
            index = 24 + (ix - 1) * 2 + (on_off == on ? 0 : 1);
            break;
        }
        case device_mist: {
            index = 28 + (ix - 1) * 2 + (on_off == on ? 0 : 1);
            break;
        }
        case device_fan_in: {
            index = 32 + (ix - 1) * 2 + (on_off == on ? 0 : 1);
            break;
        }
        case device_fan_out: {
            index = 38 + (ix - 1) * 2 + (on_off == on ? 0 : 1);
            break;
        }
    }
    return index;
}

timer_t set_timer(enum_device device, const int ix, enum_on_off on_off, int repeat, int period) {
    timer_t t;
    t.device = device;
    t.index = ix;
    t.on_off = on_off;
    t.hour = 24;
    t.minute = 60;
    t.repeat_in_days = repeat;
    t.on_period= period;
    return t;
}

void reset_timer(const char *device, const int ix, const char *on_off, int8_t h, int8_t m, int8_t repeat, int8_t period) {
    int8_t index = getTimerIndex(device, ix, on_off);
    if (trace_on == 1) {
        Serial1.print("Reset timer of ");
        Serial1.print(device);
        Serial1.print(" on index = ");
        Serial1.println(index);
    }
    timers[index].device = cvt_device_str2enum(device);
    timers[index].index = ix;
    timers[index].on_off = cvt_on_off_str2enum(on_off);
    timers[index].hour = h;
    timers[index].minute = m;
    timers[index].repeat_in_days = repeat;
    timers[index].on_period = period;
    update_timer(index);
}

void reset_fan_period(int16_t start_period, int16_t period) { 
    update_fan_period(start_period, period);
}

char * timer_to_json(timer_t t) {
    char *tmp = (char *)malloc(sizeof(char) * 120);
    if (tmp == NULL) {
        return "Out of heap space";
    }
    sprintf(tmp,
            F("{\"device\":\"%s\",\"index\":%d,\"on_off\":\"%s\",\"hour\":%d,\"minute\":%d,\"repeat\":%d,\"period\":%d}"),
            cvt_enum_device_to_string(t.device), t.index,
            cvt_enum_on_off_to_string(t.on_off), t.hour, t.minute,
            t.repeat_in_days, t.on_period);
    return tmp;
}

int8_t get_nr_of_timers_per_device(enum_device device) {
    switch (device) {
        case device_light1:
        case device_light2:
        case device_light3:
        case device_light4: {
            return 4; // number of on and off moments
            break;
        }
        case device_light5:
        case device_light6: {
            return 2; // number of on and off moments
            break;
        }
        case device_pump:
        case device_sprayer:
        case device_mist: {
            return 4;
            break;
        }
        case device_fan_in: {
            return 6; // number of on and off moments
            break;
        }
        case device_fan_out: {
            return 8;
            break;
        }
    }
    return 0;
}

int8_t get_timers_as_json(const enum_device device, char ** json) {
    int8_t rc = 0;
    int8_t ix = getTimerIndex(device, 1, on);
    int8_t n = get_nr_of_timers_per_device(device);
    if (trace_on == 1) {
        Serial1.print("Device: ");
        Serial1.print(cvt_enum_device_to_string(device));
        Serial1.print(" -> ");
        Serial1.print(n);
        Serial1.println(" timers");
    }
    strcpy(*json, "[");
    for (int i = 0; i < n; i++) {
        timer_t t = timers[ix + i];
        char *tmp = timer_to_json(t);
        strcat(*json, tmp);
        free(tmp);
        if (i != (n - 1)) {
            strcat(*json, ",");
        }
    }
    strcat(*json, "]");
    return rc;
}

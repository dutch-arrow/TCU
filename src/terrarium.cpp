#include "terrarium.h"
#include "sensors.h"

device_state device_states[NR_OF_DEVICES];
extern int8_t trace_on;
extern int16_t fan_start_period;
extern int16_t fan_period;
extern int32_t fan_start_dt;
extern int32_t drying_period_end_dt;
extern rule_set_t programs[NR_OF_PROGRAMS];
extern int8_t prognr;

/*
 * Lights are connected to a low-active relais.
 * This means that the pin should be high (5V) in the rest phase.
 * If the relais should be activated the pin should be low (0V).
 */
void setup_lights() {
    pinMode(pin_light1, OUTPUT);
    digitalWrite(pin_light1, HIGH);
    device_states[device_light1].on_off = off;
    pinMode(pin_light2, OUTPUT);
    digitalWrite(pin_light2, HIGH);
    device_states[device_light2].on_off = off;
    pinMode(pin_light3, OUTPUT);
    digitalWrite(pin_light3, HIGH);
    device_states[device_light3].on_off = off;
    pinMode(pin_light4, OUTPUT);
    digitalWrite(pin_light4, HIGH);
    device_states[device_light4].on_off = off;
    pinMode(pin_light5, OUTPUT);
    digitalWrite(pin_light5, HIGH);
    device_states[device_light5].on_off = off;
    pinMode(pin_light6, OUTPUT);
    digitalWrite(pin_light6, HIGH);
    device_states[device_light6].on_off = off;
    pinMode(pin_light7, OUTPUT);
    digitalWrite(pin_light7, HIGH);
    pinMode(pin_light8, OUTPUT);
    digitalWrite(pin_light8, HIGH);
}

/*
 * The sprayer, mist and fans are connected to a hight-active relais.
 * This means that the pin should be low (0V) in the rest phase.
 * If the relais should be activated the pin should be high (5V).
 */
void setup_sprayer() {
    pinMode(pin_sprayer, OUTPUT);
    digitalWrite(pin_sprayer, LOW);
    device_states[device_sprayer].on_off = off;
}

void setup_mist() {
    pinMode(pin_mist, OUTPUT);
    digitalWrite(pin_mist, LOW);
    device_states[device_mist].on_off = off;
}

void setup_pump() {
    pinMode(pin_pump, OUTPUT);
    digitalWrite(pin_pump, LOW);
    device_states[device_pump].on_off = off;
}

void setup_fans() {
    pinMode(pin_fan_in, OUTPUT);
    digitalWrite(pin_fan_in, LOW);
    device_states[device_fan_in].on_off = off;
    pinMode(pin_fan_out, OUTPUT);
    digitalWrite(pin_fan_out, LOW);
    device_states[device_fan_out].on_off = off;
}

void set_device_state(enum_device device, enum_on_off on_off, int32_t end_time) {
    if (device != no_device) {
        switch (device) {
        case device_light1: {
            digitalWrite(pin_light1, (on_off == on ? LOW : HIGH));
            device_states[device_light1].on_off = on_off;
            device_states[device_light1].end_time = end_time;
            break;
        }
        case device_light2: {
            digitalWrite(pin_light2, (on_off == on ? LOW : HIGH));
            device_states[device_light2].on_off = on_off;
            device_states[device_light2].end_time = end_time;
            break;
        }
        case device_light3: {
            digitalWrite(pin_light3, (on_off == on ? LOW : HIGH));
            device_states[device_light3].on_off = on_off;
            device_states[device_light3].end_time = end_time;
            break;
        }
        case device_light4: {
            digitalWrite(pin_light4, (on_off == on ? LOW : HIGH));
            device_states[device_light4].on_off = on_off;
            device_states[device_light4].end_time = end_time;
            break;
        }
        case device_light5: {
            digitalWrite(pin_light5, (on_off == on ? LOW : HIGH));
            device_states[device_light5].on_off = on_off;
            device_states[device_light5].end_time = end_time;
            break;
        }
        case device_light6: {
            digitalWrite(pin_light6, (on_off == on ? LOW : HIGH));
            device_states[device_light6].on_off = on_off;
            device_states[device_light6].end_time = end_time;
            break;
        }
        case device_pump: {
            digitalWrite(pin_pump, (on_off == on ? HIGH : LOW));
            device_states[device_pump].on_off = on_off;
            device_states[device_pump].end_time = end_time;
            break;
        }
        case device_sprayer: {
            digitalWrite(pin_sprayer, (on_off == on ? HIGH : LOW));
            device_states[device_sprayer].on_off = on_off;
            device_states[device_sprayer].end_time = end_time;
            int32_t now = rtc.now().unixtime();
            if (on_off == on) {
                drying_period_end_dt = end_time + ((fan_start_period + fan_period + 5) * 60);
                programs[prognr].active = off; // stop rule checking during spraying and drying
            }
            if (on_off == off) {
                fan_start_dt = now + (fan_start_period * 60);
                if (trace_on == 1) {
                    Serial1.print(F("Drying will start at "));
                    DateTime dt = DateTime(fan_start_dt);
                    Serial1.print(dt.hour());
                    Serial1.print(F(":"));
                    Serial1.println(dt.minute());
                }
            }
            break;
        }
        case device_mist: {
            digitalWrite(pin_mist, (on_off == on ? HIGH : LOW));
            device_states[device_mist].on_off = on_off;
            device_states[device_mist].end_time = end_time;
            break;
        }
        case device_fan_in: {
            digitalWrite(pin_fan_in, (on_off == on ? HIGH : LOW));
            device_states[device_fan_in].on_off = on_off;
            device_states[device_fan_in].end_time = end_time;
            break;
        }
        case device_fan_out: {
            digitalWrite(pin_fan_out, (on_off == on ? HIGH : LOW));
            device_states[device_fan_out].on_off = on_off;
            device_states[device_fan_out].end_time = end_time;
            break;
        }
        default: {
            if (trace_on == 1) {
                Serial1.println(F("Device not recognized."));
            }
        }
        }
        if (trace_on == 1) {
            Serial1.print(F("Device "));
            Serial1.print(cvt_enum_device_to_string(device));
            Serial1.print(F(" is switched "));
            Serial1.print(cvt_enum_on_off_to_string(on_off));
            if (on_off == on) {
                if (end_time == -1) {
                    Serial1.print(F(" untill ideal value is reached"));
                } else if (end_time == 0) {
                    Serial1.print(F(" permanently"));
                } else {
                    DateTime dt = DateTime(end_time);
                    char tmp[30];
                    sprintf(tmp, F(" untill %02d:%02d:%02d"), dt.hour(), dt.minute(), dt.second());
                    Serial1.print(tmp);
                }
            }
            Serial1.println();
        }
    }
}

char * cvt_enum_device_to_string(enum_device device) {
    switch(device) {
        case no_device:
            return F("no device");
        case device_light1:
            return F("light1");
        case device_light2:
            return F("light2");
        case device_light3:
            return F("light3");
        case device_light4:
            return F("light4");
        case device_light5:
            return F("light5");
        case device_light6:
            return F("light6");
        case device_sprayer:
            return F("sprayer");
        case device_mist:
            return F("mist");
        case device_pump:
            return F("pump");
        case device_fan_in:
            return F("fan_in");
        case device_fan_out:
            return F("fan_out");
    }
}

char *cvt_enum_on_off_to_string(enum_on_off on_off) {
    switch(on_off) {
        case on:
            return "on";
        case off:
            return "off";
            
    }
}

int8_t get_nr_of_timers_per_device(enum_device device);

void get_properties(char **json) {
    char temp[100];
    strcpy(*json, F("{"));
    sprintf(temp, F("\"nr_of_timers\":%d,"), NR_OF_TIMERS);
    strcat(*json, temp);
    sprintf(temp, F("\"nr_of_programs\":%d,"), NR_OF_PROGRAMS);
    strcat(*json, temp);
    strcat(*json, F("\"devices\": ["));
    for (int i = 0; i < NR_OF_DEVICES; i++) {
        sprintf(temp, F("{\"device\":\"%s\", \"nr_of_timers\":%d}"), cvt_enum_device_to_string((enum_device)i), get_nr_of_timers_per_device((enum_device)i));
        strcat(*json, temp);
        if (i != NR_OF_DEVICES - 1) {
            strcat(*json, F(","));
        }
    }
    strcat(*json, F("]}"));
}

void get_device_state(char **json) {
    char temp[100];
    strcpy(*json, F("["));
    for (int i = 1; i < NR_OF_DEVICES; i++) {
        if (device_states[i].end_time > 0) { // an endtime is defined
            DateTime dt = DateTime(device_states[i].end_time); // seconds since 1-1-2000
            sprintf(temp, F("{\"device\":\"%s\",\"state\":\"%s\",\"end_time\":\"%02d:%02d:%02d\"}"),
                    cvt_enum_device_to_string((enum_device)i),
                    (device_states[i].on_off == on ? F("on") : F("off")),
                    (device_states[i].on_off == on ? dt.hour() : 0),
                    (device_states[i].on_off == on ? dt.minute() : 0),
                    (device_states[i].on_off == on ? dt.second() : 0));
        } else if (device_states[i].end_time == 0) { // endless
            sprintf(temp, F("{\"device\":\"%s\",\"state\":\"%s\",\"end_time\":\"no endtime\"}"),
                    cvt_enum_device_to_string((enum_device)i),
                    (device_states[i].on_off == on ? F("on") : F("off")));
        } else if (device_states[i].end_time == -1) { // untill ideal temperature value is reached
            sprintf(temp, F("{\"device\":\"%s\",\"state\":\"%s\",\"end_time\":\"until ideal temperature is reached\"}"),
                    cvt_enum_device_to_string((enum_device)i),
                    (device_states[i].on_off == on ? F("on") : F("off")));
        } else if (device_states[i].end_time == -2) { // untill ideal value is reached
            sprintf(temp, F("{\"device\":\"%s\",\"state\":\"%s\",\"end_time\":\"until ideal humidity is reached\"}"),
                    cvt_enum_device_to_string((enum_device)i),
                    (device_states[i].on_off == on ? F("on") : F("off")));
        }
        strcat(*json, temp);
        if (i != NR_OF_DEVICES - 1) {
            strcat(*json, F(","));
        }
    }
    strcat(*json, F("]"));
}

int8_t check_state_rules() {
    int8_t rc = 0;
    uint32_t secs = rtc.now().unixtime(); // in seconds since 1970-01-01
    // go through all device states and check the end times
    for (int i = 1; i < NR_OF_DEVICES; i++) {
        if (device_states[i].end_time > 0 && secs > device_states[i].end_time) {
            // if it is past, switch device off
            if (trace_on == 1) {
                Serial.print(F("[check_state_rules] "));
            }
            set_device_state((enum_device)i, off, 0);
        }
    }
    return rc;
}

enum_device cvt_device_str2enum(const char *device) {
    if (strcmp(device, F("no device")) == 0) {
        return no_device;
    } else if (strcmp(device, F("light1")) == 0) {
        return device_light1;
    } else if (strcmp(device, F("light2")) == 0) {
        return device_light2;
    } else if (strcmp(device, F("light3")) == 0) {
        return device_light3;
    } else if (strcmp(device, F("light4")) == 0) {
        return device_light4;
    } else if (strcmp(device, F("light5")) == 0) {
        return device_light5;
    } else if (strcmp(device, F("light6")) == 0) {
        return device_light6;
    } else if (strcmp(device, F("pump")) == 0) {
        return device_pump;
    } else if (strcmp(device, F("sprayer")) == 0) {
        return device_sprayer;
    } else if (strcmp(device, F("mist")) == 0) {
        return device_mist;
    } else if (strcmp(device, F("fan_in")) == 0) {
        return device_fan_in;
    } else if (strcmp(device, F("fan_out")) == 0) {
        return device_fan_out;
    }
}

enum_on_off cvt_on_off_str2enum(const char *on_off) {
    if (strcmp(on_off, F("on")) == 0) {
        return on;
    } else {
        return off;
    }
}

#include "sensors.h"

dht DHT;
rule_set_t programs[NR_OF_PROGRAMS];
extern int8_t prognr;
extern int8_t prevPrognr;
int8_t switchProgram;
extern rule_set_t programs[NR_OF_PROGRAMS];
extern int8_t trace_on;
extern int8_t in_test;
extern int8_t test_room_humidity;
extern int8_t test_room_temperature;
extern int8_t test_terrarium_humidity;
extern int8_t test_terrarium_temperature;
extern int8_t room_sensor; //0=no room sensor available
extern int8_t terrarium_sensor; //0=no terrarium sensor available

int8_t read_sensor(int8_t r_or_t, int8_t *h, int8_t *t) {
    int8_t sensor_pin;
    int8_t sensor;
    int8_t test_temperature;
    int8_t test_humidity;
    if (r_or_t == 1) { // terrarium sensor
        sensor_pin = pin_sensor_in;
        sensor = terrarium_sensor;
        test_temperature = test_terrarium_temperature;
        test_humidity = test_terrarium_humidity;
    } else { // room sensor
        sensor_pin = pin_sensor_out;
        sensor = room_sensor;
        test_temperature = test_room_temperature;
        test_humidity = test_room_humidity;
    }
    int8_t rc = 0;
    if (in_test == 0) {
        if (sensor == 1) { // if sensor is connected
            if (*h == -1 && *t == -1) {
                return rc;
            } else {
                int chk = DHT.read2301(sensor_pin);
                // int tms = 1;
                // // Read it max 10 times if in error 
                // while (chk != DHTLIB_OK && tms < 10) {
                //     delay(100);
                //     chk = DHT.read2301(sensor_pin);
                //     tms++;
                // }
                if (chk == DHTLIB_OK) {
                    if (*h != -1) { // -1 means suppress control on humidity
                        *h = int(DHT.humidity);     // cut off decimals
                    }
                    if (*t != -1) { // -1 means suppress control on temperture
                        *t = int(DHT.temperature); // cut off decimals
                    }
                } else {
                    switch (chk) {
                    case DHTLIB_ERROR_CHECKSUM:
                        rc = 1;
                        break;
                    case DHTLIB_ERROR_TIMEOUT:
                        rc = 2;
                        break;
                    case DHTLIB_INVALID_VALUE:
                        rc = 3;
                    }
                    *h = 0;
                    *t = 0;
                }
            }
        } else { // sensor is not connected
            *h = 0;
            *t = 0;
        }
    } else {
        *h = test_humidity;
        *t = test_temperature;
    }
    return rc;
}

int8_t read_room_sensor(int8_t *h, int8_t *t) {
    return read_sensor(2,h, t);
}

int8_t read_terrarium_sensor(int8_t *h, int8_t *t) {
    return read_sensor(1, h, t);
}

int8_t read_sensors_as_json(char **json) {
    int8_t rc = 0;
    char tmp[100];
    int8_t hum;
    int8_t temp;
    char dt_str[] = F("DD-MMM-YYYY hh:mm");
    sprintf(tmp, F("{\"clock\":\"%s\","),rtc.now().toString(dt_str));
    strcpy(*json, tmp);
    if (prognr != -1) {
        if (programs[prognr].active == off) {
            sprintf(tmp, F("\"rules\":\"off\",\"sensors\":"));
        } else {
            sprintf(tmp, F("\"rules\":\"%s\",\"sensors\":"), prognr == 0 ? "day" : "night");
        }
    } else {
        sprintf(tmp, F("\"rules\":\"no\",\"sensors\":"));
    }
    strcat(*json, tmp);
    read_room_sensor(&hum, &temp);
    sprintf(
        tmp,
        F("[{\"location\":\"room\",\"humidity\":%d,\"temperature\":%d},"),
        hum, temp);
    strcat(*json, tmp);
    read_terrarium_sensor(&hum, &temp);
    sprintf(tmp,
            F("{\"location\":\"terrarium\",\"humidity\":%d,\"temperature\":"
                "%d}]}"),
            hum, temp);
    strcat(*json, tmp);
    return rc;
}

void setup_rules() { 
    read_programs();
    if (trace_on == 1) {
        Serial1.println(F("All rules read from EEPROM"));
    }
    // Set the current rule
    check_program();
}

void check_program() {
    // Set the current rule
    DateTime now = rtc.now();
    int16_t now_time = now.hour() * 60 + now.minute();
    if (now_time >= programs[0].time_of_day && now_time <= programs[1].time_of_day) {
        // Day program should be active
        prognr = 0;
        if (prevPrognr != prognr) {
            switchProgram = (prevPrognr == -2 ? 0 : 1); // 0=no switch/power up
            prevPrognr = prognr;
        } else {
            switchProgram = 0;
        }
    } else {
        // Night program should be active
        prognr = 1;
        if (prevPrognr != prognr) {
            switchProgram = (prevPrognr == -2 ? 0 : 1); // 0=no switch/power up
            prevPrognr = prognr;
        } else {
            switchProgram = 0;
        }
    }
}

int8_t reset_rules(int ix) {
    update_program(ix);
}

int8_t check_sensor_rules(int8_t h_terrarium, int8_t t_terrarium, int8_t h_room, int8_t t_room) {
    int8_t rc = 0;
    if (programs[prognr].active == off || terrarium_sensor == 0) {
        return 0;
    }
    int8_t ht, tt, hr, tr;
    if (h_terrarium == 0 && t_terrarium == 0) {
        int rc = read_sensor(2, &ht, &tt);
        int cnt = 0;
        while ((rc != 0 || ht < 40 || tt < 15 || tt > 40) && cnt < 10) {
            delay(1000);
            rc = read_sensor(1, &ht, &tt);
            cnt++;
        }
        if (cnt >= 10) {
            return rc;
        }
    } else {
        ht = h_terrarium;
        tt = t_terrarium;
    }
    if (h_room == 0 && t_room == 0 && room_sensor == 1) {
        int rc = read_sensor(1, &hr, &tr);
        int cnt = 0;
        while ((rc != 0 || hr < 40 || tr < 15 || tr > 40) && cnt < 10) {
            delay(1000);
            rc = read_sensor(1, &hr, &tr);
            cnt++;
        }
        if (cnt >= 10) {
            return rc;
        }
    } else {
        hr = h_room;
        tr = t_room;
    }
    if (trace_on == 1) {
        Serial1.println(F("[check_sensor_rules ] "));
    }
    for (int p = 0; p < 2; p++) { // temperature (0) and humidity (1)
        if (p == 1 && ht == -1) { // skip the humidity rules
            continue;
        }
        if (p == 0 && tt == -1) { // skip the temperature rules
            continue;
        }
        int8_t t_sensor_value = (p == 0 ? tt : ht);
        int8_t r_sensor_value = (p == 0 ? tr : hr);
        int8_t ideal_value = (p == 0 ? programs[prognr].temp_ideal : programs[prognr].hum_ideal);
        if (trace_on == 1) {
            Serial1.print(F(" ideal="));
            Serial1.print(ideal_value);
            Serial1.print(F(" room sensor: "));
            Serial1.print(room_sensor == 0 ? "not available" : "available");
            Serial1.print(F(" room="));
            Serial1.print(r_sensor_value);
            Serial1.print(F(" terrarium="));
            Serial1.println(t_sensor_value);
        }
        if (ideal_value == 0) {
            break;
        }
        // Read sensor value and see if it needs to be acted upon
        // if sensor value = ideal, switch device that is on to off when its
        // end_time is -1 (temp) or -2 (hum)
        if (t_sensor_value == ideal_value || switchProgram == 1) {
            if (trace_on == 1) {
                if (switchProgram == 1) {
                    Serial1.println(F("program switched"));
                } else {
                    Serial1.println(F("data value = Ideal value"));
                }
            }
            int32_t now = rtc.now().unixtime();
            for (int i = device_pump; i < NR_OF_DEVICES; i++) {
                if (device_states[i].on_off == on && p == 0 // Check temperature rules
                            && (device_states[i].end_time == -1 || (device_states[i].end_time > 0 && device_states[i].end_time < now))) {
                    if (trace_on == 1) {
                        Serial1.print(F(" -[check_sensor_rules temp]->"));
                    }
                    set_device_state((enum_device)i, off, 0);
                } else if (device_states[i].on_off == on && p == 1 // checking humidity rules
                            && (device_states[i].end_time == -2 || (device_states[i].end_time > 0 && device_states[i].end_time < now))) {
                    if (trace_on == 1) {
                        Serial1.print(F(" -[check_sensor_rules hum]->"));
                    }
                    set_device_state((enum_device)i, off, 0);
                }
            }
        }
        int8_t st  = (p == 0 ? 0 : 2);  // data[0] and data[1] are for temperature checks
        int8_t end = (p == 0 ? 2 : 4); // data[2] and data[3] are for humidity checks
        for (int pd = st; pd < end; pd++) {
            rule_data_t data = programs[prognr].program_data[pd];
            // if (trace_on == 1) {
            //     Serial1.print(F("data value="));
            //     Serial1.print(data.value);
            //     Serial1.print(" p =");
            //     Serial1.print(p);
            //     Serial1.print(" pd=");
            //     Serial1.println(pd);
            // }
            // terrarium sensor value (=0 means no value read, so skip) above a defined value (data_value < 0)
            // and room sensor value below defined value, 
            // switch defined devices on for a defined period (p >= 2 => humidity, p < 2 => temperature)
            if (data.value > 0 && t_sensor_value > 0 && t_sensor_value > data.value 
                    && ((room_sensor == 1 && r_sensor_value <= data.value) || room_sensor == 0)
                ) {
                for (int a = 0; a < 2; a++) {
                    action_t action = data.actions[a];
                    if (action.on_period < 0) { // keep on until ideal value is reached
                        if (device_states[action.device].on_off == off) {
                            if (trace_on == 1) {
                                Serial1.print(F("- [check_sensor_rules 2]->"));
                            }
                            set_device_state(action.device, on, action.on_period);
                        }
                    } else { // set it on for a given period of time
                        if (device_states[action.device].on_off == off) {
                            if (trace_on == 1) {
                                Serial1.print(F("- [check_sensor_rules 3]->"));
                            }
                            set_device_state(action.device, on, rtc.now().unixtime() + action.on_period);
                        }
                    }
                }
            // terrarium sensor value (=0 means no value read, so skip) below a defined value (data_value < 0)
            // and room sensor value above defined value
            // switch defined devices on for a defined period (p >= 2 => humidity, p < 2 => temperature)
            } else if (data.value < 0 && t_sensor_value > 0 && t_sensor_value < -data.value && 
                        ((room_sensor == 1 && (p >= 2 && r_sensor_value >= -data.value) || p < 2 ) || room_sensor == 0)) {
                for (int a = 0; a < 2; a++) {
                    action_t action = data.actions[a];
                    if (action.on_period < 0) { // keep on until ideal value is reached
                        if (device_states[action.device].on_off == off) {
                            if (trace_on == 1) {
                                Serial1.print(F("- [check_sensor_rules 4]->"));
                            }
                            set_device_state(action.device, on, action.on_period);
                        }
                    } else { // set it on for a given period of time
                        if (device_states[action.device].on_off == off) {
                            if (trace_on == 1) {
                                Serial1.print(F("- [check_sensor_rules 5]->"));
                            }
                            set_device_state(action.device, on, rtc.now().unixtime() + action.on_period);
                        }
                    }
                }
            }
        }
    }
    return rc;
}

int8_t get_rule_set_as_json(rule_set_t program, char **json) {
    int8_t rc = 0;
    char *tmp = (char *)malloc(sizeof(char) * 100);
    if (tmp != NULL) {
        sprintf(tmp, F("{ \"program\": { \"active\":\"%s\","), program.active == on ? "yes" : "no");
        strcpy(*json, tmp);
        sprintf(tmp, F("\"time_of_day\":\"%02d:%02d\","), program.time_of_day / 60, program.time_of_day - (program.time_of_day / 60) * 60);
        strcat(*json, tmp);
        sprintf(tmp, F("\"temp_ideal\":%d,"), program.temp_ideal);
        strcat(*json, tmp);
        sprintf(tmp, F("\"hum_ideal\":%d, \"program_data\": ["), program.hum_ideal);
        strcat(*json, tmp);
        for (int i = 0; i < 4; i++) {
            sprintf(tmp, F("{\"value\":%d, \"actions\": ["), program.program_data[i].value);
            strcat(*json, tmp);
            sprintf(tmp, F("{\"device\":\"%s\", \"on_period\":%d},"), 
                cvt_enum_device_to_string(program.program_data[i].actions[0].device), 
                program.program_data[i].actions[0].on_period);
            strcat(*json, tmp);
            sprintf(tmp, F("{\"device\":\"%s\", \"on_period\":%d}]}"), 
                cvt_enum_device_to_string(program.program_data[i].actions[1].device), 
                program.program_data[i].actions[1].on_period);
            strcat(*json, tmp);
            if (i != 3) {
                strcat(*json, ",");
            } else {
                strcat(*json, "]");
            }
        }
        strcat(*json, "}}");
    } else {
        strcpy(*json, F("{\"error_msg\":\"get_rule_set_as_json() : out of heap space\"}"));
    }
    free(tmp);
    return rc;
}

int8_t get_rule_sets_as_json(char **json) {
    int8_t rc = 0;
    char *tmp = (char *)malloc(sizeof(char) * 600);
    if (tmp != NULL) {
        strcpy(*json, F("["));
        for (int i = 0; i < NR_OF_PROGRAMS; i++) {
            get_rule_set_as_json(programs[i], &tmp);
            strcat(*json, tmp);
            if (i != NR_OF_PROGRAMS - 1) {
                strcat(*json, F(","));
            }
        }
        strcat(*json, "]");
    } else {
        strcpy(*json, F("{\"error_msg\":\"get_rule_sets_as_json() : out of heap space\"}"));
    }
    free(tmp);
    return rc;
}
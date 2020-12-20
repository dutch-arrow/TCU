#include <Arduino.h>
#include "terrarium.h"
#include "timer.h"
#include "sensors.h"
#include "eeprom.h"

int8_t trace_on = 1; // 0 = no tracing, 1 = tracing activated

// EEPROM has 256 bytes. 
// Each program is 33 bytes. 2 programs = 66 bytes (256 - 66 = 190 /4 = 46)
// Max 46 timers.

void setup() {
    Serial1.begin(115200);
    /*
     * Timers
     */
    timers[0] = set_timer(device_light1, 1, on, 0, 0);
    timers[1] = set_timer(device_light1, 1, off, 0, 0);
    timers[2] = set_timer(device_light1, 2, on, 0, 0);
    timers[3] = set_timer(device_light1, 2, off, 0, 0);
    timers[4] = set_timer(device_light2, 1, on, 0, 0);
    timers[5] = set_timer(device_light2, 1, off, 0, 0);
    timers[6] = set_timer(device_light2, 2, on, 0, 0);
    timers[7] = set_timer(device_light2, 2, off, 0, 0);
    timers[8] = set_timer(device_light3, 1, on, 0, 0);
    timers[9] = set_timer(device_light3, 1, off, 0, 0);
    timers[10] = set_timer(device_light3, 2, on, 0, 0);
    timers[11] = set_timer(device_light3, 2, off, 0, 0);
    timers[12] = set_timer(device_light4, 1, on, 0, 0);
    timers[13] = set_timer(device_light4, 1, off, 0, 0);
    timers[14] = set_timer(device_light4, 2, on, 0, 0);
    timers[15] = set_timer(device_light4, 2, off, 0, 0);
    timers[16] = set_timer(device_light5, 1, on, 0, 0);
    timers[17] = set_timer(device_light5, 1, off, 0, 0);
    timers[18] = set_timer(device_light6, 1, on, 0, 0);
    timers[19] = set_timer(device_light6, 1, off, 0, 0);
    timers[20] = set_timer(device_pump, 1, on, 0, 0);
    timers[21] = set_timer(device_pump, 1, off, 0, 0);
    timers[22] = set_timer(device_pump, 2, on, 0, 0);
    timers[23] = set_timer(device_pump, 2, off, 0, 0);
    timers[24] = set_timer(device_sprayer, 1, on, 0, 0);
    timers[25] = set_timer(device_sprayer, 1, off, 0, 0);
    timers[26] = set_timer(device_sprayer, 2, on, 0, 0);
    timers[27] = set_timer(device_sprayer, 2, off, 0, 0);
    timers[28] = set_timer(device_mist, 1, on, 0, 0);
    timers[29] = set_timer(device_mist, 1, off, 0, 0);
    timers[30] = set_timer(device_mist, 2, on, 0, 0);
    timers[31] = set_timer(device_mist, 2, off, 0, 0);
    timers[32] = set_timer(device_fan_in, 1, on, 0, 0);
    timers[33] = set_timer(device_fan_in, 1, off, 0, 0);
    timers[34] = set_timer(device_fan_in, 2, on, 0, 0);
    timers[35] = set_timer(device_fan_in, 2, off, 0, 0);
    timers[36] = set_timer(device_fan_in, 3, on, 0, 0);
    timers[37] = set_timer(device_fan_in, 3, off, 0, 0);
    timers[38] = set_timer(device_fan_out, 1, on, 0, 0);
    timers[39] = set_timer(device_fan_out, 1, off, 0, 0);
    timers[40] = set_timer(device_fan_out, 2, on, 0, 0);
    timers[41] = set_timer(device_fan_out, 2, off, 0, 0);
    timers[42] = set_timer(device_fan_out, 3, on, 0, 0);
    timers[43] = set_timer(device_fan_out, 3, off, 0, 0);
    timers[44] = set_timer(device_fan_out, 4, on, 0, 0);
    timers[45] = set_timer(device_fan_out, 4, off, 0, 0);

    char *json = (char *)malloc(sizeof(char) * 1000);
    get_timers_as_json(device_sprayer, &json);
    Serial1.println("Before");
    Serial1.println(json);
    // Indicate that the timer and rule data settings are written
    EEPROM.write(0, 0xAA);
    // Write the timers to EEPROM
    int bix = START_INDEX_TIMERS;
    for (int i = 0; i < NR_OF_TIMERS; i++) {
        uint32_t tbits;
        cvt_timer_to_uint32(timers[i], &tbits);
        write_timer_int(tbits, bix);
        bix += 4;
    }

    // Check by reading back
    int8_t rc = read_timers();
    if (rc == 0) {
        get_timers_as_json(device_sprayer, &json);
        Serial1.println("After");
        Serial1.println(json);
    } else {
        Serial1.println("Could not read EEPROM");
    }
    // Fan start period and the running period
    bix = START_INDEX_REST;
    EEPROM.write(bix, 0);
    EEPROM.write(bix + 1, 0);
    bix += 2;
    EEPROM.write(bix, 0);
    EEPROM.write(bix + 1, 0);

    /* PROGRAMS */
    // Day program
    programs[0].active = 1; // inactive
    programs[0].time_of_day = 5 * 60 + 15;
    programs[0].temp_ideal = 25;
    programs[0].hum_ideal = 60;
    // Temperature
    programs[0].program_data[0].value = -25; // negative means below, positve means above
    programs[0].program_data[0].actions[0].device = device_fan_in;
    programs[0].program_data[0].actions[0].on_period = -1; // minutes, or -1 = until ideal value has been reached
    programs[0].program_data[0].actions[1].device = no_device;
    programs[0].program_data[0].actions[1].on_period = 0; // not defined
    programs[0].program_data[1].value = 28; // negative means below, positve means above
    programs[0].program_data[1].actions[0].device = device_fan_out;
    programs[0].program_data[1].actions[0].on_period = 15 * 60; // minutes
    programs[0].program_data[1].actions[1].device = device_sprayer;
    programs[0].program_data[1].actions[1].on_period = 15; // seconds
    // Humidity
    programs[0].program_data[2].value = -50; // negative means below, positve means above
    programs[0].program_data[2].actions[0].device = device_mist;
    programs[0].program_data[2].actions[0].on_period = 15; // minutes
    programs[0].program_data[2].actions[1].device = no_device;
    programs[0].program_data[2].actions[1].on_period = 0;    // minutes
    programs[0].program_data[3].value = 70; // negative means below, positve means above
    programs[0].program_data[3].actions[0].device = device_fan_out;
    programs[0].program_data[3].actions[0].on_period = 15; // minutes
    programs[0].program_data[3].actions[1].device = no_device;
    programs[0].program_data[3].actions[1].on_period = 0;    // minutes
    // Night program
    programs[1].active = 1; // inactive
    programs[1].time_of_day = 5 * 60 + 15;
    programs[1].temp_ideal = 25;
    programs[1].hum_ideal = 60;
    // Temperature
    programs[1].program_data[0].value = -19; // negative means below, positve means above
    programs[1].program_data[0].actions[0].device = device_fan_in;
    programs[1].program_data[0].actions[0].on_period = -1; // minutes, or -1 = until ideal value has been reached
    programs[1].program_data[0].actions[1].device = no_device;
    programs[1].program_data[0].actions[1].on_period = 0; // not defined
    programs[1].program_data[1].value = 0; // negative means below, positve means above
    programs[1].program_data[1].actions[0].device = no_device;
    programs[1].program_data[1].actions[0].on_period = 0; // minutes
    programs[1].program_data[1].actions[1].device = no_device;
    programs[1].program_data[1].actions[1].on_period = 0; // seconds
    // Humidity
    programs[1].program_data[2].value = -50; // negative means below, positve means above
    programs[1].program_data[2].actions[0].device = device_mist;
    programs[1].program_data[2].actions[0].on_period = 15; // minutes
    programs[1].program_data[2].actions[1].device = no_device;
    programs[1].program_data[2].actions[1].on_period = 0;    // minutes
    programs[1].program_data[3].value = 70; // negative means below, positve means above
    programs[1].program_data[3].actions[0].device = device_fan_out;
    programs[1].program_data[3].actions[0].on_period = 15; // minutes
    programs[1].program_data[3].actions[1].device = no_device;
    programs[1].program_data[3].actions[1].on_period = 0;    // minutes

    get_rule_set_as_json(programs[1], &json);
    Serial1.println("Before");
    Serial1.println(json);

    write_rule_set_int(programs[0], START_INDEX_PROGRAMS);
    write_rule_set_int(programs[1], START_INDEX_PROGRAMS + 33);

    // Check by reading back
    rc = read_programs();
    if (rc == 0) {
        get_rule_set_as_json(programs[1], &json);
        Serial1.println("After");
        Serial1.println(json);
    } else {
        Serial1.println("Could not read EEPROM");
    }
}

void loop() {
        // put your main code here, to run repeatedly:
}
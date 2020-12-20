#define __STDC_HOSTED__ 0

#include <stdio.h>
#include "sensors.h"
#include "json/ArduinoJson.h"

rule_set_t programs[NR_OF_PROGRAMS];

void setup_rule() {
    const size_t capacity = 4 * JSON_ARRAY_SIZE(2) + JSON_ARRAY_SIZE(4) +
        JSON_OBJECT_SIZE(1) + 12 * JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(5) + 190;
    DynamicJsonDocument doc(capacity);
    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
        // create the error response
        sprintf(jsonString, "{\"error_msg\":\"Could not deserialize the JSON: %s\"}", error.c_str());
        Serial1.print(F("deserializeJson() failed: "));
        Serial1.println(error.c_str());
    } else {
        JsonObject program = doc["program"];
        programs[ix].active = cvt_on_off_str2enum(program["active"]);
        programs[ix].time_of_day = 
            atoi(strtok(program["time_of_day"], ":")) * 60 +
            atoi(strtok(NULL, ":"));
        programs[ix].temp_ideal = program["temp_ideal"]; // 25
        programs[ix].hum_ideal = program["hum_ideal"]; // 60
        JsonArray program_data = program["program_data"];
        for (int i = 0; i < program_data.size(); i++) {
            JsonObject data = program_data[i];
            programs[ix].program_data[i].value = data["value"];
            JsonArray actions = data["actions"];
            for (int j = 0; j < actions.size(); j++) {
                JsonObject action = actions[j];
                programs[ix].program_data[i].actions[j].device = cvt_device_str2enum(action["device"]);
                programs[ix].program_data[i].actions[j].on_period = action["on_period"];
            }
        }
    }

}

int main()
{
    setup_rule();

    int8_t h_terrarium = 80;
    int8_t t_terrarium = 26;
    int8_t h_room = 70;
    int8_t t_room = 21;
    check_sensor_rules(h_terrarium, t_terrarium, h_room, t_room, );
    return 0;
}
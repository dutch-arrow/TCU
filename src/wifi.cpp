#include "wifi.h"
#include "sensors.h"
#include "timer.h"

//char ssid[] = "Familiepijl";    // my network SSID (name)
//char pass[] = "Arrow6666!";     // my network password
char ssid[] = "ASUS-fampijl";    // my network SSID (name)
char pass[] = "arrowfamily2014";     // my network password
int keyIndex = 0;                // my network key Index number
int status = WL_IDLE_STATUS;

char cvt_month[13][4] = {
    F(""),
    F("Jan"),
    F("Feb"),
    F("Mar"),
    F("Apr"),
    F("May"),
    F("Jun"),
    F("Jul"),
    F("Aug"),
    F("Sep"),
    F("Oct"),
    F("Nov"),
    F("Dec")
};

extern int8_t trace_on;
extern int8_t in_test;
extern int8_t test_room_humidity;
extern int8_t test_room_temperature;
extern int8_t test_terrarium_humidity;
extern int8_t test_terrarium_temperature;

extern int8_t h_terrarium;
extern int8_t t_terrarium;
extern int8_t room_sensor; //0=no room sensor available
extern int8_t terrarium_sensor; //0=no terrarium sensor available
extern int8_t prognr;
extern RTC_Millis rtc;
extern int8_t last_minute;
extern int16_t fan_period;
extern int16_t fan_start_period;

/*
 * Connect to the local Wifi network.
 * return value: 0 = connection is made
 *               1 = Wifi module is broken
 *               2 = cannot connect to network after 10 tries
 */
int8_t wifi_connect() {
    if (trace_on == 1) {
        Serial1.println(F("Entering wifi_connect()..."));
    }
    if (WiFi.status() == WL_NO_MODULE) {
        return 1;
    }
    // attempt to connect to Wifi network:
    int8_t attempt = 0;
    while (status != WL_CONNECTED && attempt < 10) {
        if (trace_on == 1) {
            Serial1.print(F("Attempt "));
            Serial1.print(attempt);
            Serial1.println(F(" ..."));
        }
        attempt++;
        WiFi.disconnect();
        delay(1000);
        status = WiFi.begin(ssid, keyIndex, pass);
        if (trace_on == 1) {
            Serial1.print(F("Wifi connection error: "));
            Serial1.println(status);
        }
        // wait 10 seconds for connection to be made
        delay(10000);
    }
    if (status != WL_CONNECTED && attempt == 10) {
        return 2;
    }
    if (status == WL_CONNECTED) {
        return 0;
    }
}

int8_t wifi_check() {
    if (WiFi.status() == WL_CONNECTED) {
        return 0;
    } else {
        return 1;
    }
}

/*
 * Set the date and time by extracting it from the internet.
 * Return value:
 * - 0 : Ok, datetime is set
 * - 1 : Could not set the current datetime
 */
int8_t setRTC(WiFiClient *client) {
    int8_t rc = 0;
    if (wifi_check() != 0) {
        if (trace_on == 1) {
            Serial1.println(F("Wifi not connected, so try to connect again (5 times)"));
        }
        // Wifi is not connected so try to connect to wifi again (5 times)
        int retry_count = 0;
        int8_t rc = wifi_connect();
        while (rc != 0 && retry_count < 5) {
            retry_count++;
            delay(1000);
        }
    }
    if (wifi_check() == 0) {
        // Get the current date time from internet and set the RTC clock
        if (client->connect(F("worldtimeapi.org"), 80)) {
            client->print(F("GET /api/timezone/Europe/Amsterdam HTTP/1.1\r\nHost: worldtimeapi.org\r\nConnection: close\r\n\r\n"));
            while(client->connected() || client->available()) {
                if (client->available()) {
                    client->find("\r\n\r\n");
                    const size_t capacity = JSON_OBJECT_SIZE(15) + 360;
                    DynamicJsonDocument doc(capacity);
                    DeserializationError error = deserializeJson(doc, *client);
                    if (error) {
                        if (trace_on == 1) {
                            Serial1.print(F("setRTC() -> deserializeJson() failed: "));
                            Serial1.println(error.c_str());
                        }
                        // create the error response
                        return 1;
                    }
                    String unixtime = doc[F("unixtime")];
                    String offset = doc[F("utc_offset")];
                    long tm = unixtime.toInt() + (offset.substring(2, 3).toInt() * 3600);
                    rtc.adjust(DateTime(tm));
                    if (trace_on == 1) {
                        Serial1.println(F("Date and time is synced."));
                    }
                    rc = 0;
                }
            }
            if (client->connected()) {
                client->stop();
            }
        } else {
            rc = 1;
        }
    } else {
        rc = 1;
    }
    return rc;
}

void sendResponse(WiFiClient *client, char *jsonString) {
    client->println(F("HTTP/1.1 200 OK"));
    client->println(F("Content-Type: application/json"));
    client->println(F("Connection: close"));
    client->print(F("Content-Length: "));
    client->println(strlen(jsonString));
    client->println();
    client->println(jsonString);
}
/*
 * Handle an incoming request from the given client..
 * Return value:
 * - 0 : Ok
 * - 1 : Could not handle the request
 */
int8_t handleRequest(WiFiClient *client) {
    if (!client->connected() || !client->available()) {
        if (trace_on == 1) {
            Serial1.println(F("Client not connected or available."));
        }
        return 0;
    }
    if (trace_on == 1) {
        Serial1.print(F("Start of handleRequest: freeMemory: "));
        Serial1.println(freeMemory());
    }
    int8_t rc = 1;
    char *jsonString = (char *)malloc(sizeof(char) * 1300);
    if (jsonString == NULL) {
        char tmp[50];
        strcpy(tmp, F("{\"error_msg\":\" out of heap space\"}"));
        sendResponse(client, tmp);
        return 0;
    }
    if (trace_on == 1) {
        Serial1.println(F("Incoming request"));
    }
    // an http request ends with a blank line
    // read the reuest line
    char req[50] = {0};
    client->readBytesUntil('\r', req, sizeof(req));
    char object[30];
    int8_t method = 1;
    // PUT /pump HTTP/1.1
    if (strncmp(req, F("GET /"), 5) == 0) {
        *strstr(req, F(" HTTP")) = 0;
        strncpy(object, &req[5], 30);
        method = 1;
    } else if (strncmp(req, F("PUT /"), 5) == 0) {
        *strstr(req, F(" HTTP")) = 0;
        strncpy(object, &req[5], 30);
        method = 2;
    } else if (strncmp(req, "POST /", 6) == 0) {
        *strstr(req, F(" HTTP")) = 0;
        strncpy(object, &req[6], 30);
        method = 3;
    } else if (strncmp(req, "DELETE /", 8) == 0) {
        *strstr(req, F(" HTTP")) = 0;
        strncpy(object, &req[8], 30);
        method = 4;
    }
    if (trace_on == 1) {
        Serial1.print(F("Method: "));
        Serial1.print(method == 1?"GET":(method == 2?"PUT":(method == 3?"POST":"DELETE")));
        Serial1.print(F(", Object: "));
        Serial1.println(object);
    }
/*==================================================================
*                    G E T
*==================================================================*/
    if (method == 1) { // GET
/*==================================================================
*                    properties
*==================================================================*/
        if (strcmp(object, F("properties")) == 0) {
            get_properties(&jsonString);
/*==================================================================
*                    sensors
*==================================================================*/
        } else if (strcmp(object, F("sensors")) == 0) {
            int8_t rc = read_sensors_as_json(&jsonString);
            if (rc != 0) {
                sprintf(jsonString, "{\"error_code\":%d}", rc);
            }
/*==================================================================
*                    state
*==================================================================*/
        } else if (strcmp(object, F("state")) == 0) {
            get_device_state(&jsonString);
/*==================================================================
*                    rule
*==================================================================*/
        } else if (strncmp(object, F("rule"), 4) == 0) {
            if (strlen(object) > 5) {
                int ix = atoi(&object[5]); // /n = get rule n
                get_rule_set_as_json(programs[ix], &jsonString);
            }
/*==================================================================
*                    timers
*==================================================================*/
        } else if (strncmp(object, F("timers"), 6) == 0) {
            if (strlen(object) > 7) {
                char dev[20];
                strcpy(dev, &object[7]);
                get_timers_as_json(cvt_device_str2enum(dev), &jsonString);
            }
/*==================================================================
*                    fanperiod
*==================================================================*/
        } else if (strcmp(object, F("fanperiod")) == 0) {
            int16_t period;
            int16_t start_period;
            read_fan_period(&start_period, &period);
            sprintf(jsonString, F("{\"start_fan_period\": %d,\"fan_period\": %d}"), start_period, period);
        }
        // create the response
        if (client->connected()) {
            sendResponse(client, jsonString);
            if (trace_on == 1) {
                Serial1.print(F("Sent "));
                Serial1.print(strlen(jsonString));
                Serial1.print(F(" bytes: "));
                Serial1.println(jsonString);
            }
        } else {
            if (trace_on == 1) {
                Serial1.println(F("Client not connected."));
            }
        }
/*==================================================================
*                    P U T
*==================================================================*/
    } else if (method == 2) { // PUT
        // skip till start of json
        client->find("\r\n\r\n");
        int sz = client->readBytes(jsonString, 1200);
        jsonString[sz] = '\0';
        // if (trace_on == 1) {
        //     Serial1.println(jsonString);
        // }
/*==================================================================
*      rules/(on | off | temp/on | temp/off | hum/on | hum/off)
*==================================================================*/
        if (strncmp(object, F("rules"), 5) == 0) {
            char *token = strtok(object, "/"); // 'rules'
            token = strtok(NULL, "/"); // on/off/temp/hum
            if (strcmp(token, F("on")) == 0) {
                setup_rules();
            } else if (strcmp(token, F("off")) == 0) {
                prognr = -1;
            } else if (strcmp(token, F("temp")) == 0) {
                token = strtok(NULL, "/"); // on/off
                if (strcmp(token, F("on")) == 0) {
                    t_terrarium = 0;
                } else {
                    t_terrarium = -1;
                }
            } else if (strcmp(token, F("hum")) == 0) {
                token = strtok(NULL, "/"); // on/off
                if (strcmp(token, F("on")) == 0) {
                    h_terrarium = 0;
                } else {
                    h_terrarium = -1;
                }
            }
            jsonString[0] = '\0';
/*==================================================================
*                    rule
--------------------------------------------------------------------
{ 
    "program": { 
        "active":"on","time_of_day":"10:30","temp_ideal":26,"hum_ideal":60, 
        "program_data": [
            {"value":-25, "actions": [{"device":"fan_in",    "on_period":  -1},{"device":"no device", "on_period": 0}]},
            {"value": 28, "actions": [{"device":"fan_out",   "on_period":1800},{"device":"sprayer",   "on_period":15}]},
            {"value":-50, "actions": [{"device":"mist",      "on_period":  60},{"device":"no device", "on_period": 0}]},
            {"value":  0, "actions": [{"device":"no device", "on_period":   0},{"device":"no device", "on_period": 0}]}
        ]
    }
}
*==================================================================*/
        } else if (strncmp(object, F("rule"), 4) == 0) {
            int ix = atoi(&object[5]); // /n = update rule n
            if (trace_on == 1) {
                Serial1.print(F("Update rule "));
                Serial1.println(ix);
            }
            const size_t capacity = 4 * JSON_ARRAY_SIZE(2) + JSON_ARRAY_SIZE(4) +
                JSON_OBJECT_SIZE(1) + 12 * JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(5) + 190;
            DynamicJsonDocument doc(capacity);
            DeserializationError error = deserializeJson(doc, jsonString);
            if (error) {
                // create the error response
                sprintf(jsonString, F("{\"error_msg\":\"Could not deserialize the JSON: %s\"}"), error.c_str());
                if (trace_on == 1) {
                    Serial1.print(F("deserializeJson() failed: "));
                    Serial1.println(error.c_str());
                }
            } else {
                JsonObject program = doc[F("program")];
                programs[ix].active = cvt_on_off_str2enum(program["active"]);
                programs[ix].time_of_day = 
                    atoi(strtok(program[F("time_of_day")], ":")) * 60 +
                    atoi(strtok(NULL, F(":")));
                programs[ix].temp_ideal = program[F("temp_ideal")]; // 25
                programs[ix].hum_ideal = program[F("hum_ideal")]; // 60
                JsonArray program_data = program[F("program_data")];
                for (int i = 0; i < program_data.size(); i++) {
                    JsonObject data = program_data[i];
                    programs[ix].program_data[i].value = data[F("value")];
                    JsonArray actions = data[F("actions")];
                    for (int j = 0; j < actions.size(); j++) {
                        JsonObject action = actions[j];
                        programs[ix].program_data[i].actions[j].device = cvt_device_str2enum(action[F("device")]);
                        programs[ix].program_data[i].actions[j].on_period = action[F("on_period")];
                    }
                }
            }
            update_program(ix);
            jsonString[0] = '\0';
/*==================================================================
*                    timers
--------------------------------------------------------------------
[
    {"device": "light1", "index": 1,"on_off": "on", "hour":  9,"minute": 30,"repeat": 0},
    {"device": "light1", "index": 1,"on_off": "off","hour": 21,"minute": 30,"repeat": 0}
]
*==================================================================*/
        } else if (strncmp(object, F("timers"), 6) == 0) {
            char *token = strtok(object, "/"); // 'timers'
            char *device = strtok(NULL, "/"); // device name
            if (trace_on == 1) {
                Serial1.print(F("Update timers for device: "));
                Serial1.println(device);
            }
            // [{"device": "sprayer","index": 1,"on_off": "on","hour": 21,"minute": 30,"repeat": 0}]
            const size_t capacity = JSON_ARRAY_SIZE(6) + 6*JSON_OBJECT_SIZE(6) + 120;
            DynamicJsonDocument doc(capacity);
            DeserializationError error = deserializeJson(doc, jsonString);
            if (error) {
                // create the error response
                sprintf(jsonString, F("{\"error_msg\":\"Could not deserialize the JSON: %s\"}"), error.c_str());
                if (trace_on == 1) {
                    Serial1.print(F("deserializeJson() failed: "));
                    Serial1.println(error.c_str());
                }
            } else {
                JsonArray arr = doc.as<JsonArray>();
                for (int i = 0; i < arr.size(); i++) {
                    JsonObject elmnt = arr[i];
                    reset_timer(device, elmnt[F("index")], elmnt[F("on_off")],elmnt[F("hour")], elmnt[F("minute")], elmnt[F("repeat")], elmnt[F("period")]);
                }
                check_timers();
                jsonString[0] = '\0';
            }
/*==================================================================
*                    device/name/(on[/period in seconds] | off)
*==================================================================*/
        } else if (strncmp(object, F("device"), 6) == 0) {
            char *token = strtok(object, "/"); // 'device'
            token = strtok(NULL, "/"); // device name
            enum_device device = cvt_device_str2enum(token);
            token = strtok(NULL, "/"); // 'on' or 'off'
            enum_on_off on_off = cvt_on_off_str2enum(token);
            token = strtok(NULL, "/"); // on period
            if (token != NULL) {
                int32_t end_time = rtc.now().unixtime() + atoi(token);
                set_device_state(device, on_off, end_time);
            } else {
                set_device_state(device, on_off, 0);
            }
            jsonString[0] = '\0';
/*==================================================================
*                    fanperiod/start-after/period in minutes
*==================================================================*/
        } else if (strncmp(object, F("fanperiod"), 9) == 0) {
            char *token = strtok(object, "/"); // 'fanperiod'
            token = strtok(NULL, "/"); // start periodin minutes
            fan_start_period = atoi(token);
            token = strtok(NULL, "/"); // period in minutes
            fan_period = atoi(token);
            reset_fan_period(fan_start_period, fan_period);
            jsonString[0] = '\0';
/*==================================================================
*                    trace/on
*==================================================================*/
        } else if (strcmp(object, F("trace/on")) == 0) {
            trace_on = 1;
            jsonString[0] = '\0';
/*==================================================================
*                    trace/off
*==================================================================*/
        } else if (strcmp(object, F("trace/off")) == 0) {
            trace_on = 0;
            jsonString[0] = '\0';
        }
        // create the success response
        if (client->connected()) {
            sendResponse(client, jsonString);
        } else {
            if (trace_on == 1) {
                Serial1.println(F("Client not connected."));
            }
        }
/*==================================================================
*                    P O S T
*==================================================================*/
    } else if (method == 3) { // POST
        // skip till start of json
        client->find("\r\n\r\n");
        int sz = client->readBytes(jsonString, 1200);
        jsonString[sz] = '\0';
        // if (trace_on == 1) {
        //     Serial1.println(jsonString);
        // }
/*==================================================================
*                    starttest
--------------------------------------------------------------------
[
  { "location": "room", "humidity": 70, "temperature": 20 },
  { "location": "terrarium", "humidity": 70, "temperature": 26 }
]
*==================================================================*/
        if (strcmp(object, F("starttest")) == 0) {
            const size_t capacity = JSON_ARRAY_SIZE(2) + 2*JSON_OBJECT_SIZE(3) + 50;
            DynamicJsonDocument doc(capacity);
            DeserializationError error = deserializeJson(doc, jsonString);
            if (error) {
                // create the error response
                sprintf(jsonString, F("{\"error_msg\":\"Could not deserialize the JSON: %s\"}"), error.c_str());
                if (trace_on == 1) {
                    Serial1.print(F("deserializeJson() failed: "));
                    Serial1.println(error.c_str());
                }
            } else {
                in_test = 1;
                terrarium_sensor = 1;
                room_sensor = 1;
                JsonArray arr = doc.as<JsonArray>();
                if (strcmp(arr[0][F("location")], F("room")) == 0) {
                    test_room_temperature = arr[0][F("temperature")];
                    test_room_humidity = arr[0][F("humidity")];
                }
                if (strcmp(arr[1][F("location")], F("terrarium")) == 0) {
                    test_terrarium_temperature = arr[1][F("temperature")];
                    test_terrarium_humidity = arr[1][F("humidity")];
                }
                jsonString[0] = '\0';
            }
/*==================================================================
*                    stoptest
*==================================================================*/
        } else if (strcmp(object, F("stoptest")) == 0) {
            in_test = 0;
/*==================================================================
*                    roomsensor/on
*==================================================================*/
        } else if (strcmp(object, F("roomsensor/on")) == 0) {
            room_sensor = 1;
/*==================================================================
*                    roomsensor/off
*==================================================================*/
        } else if (strcmp(object, F("roomsensor/off")) == 0) {
            room_sensor = 0;
/*==================================================================
*                    terrariumsensor/on
*==================================================================*/
        } else if (strcmp(object, F("terrariumsensor/on")) == 0) {
            terrarium_sensor = 1;
/*==================================================================
*                    terrariumsensor/off
*==================================================================*/
        } else if (strcmp(object, F("terrariumsensor/off")) == 0) {
            terrarium_sensor = 0;
/*==================================================================
*                    setdate/yyyy-mm-ddThh:mm:ss
*==================================================================*/
        } else if (strncmp(object, F("setdate"), 7) == 0) {
            // 2020-10-03T16:21:00
            rtc.adjust(DateTime(&object[8]));
            last_minute = rtc.now().minute();
            setup_all_timers();
            setup_rules();
        }
        // create the success response
        if (client->connected()) {
            sendResponse(client, jsonString);
        } else {
            if (trace_on == 1) {
                Serial1.println(F("Client not connected."));
            }
        }
    } else if (method == 4) { // DELETE
        if (client->connected()) {
            sendResponse(client, F("DELETE is not implemented."));
        } else {
            if (trace_on == 1) {
                Serial1.println(F("Client not connected."));
            }
        }
    }
    rc = 0;
    free(jsonString);
    return rc;
}

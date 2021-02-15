#include <Arduino.h>
#include <RTClib.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <dht.h>
#include <MemoryFree.h>
#include <pgmStrToRAM.h>

#include "lcd.h"
#include "sensors.h"
#include "terrarium.h"
#include "timer.h"
#include "wifi.h"

void setRTC();

// Define a webserver on http-port 80
WiFiServer server(80);
// Define a webclient
WiFiClient client;

int8_t retry_count = 0;
int8_t last_minute;
int8_t last_day = 0;
int8_t nr_of_display_lines = 2;
int8_t cur_display_line = 0;
int8_t h_terrarium = 0;
int8_t t_terrarium = 0;
int8_t h_room;
int8_t t_room;
int8_t prognr = -1; //-1 = no,  0 = day, 1 = night
int8_t prevPrognr = -2;
int16_t fan_period;
int16_t fan_start_period;
int32_t fan_start_dt;
int32_t drying_period_end_dt;

int8_t trace_on = 0; // 0 = no tracing, 1 = tracing activated
int8_t in_test = 0;
int8_t test_room_humidity;
int8_t test_room_temperature;
int8_t test_terrarium_humidity;
int8_t test_terrarium_temperature;
int8_t room_sensor = 1; //0 = no room sensor available
int8_t rscnt = 0;
int8_t terrarium_sensor = 1; // terrarium sensor available
int8_t tscnt = 0;

void setup() {
    delay(10000); // wait for uploading to finish
    // put your setup code here, to run once:
    Serial1.begin(115200);
    if (trace_on == 1) {
        Serial1.println(F("Setup phase"));
        Serial1.println(F("==========="));
    }

    WiFi.end();   
    WiFi.disconnect();
    delay(5000);

    int8_t rc = wifi_connect();
    if (rc == 0) {
        // once you are connected :
        if (trace_on == 1) {
            Serial1.print(F("You're connected to the network with IP-address: "));
            Serial1.println(WiFi.localIP());
        }
        // start the server
        server.begin();
        // set the current datetime
        rc = setRTC(&client);
        if (rc == 1) {
            rtc.adjust(DateTime(__DATE__, __TIME__));
            if (trace_on == 1) {
                Serial1.println(F("Date and time sync error."));
            }
        }
        last_minute = rtc.now().minute();
    } else {
        if (trace_on == 1) {
            Serial1.println(F("Could not connect to Wifi."));
        }
    }
    setup_lcd();
    setup_lights();
    setup_mist();
    setup_sprayer();
    setup_pump();
    setup_fans();
    setup_all_timers();
    setup_rules();
    drying_period_end_dt = 0;
    prognr = -1;
    prevPrognr = -2;
    if (trace_on == 1) {
        Serial1.println();
        Serial1.println(F("Starting LOOP phase"));
        Serial1.println(F("==================="));
        Serial1.print(F("After setup: freeMemory: "));
        Serial1.println(freeMemory());
    }
}

void loop() {
    int8_t rc = 0;
    retry_count = 0;
    if (server.status() != CLOSED) {
        // listen for incoming clients
        client = server.available();
        if (client) {
            rc = handleRequest(&client);
            delay(2000);
            // close the connection:
            client.stop();
            if (trace_on == 1) {
                Serial1.print(F("client disconnected: "));
                Serial1.println(rc);
            }
        }
    } else { // Webserver is closed
        Serial1.println(F("Webserver is closed."));
        // Check if Wifi connection is active
        if (wifi_check() == 0) {
            // If Wifi is OK restart the webserver
            server.begin();
            if (trace_on == 1) {
                Serial1.println(F("Webserver is started again."));
            }
            rc = 0;
        } else {
            if (trace_on == 1) {
                Serial1.println(F("Wifi not connected, so try to connect again (5 times)"));
            }
            // Wifi is not connected so try to connect to wifi again (5 times)
            int8_t rc = wifi_connect();
            while (rc != 0 && retry_count < 5) {
                retry_count++;
                delay(1000);
            }
            if (rc == 0) {
               // start the server
                server.begin();
                if (trace_on == 1) {
                    Serial1.println(F("Webserver is started again."));
                }
            } else {
                rc = 1; // No wifi
            }
        }
    }
    // Read sensors
    rc = read_terrarium_sensor(&h_terrarium, &t_terrarium);
    // If timeout error increase counter
    // if (rc == 2) {
    //     tscnt++;
    // } else {
    //     tscnt = 0;
    // }
    // if (tscnt == 5) { // switch terrarium sensor off when 5 times a timeout is received
    //     terrarium_sensor = 0;
    //     h_terrarium = 0;
    //     t_terrarium = 0;
    // }
    rc = read_room_sensor(&h_room, &t_room);
    // If timeout error increase counter
    if (rc == 2) {
        rscnt++;
    } else {
        rscnt = 0;
    }
    if (tscnt == 5) { // switch room sensor off when 5 times a timeout is received
        room_sensor = 0;
        h_room = 0;
        t_room = 0;
    }
    // Display
    nr_of_display_lines = 3;
    if (room_sensor == 0) {
        nr_of_display_lines--;
    }
    if (rc != 0) {
        nr_of_display_lines++;
    }
    char lcd_line0_text[17];
    char lcd_line1_text[17];
    // display terrarium sensor readings always on line 1 of LCD
    lcd_clear_line(0);
    if (rc != 0) {
        sprintf(lcd_line0_text, F("ERROR %d"), rc);
        lcd.print(lcd_line0_text);
    } else {
        sprintf(lcd_line0_text, F("Terrm: %2d%% %2d"), h_terrarium, t_terrarium);
        lcd.print(lcd_line0_text);
        lcd.write(0);
        lcd.print("C");
    }
    // rotate LCD display for line 2
    if (cur_display_line == 0) {
        // display datetime
        char dt_str[] = F("DD-MMM-YY hh:mm");
        lcd_clear_line(1);
        lcd.print(rtc.now().toString(dt_str));
        if (prognr != -1) {
            if (programs[prognr].active == off) {
                lcd.print("-");
            } else {
                lcd.write((prognr == 0 ? 1 : 2));
            }
        }
    } else if (cur_display_line == 1) {
        // display room sensor readings
        lcd_clear_line(1);
        if (rc != 0) {
            sprintf(lcd_line1_text, F("ERROR %d"), rc);
            lcd.print(lcd_line1_text);
        } else {
            sprintf(lcd_line1_text, F("Kamer: %2d%% %2d"), h_room, t_room);
            lcd.print(lcd_line1_text);
            lcd.write(0);
            lcd.print("C");
        }
    } else if (cur_display_line == 2) {
        // display IP address
        lcd_clear_line(1);
        lcd.print(WiFi.localIP());
    } else if (cur_display_line == 3) {
        // display error
        lcd_clear_line(1);
        lcd.print(F("ERR: No wifi"));
    }
    cur_display_line++;
    cur_display_line = cur_display_line % nr_of_display_lines;
    if (rc == 0) {
        // Since devices on-time are expressed in seconds the state is checked each second
        check_state_rules();
        DateTime now = rtc.now();
        int8_t cur_minute = now.minute();
        // Timers are checked each minute
        if ((cur_minute == 0 && last_minute == 59) || cur_minute > last_minute) {
            // A minute has passed, so check timers
            char dt_str[] = F("DD-MMM-YY hh:mm");
            if (trace_on == 1) {
                Serial1.print(now.toString(dt_str));
                Serial1.print(F(" Perform check (free memory: "));
                Serial1.print(freeMemory());
                Serial1.println(F(")"));
            }
            check_timers(&now);
            check_program();
            check_sensor_rules(h_terrarium, t_terrarium, h_room, t_room);
            // Drying phase check. The 'fan_start_dt' and 'drying_period_end_dt' are set in 'terrarium.cpp'.
            // 'drying_period_end_dt' when sprayer is switched on
            // 'fan_start_dt' when sprayer is switched off
            if (fan_start_dt != 0 && now.unixtime() >= fan_start_dt) {
                set_device_state(device_fan_in, on, drying_period_end_dt);
                set_device_state(device_fan_out, on, drying_period_end_dt);
                fan_start_dt = 0;
            }
            if (drying_period_end_dt != 0 && now.unixtime() >= drying_period_end_dt) {
                // start rule checking again now drying has finished. 
                // It was set to off in 'terrarium.cpp' when sprayer switched on.
                programs[prognr].active = on; 
                drying_period_end_dt = 0;
            }
            last_minute = cur_minute;
        }
        // Clock is synchronized every day
        int8_t cur_day = rtc.now().day();
        if (cur_day != last_day) {
            if (setRTC(&client) == 0) {
                last_day = rtc.now().day();
            }
        }
    }
    // Wait 1 second
    delay(1000);
}

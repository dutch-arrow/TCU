#ifndef sensors_h
#define sensors_h

#ifdef _NATIVE_
#include <stdint.h>
#else
#include <dht.h>
#endif

#include "terrarium.h"

enum enum_ht { temperature, humidity };

typedef struct {            // 3 bytes
    enum_device device;     // 0 - 13                =  4 bits
    int16_t on_period;      // in seconds (5 - 3600) = 12 bits
} action_t;

typedef struct {            // 7 bytes
    int8_t value;           // -19 - +99             =  7 bits
    action_t actions[2];    // 2 x 3 bytes
} rule_data_t;

typedef struct {            // 33 bytes
    int8_t active;          // 0 = active, 1 is inactive       =  1 bit
    int16_t time_of_day;    // in minutes: hour x 60 + minutes = 11 bits
    int8_t temp_ideal;      // 25                              =  5 bits
    int8_t hum_ideal;       // 60                              =  6 bits
                            //                         Totaal: = 23 bits = 3 bytes
    rule_data_t program_data[4]; // 4 x 7 + 3 = 31 bytes 
} rule_set_t;

extern rule_set_t programs[NR_OF_PROGRAMS];

int8_t read_room_sensor(int8_t *h, int8_t *t);
int8_t read_terrarium_sensor(int8_t *h, int8_t *t);
int8_t read_sensors_as_json(char **json);

int8_t read_programs();
int8_t update_program(int8_t index);
void check_program();

void setup_rules();
int8_t check_sensor_rules(int8_t h_terrarium, int8_t t_terrarium, int8_t h_room, int8_t t_room);
int8_t get_rule_set_as_json(rule_set_t program, char **json);
int8_t get_rule_sets_as_json(char **json);

#endif /* sensors_h */

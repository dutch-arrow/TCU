#ifndef terrarium_h
#define terrarium_h

#ifdef _NATIVE_
#include <stdint.h>
#else
#include <RTClib.h>
#endif

// EEPROM has 256 bytes.
// Each timer is compressed into 4 bytes.
// 46 timers = 184 bytes
// Each programs is 33 bytes.
// 2 programs = 66 bytes
// Total: 250 bytes. Free: 6 bytes
#define NR_OF_TIMERS 46
#define NR_OF_PROGRAMS 2

#define pin_serial_tx    0
#define pin_serial_rx    1
#define pin_light1       2
#define pin_light2       3
#define pin_light3       4
#define pin_light4       5
#define pin_light5       6
#define pin_light6       7
#define pin_light7       8
#define pin_light8       9
#define pin_fan_out     10
#define pin_fan_in      11
#define pin_sprayer     12
#define pin_mist        13
#define pin_pump        14
#define pin_sensor_in   17
#define pin_sensor_out  16
#define lcd_sda         18
#define lcd_scl         19

enum enum_device {
    no_device,
    device_light1,
    device_light2,
    device_light3,
    device_light4,
    device_light5,
    device_light6,
    device_pump,
    device_sprayer,
    device_mist,
    device_fan_in,
    device_fan_out
};
#define NR_OF_DEVICES 12 // MODIFY IF enum_device CHANGES !

enum enum_on_off { on, off };

typedef struct {
    enum_on_off on_off;
    // when on, endtime in seconds since 2000-01-01 
    // or -1 = untill temp = ideal temp 
    // or -2 = untill hum = ideal hum
    int32_t end_time; 
} device_state;

extern device_state device_states[NR_OF_DEVICES];
extern RTC_Millis rtc;

void setup_lights();
void setup_fans();
void setup_sprayer();
void setup_mist();
void setup_pump();

void get_properties(char **json);
void set_device_state(enum_device device, enum_on_off on_off, int32_t end_time);
void get_device_state(char **json);
device_state get_device_state(enum_device device);
int8_t check_state_rules();

char *cvt_enum_device_to_string(enum_device device);
char *cvt_enum_on_off_to_string(enum_on_off on_off);
enum_device cvt_device_str2enum(const char *device);
enum_on_off cvt_on_off_str2enum(const char *on_off);

#endif /* terrarium_h */
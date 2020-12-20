#ifndef wifi_h
#define wifi_h


#ifdef _NATIVE_
#include <stdint.h>
#else
#include <ArduinoJson.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <MemoryFree.h>
#include <pgmStrToRAM.h>
#include <RTClib.h>
#endif

extern RTC_Millis rtc;

int8_t wifi_connect();
int8_t wifi_check();
int8_t setRTC(WiFiClient *client);

int8_t handleRequest(WiFiClient *client);

#endif /* wifi_h */
/*
  Configuration.h - Configuration file
  
  Copyright (C) 2020  Davide Perini
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of 
  this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
  copies of the Software, and to permit persons to whom the Software is 
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in 
  all copies or substantial portions of the Software.
  
  You should have received a copy of the MIT License along with this program.  
  If not, see <https://opensource.org/licenses/MIT/>.
*/

#ifndef _DPSOFTWARE_CONFIG_H
#define _DPSOFTWARE_CONFIG_H

#include <ESP8266WiFi.h>
#include <Adafruit_SSD1306.h>

const String AUTHOR = "DPsoftware";

// Serial rate for debug
#define SERIAL_RATE 115200
// Specify if you want to use a display or only Serial
const bool PRINT_TO_DISPLAY = true;
// Display specs
#define OLED_RESET LED_BUILTIN // Pin used for integrated D1 Mini blue LED
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
extern Adafruit_SSD1306 display;

#ifdef TARGET_WATCHWINDER_1
  // SENSORNAME will be used as device network name
  #define WIFI_DEVICE_NAME "WATCHWINDER_1"
  // Port for the OTA firmware uplaod
  const int OTA_PORT = 8280;
  // STATIC IP FOR THE MICROCONTROLLER
  #define IP_MICROCONTROLLER IPAddress(192, 168, 1, 53);  
#endif 
#ifdef TARGET_WATCHWINDER_2
  // SENSORNAME will be used as device network name
  #define WIFI_DEVICE_NAME "WATCHWINDER_2"
  // Port for the OTA firmware uplaod
  const int OTA_PORT = 8281;
  // STATIC IP FOR THE MICROCONTROLLER
  #define IP_MICROCONTROLLER IPAddress(192, 168, 1, 54);  
#endif 
#ifdef TARGET_WATCHWINDER_3
  // SENSORNAME will be used as device network name
  #define WIFI_DEVICE_NAME "WATCHWINDER_3"
  // Port for the OTA firmware uplaod
  const int OTA_PORT = 8282;
  // STATIC IP FOR THE MICROCONTROLLER
  #define IP_MICROCONTROLLER IPAddress(192, 168, 1, 55);  
#endif 
#ifdef TARGET_WATCHWINDER_4
  // SENSORNAME will be used as device network name
  #define WIFI_DEVICE_NAME "WATCHWINDER_4"
  // Port for the OTA firmware uplaod
  const int OTA_PORT = 8283;
  // STATIC IP FOR THE MICROCONTROLLER
  #define IP_MICROCONTROLLER IPAddress(192, 168, 1, 56);  
#endif 
#ifdef TARGET_WATCHWINDER_5
  // SENSORNAME will be used as device network name
  #define WIFI_DEVICE_NAME "WATCHWINDER_5"
  // Port for the OTA firmware uplaod
  const int OTA_PORT = 8284;
  // STATIC IP FOR THE MICROCONTROLLER
  #define IP_MICROCONTROLLER IPAddress(192, 168, 1, 57);  
#endif 
#ifdef TARGET_WATCHWINDER_6
  // SENSORNAME will be used as device network name
  #define WIFI_DEVICE_NAME "WATCHWINDER_6"
  // Port for the OTA firmware uplaod
  const int OTA_PORT = 8285;
  // STATIC IP FOR THE MICROCONTROLLER
  #define IP_MICROCONTROLLER IPAddress(192, 168, 1, 58);  
#endif 

// Set wifi power in dbm range 0/0.25, set to 0 to reduce PIR false positive due to wifi power, 0 low, 20.5 max.
const double WIFI_POWER = 0;  

// GATEWAY IP
#define IP_GATEWAY IPAddress(192, 168, 1, 1);
// MQTT server port
const int mqtt_port = 1883;
// Maximum number of reconnection (WiFi/MQTT) attemp before powering off peripherals
#define MAX_RECONNECT 500
// Maximum JSON Object Size
#define MAX_JSON_OBJECT_SIZE 20

#endif



#ifndef CONFIG_H
#define CONFIG_H

#include <set>
#include <iostream>
#include "./structs/device.h"

const char* SSID = "Odyssey";
const char* PASSWORD = "Blue4524.";

/* UserDevice devices[] = {
   {"Lauters", "10.0.0.132", "bulb"},
   {"Desja", "10.0.0.95", "bulb"},
   {"Fado", "10.0.0.212", "bulb"}
}; */

UserDevice devices[] = {
  {"Lauters", "10.0.0.130", "bulb"},
  {"Desja", "10.0.0.97", "bulb"},
   {"Fado", "10.0.0.213", "bulb"}
};

char* aliases[] = {
  "Lauter",
  "Dejsa",
  "Ball"
};

const int size = 3;

const char* ips[] = {
    "192.168.1.46",
    "192.168.1.47",
    "192.168.1.48",
    "192.168.1.49",
};

//Time between inactivity and sleep
const int inactivity_time = 15000;

const int min_brightness = 0;
const int max_brightness = 100;

#endif
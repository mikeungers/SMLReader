#ifndef CONFIG_H
#define CONFIG_H

#include "Arduino.h"
#include "Sensor.h"

const char *VERSION = "2.1.6";

// Modifying the config version will probably cause a loss of the existig configuration.
// Be careful!
const char *CONFIG_VERSION = "1.0.2";

const char *WIFI_AP_SSID = "SMLReader";
const char *WIFI_AP_DEFAULT_PASSWORD = "";

// Enables deep-sleep between measurements
// Attention: Make sure to connect RST with GPIO16 (D0 on Wemos D1) to allow the ESP to be woken up by timer.
const bool DEEP_SLEEP_ENABLED = false;
// Time between sleep cycles (3600 seconds max)
const int DEEP_SLEEP_DURATION = 1800;

static const SensorConfig SENSOR_CONFIGS[] = {
    {.pin = D2,
     .name = "1",
     .numeric_only = false,
     .status_led_enabled = true,
     .status_led_inverted = true,
     .status_led_pin = LED_BUILTIN,
     .interval = 0}};

const uint8_t NUM_OF_SENSORS = sizeof(SENSOR_CONFIGS) / sizeof(SensorConfig);

#endif
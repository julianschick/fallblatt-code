#ifndef GLOBALS_H_
#define GLOBALS_H_

// Basis
#include <string>
#include <sstream>
#include <stdio.h>

// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

// Logging
#include "esp_log.h"

// Target Device
#define TARGET_DB 0
#define TARGET_SBAHN 1
#define DEPLOYMENT_TARGET TARGET_DB

// Options
#define OPTION_UART_INPUT 0
#define OPTION_EVENT_BITS_DEBUG 0

// Log Tags
#define TAG_FLAP "flap"
#define TAG_WIFI "wlan"
#define TAG_BT "bluetooth"
#define TAG_HTTP "http"
#define TAG_NVS "nvs"

// MDNS Server Properties
#if DEPLOYMENT_TARGET == TARGET_DB
    #define MDNS_HOSTNAME "delay-db"
    #define MDNS_SERVICE_NAME "Delay-DB/REST"
#elif DEPLOYMENT_TARGET == TARGET_SBAHN
    #define MDNS_HOSTNAME "splitflap-berlin"
    #define MDNS_SERVICE_NAME "Splitflap-Berlin/REST"
#endif

// Bluetooth Com Properties
#if DEPLOYMENT_TARGET == TARGET_DB
    #define SPP_SERVER_NAME "Delay-DB/Serial"
    #define BT_DEVICE_NAME "Delay-DB"
#elif
    #define SPP_SERVER_NAME "Splitflap-Berlin/Serial"
    #define BT_DEVICE_NAME "Splitflap-Berlin"
#endif

// Syntactic Sugar
#define PRIVATE_SYMBOLS namespace {
#define PRIVATE_END }

#endif //GLOBALS_H_
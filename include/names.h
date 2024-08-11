#ifndef GLOBALS_H_
#define GLOBALS_H_

// Target Device
#define TARGET_DB 0
#define TARGET_SBAHN 1
#define DEPLOYMENT_TARGET TARGET_SBAHN

// Options
#define OPTION_UART_INPUT 0
#define OPTION_DEBUG_MESSAGES 0

// MDNS Server Properties
#if DEPLOYMENT_TARGET == TARGET_DB
    #define MDNS_HOSTNAME "delay-db"
    #define MDNS_SERVICE_NAME "Delay-DB/REST"
#elif DEPLOYMENT_TARGET == TARGET_SBAHN
    #define MDNS_HOSTNAME "splitflap-berlin"
    #define MDNS_SERVICE_NAME "Splitflap-Berlin/REST"
#endif
#define MDNS_INSTANCE_NAME "Splitflap Controller"
#define MDNS_SERVICE_TYPE "_http"

// Bluetooth Com Properties
#if DEPLOYMENT_TARGET == TARGET_DB
    #define SPP_SERVER_NAME "Delay-DB/Serial"
    #define BT_DEVICE_NAME "Delay-DB"
#elif DEPLOYMENT_TARGET == TARGET_SBAHN
    #define SPP_SERVER_NAME "Splitflap-Berlin/Serial"
    #define BT_DEVICE_NAME "Splitflap-Berlin"
#endif

// DHCP
#if DEPLOYMENT_TARGET == TARGET_DB
    #define DHCP_HOSTNAME "delay-db"
#elif DEPLOYMENT_TARGET == TARGET_SBAHN
    #define DHCP_HOSTNAME "splitflap-berlin"
#endif

// Who am I?
#if DEPLOYMENT_TARGET == TARGET_DB
#define WHOAMI "Splitflap-Controller 'delay-db'\n"
#elif DEPLOYMENT_TARGET == TARGET_SBAHN
#define WHOAMI "Splitflap-Controller 'splitflap-berlin'\n"
#endif

#endif //GLOBALS_H_
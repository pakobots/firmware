#ifndef _WS_SERVER_H_
#define _WS_SERVER_H_

#include "sdkconfig.h"

// #define WIFI_SSID               CONFIG_WIFI_SSID
// #define WIFI_PASS               CONFIG_WIFI_PASSWORD

#define WS_SERVER_TASK_NAME        "ws_server"
#define WS_SERVER_TASK_STACK_WORDS 10240
#define WS_SERVER_TASK_PRORIOTY    8

#define WS_SERVER_RECV_BUF_LEN     512
#define WS_SERVER_PORT             80

void ws_server_start(void);

#endif

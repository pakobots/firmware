#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include "sdkconfig.h"

// #define WIFI_SSID               CONFIG_WIFI_SSID
// #define WIFI_PASS               CONFIG_WIFI_PASSWORD

#define HTTP_SERVER_TASK_NAME        "http_server"
#define HTTP_SERVER_TASK_STACK_WORDS 10240
#define HTTP_SERVER_TASK_PRORIOTY    8

#define HTTP_SERVER_RECV_BUF_LEN     512
#define HTTP_SERVER_PORT             80

void http_server_start(void);

#endif

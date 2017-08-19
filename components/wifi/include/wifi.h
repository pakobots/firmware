#ifndef _WIFI_H_
#define _WIFI_H_

#include "sdkconfig.h"

#define AP_SSID CONFIG_WIFI_AP_SSID
#define AP_PASS CONFIG_WIFI_AP_PASSWORD
#define WIFI_SCAN_FINISHED 0
#define WIFI_SCAN_RUNNING 1
#define WIFI_SCAN_FAILED 2
#define WIFI_SCAN_INIT 4
#define WIFI_SCANNING_TIME 200
#define WIFI_RESCAN_TIMEOUT 2000
#define MAX_AP_FOUND 10

typedef void (*evt_connected_callback)(void);
void wifi_enable(evt_connected_callback);
int wifi_scan();
int wifi_connected();
int wifi_connect(char * ssid, char * password, unsigned int persist);
unsigned char * wifi_scan_result();

#endif

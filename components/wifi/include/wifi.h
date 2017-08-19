/*
 * This file is part of Pako Bots Firmware.
 *
 *  Pako Bots Firmware is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Pako Bots Firmware is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Pako Bots Firmware.  If not, see <http://www.gnu.org/licenses/>.
 */

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

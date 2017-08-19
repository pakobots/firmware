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

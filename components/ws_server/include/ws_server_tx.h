/*
 * This file is part of Pako Bots division of Origami 3 Firmware.
 *
 *  Pako Bots division of Origami 3 Firmware is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Pako Bots division of Origami 3 Firmware is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Pako Bots division of Origami 3 Firmware.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _WS_SERVER_TX_H_
#define _WS_SERVER_TX_H_

#include "sdkconfig.h"
#include <stdint.h>

#define WS_SERVER_TASK_TX_NAME        "ws_server_tx"
#define WS_SERVER_TASK_STACK_WORDS 10240
#define WS_SERVER_TASK_PRORIOTY    8

#define WS_SERVER_RECV_BUF_LEN     512
#define WS_SERVER_PORT             80

void ws_server_tx_start(void);

#endif

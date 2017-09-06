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

#ifndef _WS_PACKET_TASK_H_
#define _WS_PACKET_TASK_H_

#include "sdkconfig.h"
#include "lwip/api.h"

#define WS_MASK_L  0x4  /**< \brief Length of MASK field in WebSocket Header*/

/** \brief Websocket frame header type*/
typedef struct {
								uint8_t opcode:WS_MASK_L;
								uint8_t reserved : 3;
								uint8_t FIN : 1;
								uint8_t payload_length : 7;
								uint8_t mask : 1;
} WS_frame_header_t;

/** \brief Websocket frame type*/
typedef struct {
								struct netconn*  conenction;
								WS_frame_header_t frame_header;
								size_t payload_length;
								char*    payload;
}WebSocket_frame_t;

err_t ws_write_data(char* p_data, size_t length);
void ws_server(void *pvParameters);

#endif

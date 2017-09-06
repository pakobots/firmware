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
 
#include "ws_receive_task.h"
#include "ws_server_task.h"
#include "robot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// WebSocket frame receive queue
extern QueueHandle_t WebSocket_rx_queue;

void
process(char * payload, size_t length){
    robot_cmd(payload);
} /* process */

void
ws_receive(void * pvParameters){
    (void) pvParameters;

    // frame buffer
    WebSocket_frame_t frame;

    // create WebSocket RX Queue
    WebSocket_rx_queue = xQueueCreate(10, sizeof(WebSocket_frame_t));

    while (1) {
        // receive next WebSocket frame from queue
        if (xQueueReceive(WebSocket_rx_queue, &frame, 3 * portTICK_PERIOD_MS) == pdTRUE) {
            if (frame.payload == NULL) {
                free(frame.payload);
                return;
            }

            process(frame.payload, frame.payload_length);
            free(frame.payload);
        }
    }
}

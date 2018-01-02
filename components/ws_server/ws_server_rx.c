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

#include <stdint.h>
#include <string.h>
#include "ws_server_rx.h"
#include "ws_server.h"
#include "esp_system.h"
#include "esp_log.h"
#include "robot.h"

const static char *TAG = "WS_Server";

QueueHandle_t WebSocket_rx_queue;
static ws_server_handler_t handler;

static void
ws_receive_task(void *pvParameters)
{
    WebSocket_frame_t frame;
    WebSocket_rx_queue = xQueueCreate( 10, sizeof(WebSocket_frame_t) );

    while (1)
    {
        if ( xQueueReceive(WebSocket_rx_queue, &frame, 65535 * portTICK_PERIOD_MS) )
        {
            if (frame.payload != NULL)
            {
                robot_cmd(frame.payload, frame.payload_length);
            }

            free(frame.payload);
        }
    }
}

static void ws_server_rx_connected()
{
    robot_connected(NULL);
}


void
ws_server_rx_start()
{
    ESP_LOGI(TAG, "starting websocket RX server.");
    int ret;

    handler.port = 9998;
    handler.rx = (ws_server_rx_func) ws_server_receive_data;
    handler.opened = ws_server_rx_connected;
    handler.conn = NULL;

    ret = xTaskCreate(&ws_receive_task, "ws_process_rx", 2048, NULL, 5, NULL);

    if (ret != 1)
    {
        ESP_LOGI(TAG, "create task ws_process_RX failed %d", ret);
        return;
    }

    ret = xTaskCreate(&ws_server_task, WS_SERVER_TASK_RX_NAME, 2048, &handler, 5, NULL);

    if (ret != 1)
    {
        ESP_LOGI(TAG, "create task RX ws_server failed %d", ret);
        return;
    }

    ESP_LOGI(TAG, "websocket RX server available.");
}

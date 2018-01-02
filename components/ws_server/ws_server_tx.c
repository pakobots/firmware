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
#include "ws_server_tx.h"
#include "ws_server.h"
#include "esp_system.h"
#include "esp_log.h"
#include "robot.h"

const static char *TAG = "WS_Server";

QueueHandle_t WebSocket_tx_queue;
static ws_server_handler_t handler;

static void ws_server_tx_send(char *payload, uint16_t length)
{
    WebSocket_frame_t tx;
    tx.payload_length = length;
    tx.payload = malloc(length + 1);

    memcpy(tx.payload, payload, length);
    if ( !xQueueSend(WebSocket_tx_queue, &tx, 0) )
    {
        free(tx.payload);
    }
}


static void
ws_server_tx_task(void *pvParameters)
{
    WebSocket_frame_t frame;

    while (1)
    {
        if ( xQueueReceive(WebSocket_tx_queue, &frame, 65535 * portTICK_PERIOD_MS) )
        {
            if (frame.payload != NULL)
            {
                ws_server_write_data(handler.conn, frame.payload, frame.payload_length);
            }

            free(frame.payload);
        }
    }
}


static void ws_server_tx_connected()
{
    robot_connected( (tx_func)ws_server_tx_send );
}


static void ws_server_tx_disconnected()
{
    robot_disconnected();
}


void
ws_server_tx_start()
{
    ESP_LOGI(TAG, "starting websocket TX server.");
    int ret;

    handler.port = 9999;
    handler.opened = ws_server_tx_connected;
    handler.closed = ws_server_tx_disconnected;
    handler.conn = NULL;
    handler.rx = NULL;

    WebSocket_tx_queue = xQueueCreate( 2, sizeof(WebSocket_frame_t) );
    ret = xTaskCreate(&ws_server_tx_task, "ws_process_tx", 2048, NULL, 5, NULL);

    if (ret != 1)
    {
        ESP_LOGI(TAG, "create task ws_process_TX failed %d", ret);
        return;
    }

    ret = xTaskCreate(&ws_server_task, WS_SERVER_TASK_TX_NAME, 2048, &handler, 5, NULL);

    if (ret != 1)
    {
        ESP_LOGI(TAG, "create task TX ws_server failed %d", ret);
        return;
    }

    ESP_LOGI(TAG, "websocket TX server available.");
}

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

#include "ws_server.h"
#include "ws_server_task.h"
#include "ws_receive_task.h"
#include "esp_system.h"
#include "esp_log.h"

const static char * TAG = "WS_Server";

QueueHandle_t WebSocket_rx_queue;

void
ws_server_start(){
    ESP_LOGI(TAG, "starting websocket server.");
    int ret;

    ret = xTaskCreate(&ws_receive, "ws_process_rx", 2048, NULL, 5, NULL);

    if (ret != 1) {
        ESP_LOGI(TAG, "create task ws_process_rx failed %d", ret);
        return;
    }

    ret = xTaskCreate(&ws_server, "ws_server", 2048, NULL, 5, NULL);

    if (ret != 1) {
        ESP_LOGI(TAG, "create task ws_server failed %d", ret);
        return;
    }

    ESP_LOGI(TAG, "websocket server available.");
}

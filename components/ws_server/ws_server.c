// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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

// #include "ws_server_task.h"
// #include <stdio.h>
// #include <string.h>
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

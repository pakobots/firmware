#include "robot.h"
#include "ws_server.h"
#include "wifi.h"
#include "ble.h"
#include "http_server.h"
#include "esp_event_loop.h"
#include "storage.h"


void
ap_connected_callback(){
    http_server_start();
    ws_server_start();
}

void
app_main(void){
    storage_enable();
    robot_enable();
    ble_enable();
    wifi_enable(ap_connected_callback);
}

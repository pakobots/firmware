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

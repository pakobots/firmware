/*
 * This file is part of Pako Bots Firmware.
 *
 *  Pako Bots Firmware is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Pako Bots Firmware is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Pako Bots Firmware.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "http_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"

#include "nvs_flash.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include "sdkconfig.h"
#include "wifi.h"
#include "storage.h"
#include "robot.h"

const static char * TAG = "HTTP_Server";

#define STAT_404 \
    "HTTP/1.1 404 Not Found\r\n" \
    "Server: Pako Bots/2.2.14\r\n" \
    "Access-Control-Allow-Origin: *\r\n" \
    "Content-Length: 0\r\n\r\n";

#define STAT_200 \
    "HTTP/1.1 200 OK\r\n" \
    "Server: Pako Bots/2.2.14\r\n" \
    "Access-Control-Allow-Origin: *\r\n" \
    "Content-Length: 0\r\n\r\n";

#define STAT_500 \
    "HTTP/1.1 500 OK\r\n" \
    "Server: Pako Bots/2.2.14\r\n" \
    "Access-Control-Allow-Origin: *\r\n" \
    "Content-Length: 0\r\n\r\n";

#define SCAN_RESULT \
    "HTTP/1.1 200\r\n" \
    "Server: Pako Bots/2.2.14\r\n" \
    "Content-Type: application/json; charset=utf-8\r\n" \
    "Content-Length: %d\r\n\r\n" \
    "{\"s\":0,\"r\":";

const char scan_results[]    = SCAN_RESULT;
const int scan_results_bytes = sizeof(scan_results);
const char stat_404[]        = STAT_404;
const int stat_404_bytes     = sizeof(stat_404);
const char stat_200[]        = STAT_200;
const int stat_200_bytes     = sizeof(stat_200);
const char stat_500[]        = STAT_500;
const int stat_500_bytes     = sizeof(stat_500);

static void
getPath(char * recv_buf, char * path, int max_length){
    char * a = strstr(recv_buf, "GET ");
    char * b = strstr(recv_buf, " HTTP/1.1");

    if (!a || !b || b < a + 4 || b - a > max_length - 1) {
        path[0] = '/';
        path[1] = '\0';
        return;
    }

    a += 4;
    int len = b - a;
    strncpy(path, a, len);
    path[len] = '\0';
    ESP_LOGI(TAG, "----------------- \n %s \n -------------------", path);
}

static void
sendFile(char * recv_buf, char * path, int socket){
    char buf[512];

    ESP_LOGI(TAG, "Sending data ......");

    if (strcmp(path, "") == 0 || strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
        ESP_LOGI(TAG, "index.html");
        snprintf(buf, sizeof(buf),
          "HTTP/1.1 200 OK\r\n"
          "Content-type: text/html\r\n\r\n"
          "yep");
        write(socket, buf, strlen(buf));
    } else if (strncmp(path, "/wifi/connect?", 14) == 0) {
        if (strlen(path) > 256 || strlen(path) == 14) {
            write(socket, stat_500, stat_500_bytes);
        }
        char ssid[256] = "";
        char pass[256] = "";
        int pos        = (strchr(path, '|') ? strchr(path, '|') - path : false);
        if (pos) {
            memcpy(ssid, &path[14], pos - 14);
            memcpy(pass, &path[pos + 1], strlen(path) - pos);
        } else {
            memcpy(ssid, &path[14], strlen(path) - 14);
        }
        wifi_connect(ssid, pass, true);
    } else if (strcmp(path, "/wifi/list") == 0) {
        unsigned char * result;
        char data[128];
        switch (wifi_scan()) {
            case WIFI_SCAN_FAILED:
                write(socket, stat_500, stat_500_bytes);
                break;
            case WIFI_SCAN_FINISHED:
                result = wifi_scan_result();
                snprintf(data, 128, scan_results, strlen((char *) result) + 12);
                data[127] = '\0';
                write(socket, data, strlen(data));
                write(socket, result, strlen((char *) result));
                write(socket, "}", 1);
                break;
            default:
                write(socket, stat_200, stat_200_bytes);
        }
    } else if (strcmp(path, "/light/blue/on") == 0) {
        robot_light_blue(1);
        write(socket, stat_200, stat_200_bytes);
    } else if (strncmp(path, "/robot/name?", 12) == 0) {
        if (strlen(path) == 12 && strlen(path) < 48) {
            write(socket, stat_500, stat_500_bytes);
            return;
        }
        storage_set(PROPERTIES_STORAGE_KEY, "name", strchr(path, '?') + 1);
        write(socket, stat_200, stat_200_bytes);
    } else if (strcmp(path, "/robot/name") == 0) {
        size_t len;
        char data[48]   = "Nick";
        char buf[256]   = "";
        char * template = "HTTP/1.1 200 OK\r\n" \
          "Access-Control-Allow-Origin: *\r\n" \
          "Content-Length: %d\r\n" \
          "Content-type: application/json\r\n\r\n" \
          "{\"name\":\"%s\"}";
        storage_len(PROPERTIES_STORAGE_KEY, "name", &len);
        if (len > 0) {
            storage_get(PROPERTIES_STORAGE_KEY, "name", (char *) &data, &len);
        }

        snprintf(buf, sizeof(buf), template, strlen(data) + 11, data);
        write(socket, buf, strlen(buf));
    } else if (strcmp(path, "/light/blue/off") == 0) {
        robot_light_blue(0);
        write(socket, stat_200, stat_200_bytes);
    } else if (strcmp(path, "/light/green/on") == 0) {
        robot_light_green(1);
        write(socket, stat_200, stat_200_bytes);
    } else if (strcmp(path, "/light/green/off") == 0) {
        robot_light_green(0);
        write(socket, stat_200, stat_200_bytes);
    } else if (strcmp(path, "/light/red/on") == 0) {
        robot_light_red(1);
        write(socket, stat_200, stat_200_bytes);
    } else if (strcmp(path, "/light/red/off") == 0) {
        robot_light_red(0);
        write(socket, stat_200, stat_200_bytes);
    } else if (strncmp(path, "/motor/speed", 12) == 0) {
        int m1, m2;
        sscanf(path, "/motor/speed/%d/%d", &m1, &m2);
        m1 = m1 < 0 ? 0 : m1 > 100 ? 100 : m1;
        m2 = m2 < 0 ? 0 : m2 > 100 ? 100 : m2;
        robot_speed(m1, m2);
        write(socket, stat_200, stat_200_bytes);
    } else if (strcmp(path, "/motor/forward") == 0) {
        robot_fwd();
        write(socket, stat_200, stat_200_bytes);
    } else if (strcmp(path, "/motor/backward") == 0) {
        robot_bck();
        write(socket, stat_200, stat_200_bytes);
    } else if (strcmp(path, "/motor/left") == 0) {
        robot_left();
        write(socket, stat_200, stat_200_bytes);
    } else if (strcmp(path, "/motor/right") == 0) {
        robot_right();
        write(socket, stat_200, stat_200_bytes);
    } else if (strcmp(path, "/motor/stop") == 0) {
        robot_stop();
        write(socket, stat_200, stat_200_bytes);
    } else {
        write(socket, stat_404, stat_404_bytes);
    }
} /* sendFile */

/* httpd_task */
static void
httpd_task(void * p){
    ESP_LOGI(TAG, "Starting HTTPD Server ......");
    int socket, new_socket, ret;
    socklen_t addr_len;
    struct sockaddr_in sock_addr;

    // TODO parameterize this one
    char recv_buf[HTTP_SERVER_RECV_BUF_LEN];

    ESP_LOGI(TAG, "Server socket init ......");
    socket = socket(AF_INET, SOCK_STREAM, 0);
    if (socket < 0) {
        ESP_LOGI(TAG, "failed2 bottom");
        goto failed2;
    }
    ESP_LOGI(TAG, "OK");

    ESP_LOGI(TAG, "Server socket bind ......");
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family      = AF_INET;
    sock_addr.sin_addr.s_addr = 0;
    sock_addr.sin_port        = htons(HTTP_SERVER_PORT);
    ret = bind(socket, (struct sockaddr *) &sock_addr, sizeof(sock_addr));
    if (ret) {
        ESP_LOGI(TAG, "failed top");
        goto failed3;
    }
    ESP_LOGI(TAG, "OK");

    ESP_LOGI(TAG, "Server socket listen ......");
    ret = listen(socket, 32);
    if (ret) {
        ESP_LOGI(TAG, "failed3 middle");
        goto failed3;
    }
    ESP_LOGI(TAG, "OK");

reconnect:
    ESP_LOGI(TAG, "Server socket accept client ......");
    new_socket = accept(socket, (struct sockaddr *) &sock_addr, &addr_len);
    if (new_socket < 0) {
        ESP_LOGI(TAG, "socket was empty");
        goto reconnect;
    }
    ESP_LOGI(TAG, "OK");

    ESP_LOGI(TAG, "Server read message ......");
    do {
        memset(recv_buf, 0, HTTP_SERVER_RECV_BUF_LEN);
        char path[50];

        read(new_socket, recv_buf, HTTP_SERVER_RECV_BUF_LEN - 1);
        getPath(recv_buf, path, 50);

        sendFile(recv_buf, path, new_socket);
        break;
    } while (1);
    ESP_LOGI(TAG, "No more data in buffer.");
    close(new_socket);
    new_socket = -1;
    goto reconnect;
failed3:
    close(socket);
    socket = -1;
failed2:
    ESP_LOGI(TAG, "Failed to initiate socket");
    vTaskDelete(NULL);
} /* httpd_task */

void
http_server_start(){
    int ret;

    ret = xTaskCreate(httpd_task, HTTP_SERVER_TASK_NAME, HTTP_SERVER_TASK_STACK_WORDS, NULL, HTTP_SERVER_TASK_PRORIOTY,
        NULL);

    if (ret != 1) {
        ESP_LOGI(TAG, "create task http_server failed %d", ret);
    }
}

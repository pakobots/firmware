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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_event.h"
#include "esp_log.h"
#include "wifi.h"
#include "tcpip_adapter.h"
#include "storage.h"

const static char * TAG = "WIFI";

#define WIFI_STORAGE_KEY "wifi"

evt_connected_callback connected_callback;
static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;
unsigned char * scan_result    = 0;
unsigned int scan_state        = WIFI_SCAN_INIT;
unsigned long scan_last        = 0;
unsigned int connected         = false;
unsigned int sta_disconnects   = 0;

void
wifi_ap(void);

void
wifi_clear_ssid(){
    storage_set(WIFI_STORAGE_KEY, "ssid", "");
    storage_set(WIFI_STORAGE_KEY, "pass", "");
}

int
wifi_event_handler(void * ctx, system_event_t * event){
    wifi_mode_t mode;

    esp_event_process_default(event);

    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            printf("Station connected to external AP for IPV4.\n");
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            connected       = 1;
            sta_disconnects = 0;
            connected_callback();
            break;
        case SYSTEM_EVENT_AP_STA_GOT_IP6:
            printf("Station connected to internal AP for IPV6.\n");
            connected_callback();
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            connected = 0;
            printf("Station disconnected from internal AP.\n");
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            sta_disconnects++;
            if (sta_disconnects > 20) {
                printf("Switching to AP mode due to too many disconnect errors on AP");
                wifi_clear_ssid();
                sta_disconnects = 0;
                wifi_ap();
            } else {
                esp_wifi_connect();
            }
            break;
        case SYSTEM_EVENT_AP_START:
            esp_wifi_get_mode(&mode);
            switch (mode) {
                case WIFI_MODE_AP:
                    printf("AP device starting in AP mode.\n");
                    connected_callback();
                    break;
                case WIFI_MODE_STA:
                    printf("AP device starting in Station mode.\n");
                    break;
                case WIFI_MODE_APSTA:
                    printf("AP device starting in AP/Station mode.\n");
                    connected_callback();
                    break;
                default:
                    break;
            }
            break;
        case SYSTEM_EVENT_WIFI_READY:
            printf("AP device ready.\n");
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            printf("AP station connected.\n");
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            printf("AP station disconnected.\n");
            break;
        case SYSTEM_EVENT_AP_PROBEREQRECVED:
            printf("AP probe request received.\n");
            break;
        case SYSTEM_EVENT_MAX:
            printf("AP system event max.\n");
            break;
        default:
            break;
    }
    return ESP_OK;
} /* wifi_event_handler */

int
scan(){
    ESP_LOGI(TAG, "WIFI starting scan");
    scan_last  = xTaskGetTickCount();
    scan_state = WIFI_SCAN_RUNNING;
    wifi_scan_config_t config;

    config.ssid              = 0;
    config.bssid             = 0;
    config.channel           = 0;
    config.show_hidden       = false;
    config.scan_type         = WIFI_SCAN_TYPE_PASSIVE;
    config.scan_time.passive = WIFI_SCANNING_TIME;

    if (esp_wifi_scan_start(&config, false) == ESP_OK) {
        ESP_LOGI(TAG, "WIFI scan running");
        scan_state = WIFI_SCAN_RUNNING;
    } else {
        ESP_LOGI(TAG, "WIFI scan failed");
        scan_state = WIFI_SCAN_FAILED;
    }
    return scan_state;
}

void
recToJson(wifi_ap_record_t * record, char * buf, int len){
    char ssid[33];
    char * auth;

    switch (record->authmode) {
        case WIFI_AUTH_OPEN:
            auth = "OPEN";
            break;
        case WIFI_AUTH_WEP:
            auth = "WEP";
            break;
        case WIFI_AUTH_WPA_PSK:
            auth = "WPA";
            break;
        case WIFI_AUTH_WPA2_PSK:
            auth = "WPA2";
            break;
        case WIFI_AUTH_WPA_WPA2_PSK:
            auth = "WPA_WPA2";
            break;
        case WIFI_AUTH_WPA2_ENTERPRISE:
            auth = "ENT_WPA2";
            break;
        case WIFI_AUTH_MAX:
            auth = "MAX";
            break;
        default:
            auth = "unknown";
    }

    snprintf(ssid, 32, "%s", record->ssid);
    ssid[32] = '\0';
    snprintf(buf, len,
      "{\"bssid\":\"%02x:%02x:%02x:%02x:%02x:%02x\", \
        \"ssid\":\"%s\", \
        \"channel\":\"%d\", \
        \"rssi\":\"%d\", \
        \"auth\":\"%s\" \
      },",
      record->bssid[0],
      record->bssid[1],
      record->bssid[2],
      record->bssid[3],
      record->bssid[4],
      record->bssid[5],
      ssid,
      record->primary,
      record->rssi,
      auth);
} /* recToJson */

int
wifi_scan(){
    switch (scan_state) {
        case WIFI_SCAN_INIT:
        case WIFI_SCAN_FAILED:
            return scan();

        case WIFI_SCAN_FINISHED:
            return (xTaskGetTickCount() - scan_last < WIFI_RESCAN_TIMEOUT) ? WIFI_SCAN_FINISHED : scan();

        default:
            if (xTaskGetTickCount() - scan_last < WIFI_SCANNING_TIME + 100) {
                ESP_LOGI(TAG, "WIFI scan still running");
                return WIFI_SCAN_RUNNING;
            }
            ESP_LOGI(TAG, "WIFI scan getting results");
            uint16_t cnt;
            ESP_LOGI(TAG, "getting count");
            esp_wifi_scan_get_ap_num(&cnt);
            cnt = cnt > MAX_AP_FOUND ? MAX_AP_FOUND : cnt;
            if (cnt > 0) {
                int len;
                int pos = 1;
                char buf[256];
                scan_result    = malloc(256 * cnt);
                scan_result[0] = '[';
                ESP_LOGI(TAG, "WIFI scan found %d result(s)", cnt);
                wifi_ap_record_t records[cnt];
                esp_wifi_scan_get_ap_records(&cnt, records);
                while (cnt-- > 0) {
                    recToJson(&records[cnt], (char *) &buf, 256);
                    len = strnlen(buf, 256);
                    memcpy(scan_result + pos, buf, len);
                    // ESP_LOGI(TAG, "%s", buf);
                    pos += len;
                }
                scan_result[pos - 1] = ']';
                scan_result[pos]     = '\0';
                // ESP_LOGI(TAG, "%s", scan_result);
            }
            return (scan_state = WIFI_SCAN_FINISHED);
    }
    if (esp_wifi_scan_stop() == ESP_OK) {
        ESP_LOGI(TAG, "WIFI scan stop failed to stop prior scan");
        return scan();
    } else {
        return WIFI_SCAN_FAILED;
    }
} /* wifi_scan */

unsigned char *
wifi_scan_result(){
    return scan_result;
}

int
wifi_connected(){
    return connected;
}

int
wifi_connect(char * ssid, char * password, unsigned int persist){
    esp_wifi_disconnect();
    esp_wifi_stop();

    if (persist) {
        storage_set(WIFI_STORAGE_KEY, "ssid", ssid);
        storage_set(WIFI_STORAGE_KEY, "pass", password);
    }

    wifi_config_t wifi_config = {
        .sta          = {
            .ssid     = "",
            .password = ""
        }
    };
    memcpy(wifi_config.sta.ssid, ssid, strnlen(ssid, 32) + 1);
    memcpy(wifi_config.sta.password, password, strnlen(password, 64) + 1);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    return esp_wifi_start();
}

void
wifi_ap(){
    esp_wifi_disconnect();
    esp_wifi_stop();
    wifi_config_t ap_cnf = {
        .ap                  = {
            .ssid            = AP_SSID,
            .ssid_hidden     =               0,
            .ssid_len        = strnlen(AP_SSID, 32),
            .password        = AP_PASS,
            .authmode        = WIFI_AUTH_WPA2_PSK, // Set auth mode,default mode is open
            .max_connection  =               4,    // Set max connection
            .channel         =               1,
            .beacon_interval = 100
        }
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cnf));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void
wifi_enable(evt_connected_callback evt_callback){
    connected_callback = evt_callback;
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL) );
    wifi_init_config_t cnf = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cnf) );
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM) );

    size_t len = 0;
    storage_len(WIFI_STORAGE_KEY, "ssid", &len);
    if (len < 2) {
        printf("We don't have a ssid set for the wifi station. Need to switch to ap mode\n");
        wifi_ap();
    } else {
        char ssid[len + 1];
        ssid[len] = '\0';
        storage_get(WIFI_STORAGE_KEY, "ssid", (char *) &ssid, &len);
        len = 0;
        storage_len(WIFI_STORAGE_KEY, "pass", &len);
        char pass[len + 1];
        pass[len] = '\0';
        if (len == 0) {
            pass[0] = '\0';
        } else {
            storage_get(WIFI_STORAGE_KEY, "pass", (char *) &pass, &len);
        }
        printf("Connecting to wifi ap: %s %d\n", ssid, len);
        wifi_connect(ssid, pass, false);
    }
} /* wifi_enable */

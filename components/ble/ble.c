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

#include "ble.h"
#include "robot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "bta_api.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_main.h"

#include "sdkconfig.h"
#include "storage.h"

#define GATTS_TAG "GATTS"

///Declare the static function
static void
gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

#define GATTS_SERVICE_UUID_TEST_A   0x00FF
#define GATTS_CHAR_UUID_TEST_A      0xFF01
#define GATTS_DESCR_UUID_TEST_A     0x3333
#define GATTS_NUM_HANDLE_TEST_A     4

#define TEST_MANUFACTURER_DATA_LEN  17

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40

uint8_t char1_str[] = { 0x11, 0x22, 0x33 };
esp_attr_value_t gatts_demo_char1_val = {
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len     = sizeof(char1_str),
    .attr_value   = char1_str,
};

// static uint8_t test_manufacturer[8]={'P','a','k','o','B','o','t','s'};

//UUID = hex for "humans love robots"
static uint8_t test_service_uuid[16] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    0x68, 0x75, 0x6d, 0x61, 0x6e, 0x73, 0x6c, 0x6f, 0x76, 0x65, 0x72, 0x6f, 0x62, 0x6f, 0x00, 0x00
};

// static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
static esp_ble_adv_data_t test_adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = true,
    .min_interval        =                                                         0x20,
    .max_interval        =                                                         0x40,
    .appearance          =                                                         0x00,
    .manufacturer_len    =                                                            0, // TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL,                                                        // &test_manufacturer[0],
    .service_data_len    =                                                            0,
    .p_service_data      = NULL,
    .service_uuid_len    = sizeof(test_service_uuid),
    .p_service_uuid      = test_service_uuid,
    .flag                = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t test_adv_params = {
    .adv_int_min       =                              0x20,
    .adv_int_max       =                              0x40,
    .adv_type          = ADV_TYPE_IND,
    .own_addr_type     = BLE_ADDR_TYPE_PUBLIC,
    // .peer_addr            =
    // .peer_addr_type       =
    .channel_map       = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

#define PROFILE_NUM      1
#define PROFILE_A_APP_ID 0

struct gatts_profile_inst {
    esp_gatts_cb_t       gatts_cb;
    uint16_t             gatts_if;
    uint16_t             app_id;
    uint16_t             conn_id;
    uint16_t             service_handle;
    esp_gatt_srvc_id_t   service_id;
    uint16_t             char_handle;
    esp_bt_uuid_t        char_uuid;
    esp_gatt_perm_t      perm;
    esp_gatt_char_prop_t property;
    uint16_t             descr_handle;
    esp_bt_uuid_t        descr_uuid;
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gatts_cb = gatts_profile_a_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,             /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    }
};

static int MODE_FW_UPDATE = 0;

static void
gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            esp_ble_gap_start_advertising(&test_adv_params);
            break;
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            esp_ble_gap_start_advertising(&test_adv_params);
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
            esp_ble_gap_start_advertising(&test_adv_params);
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            // advertising start complete event to indicate advertising start successfully or failed
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
            {
                ESP_LOGE(GATTS_TAG, "Advertising start failed\n");
            }

            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
            {
                ESP_LOGE(GATTS_TAG, "Advertising stop failed\n");
            }
            else
            {
                ESP_LOGI(GATTS_TAG, "Stop adv successfully\n");
            }

            break;
        default:
            break;
    } /* switch */
}


static void
gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
        case ESP_GATTS_REG_EVT:
            ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
            gl_profile_tab[PROFILE_A_APP_ID].service_id.is_primary = true;
            gl_profile_tab[PROFILE_A_APP_ID].service_id.id.inst_id = 0x00;
            gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
            gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_A;

            size_t len;
            char data[48] = "Nick";
            storage_len(PROPERTIES_STORAGE_KEY, "name", &len);
            if (len > 0)
            {
                storage_get(PROPERTIES_STORAGE_KEY, "name", (char *)&data, &len);
            }

            esp_ble_gap_set_device_name(data);
            esp_ble_gap_config_adv_data(&test_adv_data);
            esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_A_APP_ID].service_id,
                                         GATTS_NUM_HANDLE_TEST_A);
            break;
        case ESP_GATTS_READ_EVT:
        {
            ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id,
                     param->read.trans_id, param->read.handle);
            esp_gatt_rsp_t rsp;
            memset( &rsp, 0, sizeof(esp_gatt_rsp_t) );
            rsp.attr_value.handle = param->read.handle;
            rsp.attr_value.len = 8;
            rsp.attr_value.value[0] = 0x4d;
            rsp.attr_value.value[1] = 0x4f;
            rsp.attr_value.value[2] = 0x56;
            rsp.attr_value.value[3] = 0x45;
            rsp.attr_value.value[4] = 0x20;
            rsp.attr_value.value[5] = 0x53;
            rsp.attr_value.value[6] = 0x56;
            rsp.attr_value.value[7] = 0x43;
            esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                        ESP_GATT_OK, &rsp);
            break;
        }
        case ESP_GATTS_WRITE_EVT:
        {
            // ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d\n", param->write.conn_id,
            //          param->write.trans_id, param->write.handle);
            // ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value %08x\n", param->write.len,
            //          *(uint32_t *) param->write.value);

            if (MODE_FW_UPDATE == 1)
            {
                // esp_err_t err = esp_ota_write( OTA_HANDLE, (const void *)param->write.value, param->write.len);
                // if (err != ESP_OK)
                // {
                //     MODE_FW_UPDATE = 0;
                //     ESP_LOGE(GATTS_TAG, "Error: esp_ota_write failed! err=0x%x", err);
                //     esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_ERROR, NULL);
                // }
                // else
                // {
                //     ESP_LOGI(GATTS_TAG, "esp_ota_write header OK");
                //     esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
                // }

                // if (esp_ota_end(update_handle) != ESP_OK) {
                //     ESP_LOGE(TAG, "esp_ota_end failed!");
                //     task_fatal_error();
                // }
                // err = esp_ota_set_boot_partition(update_partition);
                // if (err != ESP_OK) {
                //     ESP_LOGE(TAG, "esp_ota_set_boot_partition failed! err=0x%x", err);
                //     task_fatal_error();
                // }
                // ESP_LOGI(TAG, "Prepare to restart system!");
                // esp_restart();
            }
            else
            {
                int len = param->write.len < 20 ? param->write.len : 20;
                char cmd[len + 1];
                memcpy(cmd, param->write.value, len);
                cmd[len] = '\0';

                if (cmd[0] == 'U')
                {
                    // OTA_PARTITION = esp_ota_get_next_update_partition(NULL);
                    // int err = esp_ota_begin(OTA_PARTITION, OTA_SIZE_UNKNOWN, &OTA_HANDLE);
                    // if (err != ESP_OK)
                    // {
                    //     esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_ERROR, NULL);
                    // }
                    // else
                    // {
                    //     MODE_FW_UPDATE = 1;
                    //     esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
                    // }
                }
                else
                {
                    robot_cmd( cmd, sizeof(cmd) );
                }
            }

            // esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
            break;
        }
        case ESP_GATTS_EXEC_WRITE_EVT:
        case ESP_GATTS_MTU_EVT:
        case ESP_GATTS_CONF_EVT:
        case ESP_GATTS_UNREG_EVT:
            break;
        case ESP_GATTS_CREATE_EVT:
            ESP_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status,
                     param->create.service_handle);
            gl_profile_tab[PROFILE_A_APP_ID].service_handle = param->create.service_handle;
            gl_profile_tab[PROFILE_A_APP_ID].char_uuid.len = ESP_UUID_LEN_16;
            gl_profile_tab[PROFILE_A_APP_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_A;

            esp_ble_gatts_start_service(gl_profile_tab[PROFILE_A_APP_ID].service_handle);

            esp_ble_gatts_add_char(gl_profile_tab[PROFILE_A_APP_ID].service_handle,
                                   &gl_profile_tab[PROFILE_A_APP_ID].char_uuid,
                                   ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                   ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY,
                                   &gatts_demo_char1_val, NULL);
            break;
        case ESP_GATTS_ADD_INCL_SRVC_EVT:
            break;
        case ESP_GATTS_ADD_CHAR_EVT:
        {
            uint16_t length = 0;
            const uint8_t *prf_char;

            ESP_LOGI(GATTS_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                     param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
            gl_profile_tab[PROFILE_A_APP_ID].char_handle = param->add_char.attr_handle;
            gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
            gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
            esp_ble_gatts_get_attr_value(param->add_char.attr_handle, &length, &prf_char);

            ESP_LOGI(GATTS_TAG, "the gatts demo char length = %x\n", length);
            for (int i = 0; i < length; i++)
            {
                ESP_LOGI(GATTS_TAG, "prf_char[%x] =%x\n", i, prf_char[i]);
            }

            esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_A_APP_ID].service_handle,
                                         &gl_profile_tab[PROFILE_A_APP_ID].descr_uuid,
                                         ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
            break;
        }
        case ESP_GATTS_ADD_CHAR_DESCR_EVT:
            ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                     param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
            break;
        case ESP_GATTS_DELETE_EVT:
            break;
        case ESP_GATTS_START_EVT:
            ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                     param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_STOP_EVT:
            break;
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(GATTS_TAG, "CONNECTED\n");
            gl_profile_tab[PROFILE_A_APP_ID].conn_id = param->connect.conn_id;
            // robot_connected();
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(GATTS_TAG, "DISCONNECTED\n");
            robot_disconnected();
            esp_ble_gap_start_advertising(&test_adv_params);
            break;
        case ESP_GATTS_OPEN_EVT:
        case ESP_GATTS_CANCEL_OPEN_EVT:
        case ESP_GATTS_CLOSE_EVT:
        case ESP_GATTS_LISTEN_EVT:
        case ESP_GATTS_CONGEST_EVT:
        default:
            break;
    } /* switch */
} /* gatts_profile_a_event_handler */


static void
gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT)
    {
        if (param->reg.status == ESP_GATT_OK)
        {
            gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        }
        else
        {
            ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d\n",
                     param->reg.app_id,
                     param->reg.status);
            return;
        }
    }

    /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do
    {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++)
        {
            if (gatts_if == ESP_GATT_IF_NONE ||             /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                gatts_if == gl_profile_tab[idx].gatts_if)
            {
                if (gl_profile_tab[idx].gatts_cb)
                {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    }
    while (0);
}


void
ble_enable()
{
    esp_err_t ret;

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    ret = esp_bt_controller_init(&bt_cfg);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "%s initialize controller failed\n", __func__);
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "%s enable controller failed\n", __func__);
        return;
    }

    ret = esp_bluedroid_init();
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "%s init bluetooth failed\n", __func__);
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed\n", __func__);
        return;
    }

    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_app_register(PROFILE_A_APP_ID);
} /* ble_enable */

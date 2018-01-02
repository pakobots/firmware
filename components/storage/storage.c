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

#include "esp_partition.h"
#include "nvs_flash.h"
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "nvs.h"

static esp_ota_handle_t OTA_HANDLE = 0;
const esp_partition_t *OTA_PARTITION = NULL;

int
storage_set(char *store, char *key, char *value)
{
    nvs_handle store_handler;
    esp_err_t err;

    err = nvs_open(store, NVS_READWRITE, &store_handler);
    if (err != ESP_OK)
    {
        return err;
    }

    err = nvs_set_str(store_handler, key, value);

    if (err != ESP_OK)
    {
        return err;
    }

    err = nvs_commit(store_handler);
    if (err != ESP_OK)
    {
        return err;
    }

    nvs_close(store_handler);
    return ESP_OK;
}


int
storage_len(char *store, char *key, size_t *len)
{
    nvs_handle store_handler;
    esp_err_t err;

    err = nvs_open(store, NVS_READWRITE, &store_handler);
    if (err != ESP_OK)
    {
        return err;
    }

    err = nvs_get_str(store_handler, key, NULL, len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        return err;
    }

    nvs_close(store_handler);
    return ESP_OK;
}


int
storage_get(char *store, char *key, char *value, size_t *len)
{
    nvs_handle store_handler;
    esp_err_t err;

    err = nvs_open(store, NVS_READWRITE, &store_handler);
    if (err != ESP_OK)
    {
        return err;
    }

    err = nvs_get_str(store_handler, key, value, len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        return err;
    }

    nvs_close(store_handler);
    return ESP_OK;
}


int storage_init_upgrade(size_t size)
{
    if (OTA_HANDLE)
    {
        return 0;
    }

    OTA_PARTITION = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI("STORAGE", "Writing to partition subtype %d at offset 0x%x",
             OTA_PARTITION->subtype, OTA_PARTITION->address);
    return esp_ota_begin(OTA_PARTITION, size, &OTA_HANDLE);
}


int storage_upgrade(const void *data, size_t size)
{
    if (!OTA_HANDLE)
    {
        return -1;
    }

    return esp_ota_write(OTA_HANDLE, data, size);
}


int storage_fin_upgrade()
{
    int err;
    err = esp_ota_end(OTA_HANDLE);
    if (err != ESP_OK)
    {
        OTA_HANDLE = 0;
        ESP_LOGE("STORAGE", "esp_ota_end failed!");
        return err;
    }

    err = esp_ota_set_boot_partition(OTA_PARTITION);
    if (err != ESP_OK)
    {
        OTA_HANDLE = 0;
        ESP_LOGE("STORAGE", "esp_ota_set_boot_partition failed!");
        return err;
    }

    OTA_PARTITION = NULL;
    OTA_HANDLE = 0;

    return 0;
}


void
storage_enable()
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        const esp_partition_t *nvs_partition = esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
        assert(nvs_partition && "partition table must have an NVS partition");
        ESP_ERROR_CHECK( esp_partition_erase_range(nvs_partition, 0, nvs_partition->size) );
        err = nvs_flash_init();
    }
}

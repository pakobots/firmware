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

#include "esp_partition.h"
#include "nvs_flash.h"
#include "nvs.h"

int
storage_set(char * store, char * key, char * value){
    nvs_handle store_handler;
    esp_err_t err;

    err = nvs_open(store, NVS_READWRITE, &store_handler);
    if (err != ESP_OK) return err;

    err = nvs_set_str(store_handler, key, value);

    if (err != ESP_OK) return err;

    err = nvs_commit(store_handler);
    if (err != ESP_OK) return err;

    nvs_close(store_handler);
    return ESP_OK;
}

int
storage_len(char * store, char * key, size_t * len){
    nvs_handle store_handler;
    esp_err_t err;

    err = nvs_open(store, NVS_READWRITE, &store_handler);
    if (err != ESP_OK) return err;

    err = nvs_get_str(store_handler, key, NULL, len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

    nvs_close(store_handler);
    return ESP_OK;
}

int
storage_get(char * store, char * key, char * value, size_t * len){
    nvs_handle store_handler;
    esp_err_t err;

    err = nvs_open(store, NVS_READWRITE, &store_handler);
    if (err != ESP_OK) return err;

    err = nvs_get_str(store_handler, key, value, len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

    nvs_close(store_handler);
    return ESP_OK;
}

void
storage_enable(){
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        const esp_partition_t * nvs_partition = esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
        assert(nvs_partition && "partition table must have an NVS partition");
        ESP_ERROR_CHECK(esp_partition_erase_range(nvs_partition, 0, nvs_partition->size) );
        err = nvs_flash_init();
    }
}

//
// Created by Mathieu Durand on 2025-03-26.
//

#include "storage.h"
#include "nvs.h"
#include "esp_err.h"
#include "esp_log.h"

#define STORAGE_NAMESPACE "storage"
static const char *TAG = "NVS_STORAGE";

esp_err_t get_segment_id(uint8_t *segment_id) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    }

    err = nvs_get_u8(my_handle, "segment_id", segment_id);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "segment_id not found, setting default (0)");
        *segment_id = 0;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) reading segment_id!", esp_err_to_name(err));
    }

    nvs_close(my_handle);
    return err;
}

esp_err_t set_segment_id(uint8_t segment_id) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_u8(my_handle, "segment_id", segment_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) writing segment_id!", esp_err_to_name(err));
    }

    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) committing NVS storage!", esp_err_to_name(err));
    }

    nvs_close(my_handle);
    return err;
}
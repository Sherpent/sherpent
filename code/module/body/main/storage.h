//
// Created by Mathieu Durand on 2025-03-26.
//

#ifndef BODY_STORAGE_H
#define BODY_STORAGE_H

#include "nvs.h"
#include "esp_err.h"
#include "esp_log.h"

esp_err_t get_segment_id(uint8_t *segment_id);
esp_err_t set_segment_id(uint8_t segment_id);

#endif //BODY_STORAGE_H

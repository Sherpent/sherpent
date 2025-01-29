#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "esp_log.h"

#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "communication.h"

#include "sdkconfig.h"

#define TAG "BLE_SERVER"

/* ============================ FUNCTION DECLARAITON ============================= */
static void event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_interface, esp_ble_gatts_cb_param_t *param);
static void handle_general_access_profile_event(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

#define SERVICE_UUID 0x00FF
#define CHARACTERISTIC_UUID 0xFF01
#define NUM_HANDLES 4

#define MAX_CHAR_VAL_LEN 0x40

#define ADV_CONFIG_FLAG (1 << 0)
#define SCAN_RSP_CONFIG_FLAG (1 << 1)

/* ============================== TYPE DEFINITIONS =============================== */
typedef void (*message_handler_t)(struct Message*);


/* ============================== PRIVATE VARIABLES ============================== */
static message_handler_t _message_handler = NULL;
static char _device_name[ESP_BLE_ADV_NAME_LEN_MAX];

static uint8_t characteristic_value[] = {0x11, 0x22, 0x33};
static esp_gatt_char_prop_t char_properties = 0;

static uint8_t adv_config_done = 0;
static uint8_t service_uuid[32] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00,
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

static esp_attr_value_t characteristic_attr_value = {
    .attr_max_len = MAX_CHAR_VAL_LEN,
    .attr_len = sizeof(characteristic_value),
    .attr_value = characteristic_value,
};


static esp_ble_adv_data_t advertisement_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(service_uuid),
    .p_service_uuid = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_data_t scan_response_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(service_uuid),
    .p_service_uuid = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t advertisement_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatt_profile_instance {
    esp_gatts_cb_t gatts_callback;
    uint16_t gatts_interface;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

static struct gatt_profile_instance server_profile = {
    .gatts_callback = event_handler,
    .gatts_interface = ESP_GATT_IF_NONE,
};

typedef struct {
    uint8_t *prepare_buf;
    int prepare_len;
} prepare_buffer_t;

static void handle_general_access_profile_event(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0) {
                esp_ble_gap_start_advertising(&advertisement_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0) {
                esp_ble_gap_start_advertising(&advertisement_params);
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Failed to start advertising, status %d", param->adv_start_cmpl.status);
                break;
            }
            ESP_LOGI(TAG, "Advertising started successfully");
            break;
        default:
            break;
    }
}

static void event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_interface, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT:
            ESP_LOGI(TAG, "GATT server register, status %d, app_id %d, gatts_interface %d", param->reg.status,
                     param->reg.app_id, gatts_interface);
            server_profile.service_id.is_primary = true;
            server_profile.service_id.id.inst_id = 0x00;
            server_profile.service_id.id.uuid.len = ESP_UUID_LEN_16;
            server_profile.service_id.id.uuid.uuid.uuid16 = SERVICE_UUID;

            esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(_device_name);
            if (set_dev_name_ret) {
                ESP_LOGE(TAG, "set device name failed, error code = %x", set_dev_name_ret);
            }
            //config adv data
            esp_err_t ret = esp_ble_gap_config_adv_data(&advertisement_data);
            if (ret) {
                ESP_LOGE(TAG, "config adv data failed, error code = %x", ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;
            //config scan response data
            ret = esp_ble_gap_config_adv_data(&scan_response_data);
            if (ret) {
                ESP_LOGE(TAG, "config scan response data failed, error code = %x", ret);
            }
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;
            esp_ble_gatts_create_service(gatts_interface, &server_profile.service_id, NUM_HANDLES);
            break;
        case ESP_GATTS_READ_EVT: {
            ESP_LOGI(TAG, "Characteristic read, conn_id %d, trans_id %" PRIu32 ", handle %d", param->read.conn_id,
                     param->read.trans_id, param->read.handle);
            esp_gatt_rsp_t rsp;
            memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
            rsp.attr_value.handle = param->read.handle;
            rsp.attr_value.len = 3;
            for (int i = 0; i < 3; i++) {
                rsp.attr_value.value[i] = 0;
            }
            esp_ble_gatts_send_response(gatts_interface, param->read.conn_id, param->read.trans_id,
                                        ESP_GATT_OK, &rsp);
            break;
        }
        case ESP_GATTS_WRITE_EVT: {
            ESP_LOGI(TAG, "Characteristic write, conn_id %d, trans_id %" PRIu32 ", handle %d",
                     param->write.conn_id, param->write.trans_id, param->write.handle);

            ESP_LOGI(TAG, "value len %d, value ", param->write.len);
            ESP_LOG_BUFFER_HEX(TAG, param->write.value, param->write.len);

            ESP_LOGI(TAG, "First byte %d", param->write.value[0]);

            struct Message *message = (struct Message*) malloc(param->write.value[0]);
            memcpy(message, param->write.value, param->write.value[0]);
            _message_handler(message);
            break;
        }
        case ESP_GATTS_EXEC_WRITE_EVT:
            ESP_LOGI(TAG, "Execute write");
            esp_ble_gatts_send_response(gatts_interface, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
            break;
        case ESP_GATTS_CREATE_EVT:
            ESP_LOGI(TAG, "Service create, status %d, service_handle %d", param->create.status,
                     param->create.service_handle);
            server_profile.service_handle = param->create.service_handle;
            server_profile.char_uuid.len = ESP_UUID_LEN_16;
            server_profile.char_uuid.uuid.uuid16 = CHARACTERISTIC_UUID;

            esp_ble_gatts_start_service(server_profile.service_handle);
            char_properties = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
            esp_err_t add_char_ret = esp_ble_gatts_add_char(server_profile.service_handle, &server_profile.char_uuid,
                                                            ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                            char_properties,
                                                            &characteristic_attr_value, NULL);
            if (add_char_ret) {
                ESP_LOGE(TAG, "add char failed, error code =%x", add_char_ret);
            }
            break;
        case ESP_GATTS_ADD_CHAR_EVT: {
            uint16_t length = 0;
            const uint8_t *prf_char;

            ESP_LOGI(TAG, "Characteristic add, status %d, attr_handle %d, service_handle %d",
                     param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
            server_profile.char_handle = param->add_char.attr_handle;
            server_profile.descr_uuid.len = ESP_UUID_LEN_16;
            server_profile.descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
            esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle, &length, &prf_char);
            if (get_attr_ret == ESP_FAIL) {
                ESP_LOGE(TAG, "ILLEGAL HANDLE");
            }

            ESP_LOGI(TAG, "the gatts demo char length = %x", length);
            for (int i = 0; i < length; i++) {
                ESP_LOGI(TAG, "prf_char[%x] =%x", i, prf_char[i]);
            }
            esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(server_profile.service_handle, &server_profile.descr_uuid,
                                                                   ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL,
                                                                   NULL);
            if (add_descr_ret) {
                ESP_LOGE(TAG, "add char descr failed, error code =%x", add_descr_ret);
            }
            break;
        }
        case ESP_GATTS_ADD_CHAR_DESCR_EVT:
            server_profile.descr_handle = param->add_char_descr.attr_handle;
            ESP_LOGI(TAG, "Descriptor add, status %d, attr_handle %d, service_handle %d",
                     param->add_char_descr.status, param->add_char_descr.attr_handle,
                     param->add_char_descr.service_handle);
            break;
        case ESP_GATTS_START_EVT:
            ESP_LOGI(TAG, "Service start, status %d, service_handle %d",
                     param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_CONNECT_EVT: {
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
            conn_params.latency = 0;
            conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
            conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
            conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
            ESP_LOGI(TAG, "Connected, conn_id %u, remote "ESP_BD_ADDR_STR"",
                     param->connect.conn_id, ESP_BD_ADDR_HEX(param->connect.remote_bda));
            server_profile.conn_id = param->connect.conn_id;
            //start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);
            break;
        }
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(TAG, "Disconnected, remote "ESP_BD_ADDR_STR", reason 0x%02x",
                     ESP_BD_ADDR_HEX(param->disconnect.remote_bda), param->disconnect.reason);
            esp_ble_gap_start_advertising(&advertisement_params);
            break;
        case ESP_GATTS_CONF_EVT:
            ESP_LOGI(TAG, "Confirm receive, status %d, attr_handle %d", param->conf.status, param->conf.handle);
            if (param->conf.status != ESP_GATT_OK) {
                ESP_LOG_BUFFER_HEX(TAG, param->conf.value, param->conf.len);
            }
            break;
        default:
            break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_interface, esp_ble_gatts_cb_param_t *param) {
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status != ESP_GATT_OK) {
            ESP_LOGI(TAG, "Reg app failed, app_id %04x, status %d",
                     param->reg.app_id, param->reg.status);
            return;
        }
        server_profile.gatts_interface = gatts_interface;
    }

    event_handler(event, gatts_interface, param);
}

void server_main(char device_name[ESP_BLE_ADV_NAME_LEN_MAX], message_handler_t message_handler) {
    memcpy(_device_name, device_name, ESP_BLE_ADV_NAME_LEN_MAX);
    _message_handler = message_handler;

    esp_err_t ret;

    // Note: Avoid performing time-consuming operations within callback functions.
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "gatts register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gap_register_callback(handle_general_access_profile_event);
    if (ret) {
        ESP_LOGE(TAG, "gap register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gatts_app_register(0);
    if (ret) {
        ESP_LOGE(TAG, "gatts app register error, error code = %x", ret);
        return;
    }
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret) {
        ESP_LOGE(TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }
}
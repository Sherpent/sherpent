#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "communication.h"
#include "target.h"
#include "utils.h"

#include "sdkconfig.h"

#define TAG "BLE_SERVER"

/* ============================ FUNCTION DECLARAITON ============================= */
static void server_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_interface, esp_ble_gatts_cb_param_t *param);
static void handle_general_access_profile_event(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

#define SERVICE_UUID 0x00FF
#define CHARACTERISTIC_UUID 0xFF01
#define NUM_HANDLES 4

#define MAX_CHAR_VAL_LEN 0x40

#define ADV_CONFIG_FLAG (1 << 0)
#define SCAN_RSP_CONFIG_FLAG (1 << 1)

#define GATTC_TAG "GATTC_DEMO"
#define REMOTE_SERVICE_UUID        0x00FF
#define REMOTE_NOTIFY_CHAR_UUID    0xFF01
#define PROFILE_A_APP_ID 2
#define INVALID_HANDLE   0

/* ============================== TYPE DEFINITIONS =============================== */
struct ble_scan_result_t {
    char name[32];          // Human-readable name
    uint8_t mac[6];         // MAC address
    int rssi;               // Signal strength
    uint8_t adv_type;       // Advertisement type
    uint8_t phy_primary;    // Primary PHY
    uint8_t phy_secondary;  // Secondary PHY
    uint16_t appearance;    // Device appearance (if present)
    uint16_t service_uuid;  // First service UUID (if present)
};

typedef void (*message_handler_t)(struct Message*, enum Target from);
typedef void (*scan_handler_t)(struct ble_scan_result_evt_param result);
typedef void (*message_provider_t)(struct Message*, enum Target to);


/* ============================== PRIVATE VARIABLES ============================== */
static message_handler_t _message_handler = NULL;
static scan_handler_t _scan_handler = NULL;
static char _device_name[ESP_BLE_ADV_NAME_LEN_MAX];

static optional_type(int16_t) front_module_id = {false, 0};
static optional_type(int16_t) back_module_id = {false, 0};

static uint8_t characteristic_value[] = {0x11, 0x22, 0x33};
static esp_gatt_char_prop_t char_properties = 0;

static uint8_t adv_config_done = 0;
static uint8_t service_uuid[32] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00,
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

static bool connect    = false;
static bool get_server = false;
static esp_gattc_char_elem_t *char_elem_result   = NULL;
static esp_gattc_descr_elem_t *descr_elem_result = NULL;

static esp_bt_uuid_t remote_filter_service_uuid = {
        .len = ESP_UUID_LEN_16,
        .uuid = {.uuid16 = REMOTE_SERVICE_UUID,},
};

static esp_bt_uuid_t remote_filter_char_uuid = {
        .len = ESP_UUID_LEN_16,
        .uuid = {.uuid16 = REMOTE_NOTIFY_CHAR_UUID,},
};

static esp_bt_uuid_t notify_descr_uuid = {
        .len = ESP_UUID_LEN_16,
        .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
};

static esp_ble_scan_params_t ble_scan_params = {
        .scan_type              = BLE_SCAN_TYPE_ACTIVE,
        .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
        .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_interval          = 0x50,
        .scan_window            = 0x30,
        .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

struct gattc_profile_inst {
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t char_handle;
    esp_bd_addr_t remote_bda;
};

/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT */
static struct gattc_profile_inst client_profile = {
        .gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
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
    .gatts_interface = ESP_GATT_IF_NONE,
};

typedef struct {
    uint8_t *prepare_buf;
    int prepare_len;
} prepare_buffer_t;

void parse_advertising_data(uint8_t *adv_data, uint8_t adv_len, struct ble_scan_result_t *result) {
    uint8_t index = 0;

    while (index < adv_len) {
        uint8_t length = adv_data[index];  // First byte = length of this field
        if (length == 0) break;            // End of data
        if (index + length >= adv_len) break;  // Prevent overflow

        uint8_t type = adv_data[index + 1];  // Second byte = type
        uint8_t *data = &adv_data[index + 2]; // Pointer to actual data

        switch (type) {
            case 0x09: // Complete Local Name
            case 0x08: // Shortened Local Name
                memset(result->name, 0, sizeof(result->name));
                strncpy(result->name, (char *)data, length - 1);
                break;

            case 0x19: // Appearance
                result->appearance = data[0] | (data[1] << 8);
                break;

            case 0x02: // Incomplete List of 16-bit Service UUIDs
            case 0x03: // Complete List of 16-bit Service UUIDs
                result->service_uuid = data[0] | (data[1] << 8);
                break;

            default:
                break;
        }

        index += length + 1;  // Move to next field
    }
}

static void handle_general_access_profile_event(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;

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
            //advertising start complete event to indicate advertising start successfully or failed
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Advertising start failed");
            }
            ESP_LOGI(TAG, "Advertising start successfully");
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Advertising stop failed");
            } else {
                ESP_LOGI(TAG, "Stop adv successfully");
            }
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(TAG, "update connection params status = %d, conn_int = %d, latency = %d, timeout = %d",
                     param->update_conn_params.status,
                     param->update_conn_params.conn_int,
                     param->update_conn_params.latency,
                     param->update_conn_params.timeout);
            break;
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
            if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "scan stop failed, error status = %x", param->scan_stop_cmpl.status);
                break;
            }
            ESP_LOGI(TAG, "ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT, stop scan successfully");
            break;
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
            ESP_LOGI(TAG, "ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT, set scan sparameters complete");
            //the unit of the duration is second
            uint32_t duration = 120;
            esp_ble_gap_start_scanning(duration);
            break;
        }
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
                break;
            }
            ESP_LOGI(TAG, "ESP_GAP_BLE_SCAN_START_COMPLETE_EVT, scan start success");
            break;
        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
            struct ble_scan_result_evt_param scan = param->scan_rst;
            if (scan.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {  // Device found
                struct ble_scan_result_t result = {0};  // Initialize struct

                //memcpy(result.mac, scan.bda, 6); // Copy MAC address
                //result.rssi = scan.rssi;
                //result.adv_type = scan.adv_type;
                //result.phy_primary = scan.primary_phy;
                //result.phy_secondary = scan.secondary_phy;

                // Parse advertising data
                parse_advertising_data(scan.ble_adv, scan.adv_data_len, &result);

                adv_name = esp_ble_resolve_adv_data_by_type(scan_result->scan_rst.ble_adv,
                                                            scan_result->scan_rst.adv_data_len +
                                                            scan_result->scan_rst.scan_rsp_len,
                                                            ESP_BLE_AD_TYPE_NAME_CMPL,
                                                            &adv_name_len);

                /*
                if (adv_name != NULL) {
                    if (strlen(remote_device_name) == adv_name_len && strncmp((char *)adv_name, remote_device_name, adv_name_len) == 0) {
                        if (connect == false) {
                            connect = true;
                            ESP_LOGI(TAG, "connect to the remote device %s", remote_device_name);
                            esp_ble_gap_stop_scanning();

                            // Initiate GATT connection with the remote device,
                            // If ble physical connection is set up, ESP_GATTS_CONNECT_EVT and ESP_GATTC_CONNECT_EVT event will come
                            esp_ble_gatt_creat_conn_params_t creat_conn_params = {0};
                            memcpy(&creat_conn_params.remote_bda, scan_result->scan_rst.bda, ESP_BD_ADDR_LEN);
                            creat_conn_params.remote_addr_type = scan_result->scan_rst.ble_addr_type;
                            creat_conn_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
                            creat_conn_params.is_direct = true;
                            creat_conn_params.is_aux = false;
                            creat_conn_params.phy_mask = 0x0;
                            esp_ble_gattc_enh_open(client_profile.gattc_if,
                                                   &creat_conn_params);

                            // Update peer gatt server address
                            //memcpy(peer_gatts_addr, scan_result->scan_rst.bda, sizeof(esp_bd_addr_t));
                            //ESP_LOG_BUFFER_HEX("the remote device addr", peer_gatts_addr, sizeof(esp_bd_addr_t));
                        }
                    }
                }
                */
            }
            break;
        }
        default:
            break;
    }
}

static void connect_device(esp_bd_addr_t address) {
    esp_ble_conn_update_params_t conn_params = {0};
    memcpy(conn_params.bda, address, sizeof(esp_bd_addr_t));
    /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
    conn_params.latency = 0;
    conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
    conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
    conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms

    //connection_params.remote_bda.con

    //server_profile.conn_id = connection_params.conn_id;

    esp_ble_gap_update_conn_params(&conn_params);
}

static void server_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_interface, esp_ble_gatts_cb_param_t *param) {
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

            //param->write.conn_id
            _message_handler(message, FRONT);
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
            connect_device(param->connect.remote_bda);
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

static void gatts_server_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_interface, esp_ble_gatts_cb_param_t *param) {
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status != ESP_GATT_OK) {
            ESP_LOGI(TAG, "Reg app failed, app_id %04x, status %d",
                     param->reg.app_id, param->reg.status);
            return;
        }
        server_profile.gatts_interface = gatts_interface;
    }

    server_event_handler(event, gatts_interface, param);
}

static void client_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event) {
        case ESP_GATTC_REG_EVT:
            ESP_LOGI(GATTC_TAG, "GATT client register, status %d, app_id %d, gattc_if %d", param->reg.status, param->reg.app_id, gattc_if);
            esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
            if (scan_ret){
                ESP_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
            }
            break;
        case ESP_GATTC_CONNECT_EVT:{
            ESP_LOGI(GATTC_TAG, "Connected, conn_id %d, remote "ESP_BD_ADDR_STR"", p_data->connect.conn_id,
                     ESP_BD_ADDR_HEX(p_data->connect.remote_bda));
            client_profile.conn_id = p_data->connect.conn_id;
            memcpy(client_profile.remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
            esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->connect.conn_id);
            if (mtu_ret){
                ESP_LOGE(GATTC_TAG, "Config MTU error, error code = %x", mtu_ret);
            }
            break;
        }
        case ESP_GATTC_OPEN_EVT:
            if (param->open.status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "Open failed, status %d", p_data->open.status);
                break;
            }
            ESP_LOGI(GATTC_TAG, "Open successfully, MTU %u", p_data->open.mtu);
            break;
        case ESP_GATTC_DIS_SRVC_CMPL_EVT:
            if (param->dis_srvc_cmpl.status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "Service discover failed, status %d", param->dis_srvc_cmpl.status);
                break;
            }
            ESP_LOGI(GATTC_TAG, "Service discover complete, conn_id %d", param->dis_srvc_cmpl.conn_id);
            esp_ble_gattc_search_service(gattc_if, param->dis_srvc_cmpl.conn_id, &remote_filter_service_uuid);
            break;
        case ESP_GATTC_CFG_MTU_EVT:
            ESP_LOGI(GATTC_TAG, "MTU exchange, status %d, MTU %d", param->cfg_mtu.status, param->cfg_mtu.mtu);
            break;
        case ESP_GATTC_SEARCH_RES_EVT: {
            ESP_LOGI(GATTC_TAG, "Service search result, conn_id = %x, is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
            ESP_LOGI(GATTC_TAG, "start handle %d, end handle %d, current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
            if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID) {
                ESP_LOGI(GATTC_TAG, "Service found");
                get_server = true;
                client_profile.service_start_handle = p_data->search_res.start_handle;
                client_profile.service_end_handle = p_data->search_res.end_handle;
                ESP_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
            }
            break;
        }
        case ESP_GATTC_SEARCH_CMPL_EVT:
            if (p_data->search_cmpl.status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "Service search failed, status %x", p_data->search_cmpl.status);
                break;
            }
            if(p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_REMOTE_DEVICE) {
                ESP_LOGI(GATTC_TAG, "Get service information from remote device");
            } else if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_NVS_FLASH) {
                ESP_LOGI(GATTC_TAG, "Get service information from flash");
            } else {
                ESP_LOGI(GATTC_TAG, "Unknown service source");
            }
            ESP_LOGI(GATTC_TAG, "Service search complete");
            if (get_server){
                uint16_t count = 0;
                esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                         p_data->search_cmpl.conn_id,
                                                                         ESP_GATT_DB_CHARACTERISTIC,
                                                                         client_profile.service_start_handle,
                                                                         client_profile.service_end_handle,
                                                                         INVALID_HANDLE,
                                                                         &count);
                if (status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
                    break;
                }

                if (count > 0){
                    char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                    if (!char_elem_result){
                        ESP_LOGE(GATTC_TAG, "gattc no mem");
                        break;
                    }else{
                        status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                                 p_data->search_cmpl.conn_id,
                                                                 client_profile.service_start_handle,
                                                                 client_profile.service_end_handle,
                                                                 remote_filter_char_uuid,
                                                                 char_elem_result,
                                                                 &count);
                        if (status != ESP_GATT_OK){
                            ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error");
                            free(char_elem_result);
                            char_elem_result = NULL;
                            break;
                        }

                        /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
                        if (count > 0 && (char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){
                            client_profile.char_handle = char_elem_result[0].char_handle;
                            esp_ble_gattc_register_for_notify (gattc_if, client_profile.remote_bda, char_elem_result[0].char_handle);
                        }
                    }
                    /* free char_elem_result */
                    free(char_elem_result);
                }else{
                    ESP_LOGE(GATTC_TAG, "no char found");
                }
            }
            break;
        case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
            if (p_data->reg_for_notify.status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "Notification register failed, status %d", p_data->reg_for_notify.status);
            }else{
                ESP_LOGI(GATTC_TAG, "Notification register successfully");
                uint16_t count = 0;
                uint16_t notify_en = 1;
                esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                             client_profile.conn_id,
                                                                             ESP_GATT_DB_DESCRIPTOR,
                                                                             client_profile.service_start_handle,
                                                                             client_profile.service_end_handle,
                                                                             client_profile.char_handle,
                                                                             &count);
                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
                    break;
                }
                if (count > 0){
                    descr_elem_result = malloc(sizeof(esp_gattc_descr_elem_t) * count);
                    if (!descr_elem_result){
                        ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
                        break;
                    }else{
                        ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                             client_profile.conn_id,
                                                                             p_data->reg_for_notify.handle,
                                                                             notify_descr_uuid,
                                                                             descr_elem_result,
                                                                             &count);
                        if (ret_status != ESP_GATT_OK){
                            ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
                            free(descr_elem_result);
                            descr_elem_result = NULL;
                            break;
                        }
                        /* Every char has only one descriptor in our 'ESP_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
                        if (count > 0 && descr_elem_result[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
                            ret_status = esp_ble_gattc_write_char_descr( gattc_if,
                                                                         client_profile.conn_id,
                                                                         descr_elem_result[0].handle,
                                                                         sizeof(notify_en),
                                                                         (uint8_t *)&notify_en,
                                                                         ESP_GATT_WRITE_TYPE_RSP,
                                                                         ESP_GATT_AUTH_REQ_NONE);
                        }

                        if (ret_status != ESP_GATT_OK){
                            ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
                        }

                        /* free descr_elem_result */
                        free(descr_elem_result);
                    }
                }
                else{
                    ESP_LOGE(GATTC_TAG, "decsr not found");
                }

            }
            break;
        }
        case ESP_GATTC_NOTIFY_EVT:
            if (p_data->notify.is_notify){
                ESP_LOGI(GATTC_TAG, "Notification received");
            }else{
                ESP_LOGI(GATTC_TAG, "Indication received");
            }
            ESP_LOG_BUFFER_HEX(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
            break;
        case ESP_GATTC_WRITE_DESCR_EVT:
            if (p_data->write.status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "Descriptor write failed, status %x", p_data->write.status);
                break;
            }
            ESP_LOGI(GATTC_TAG, "Descriptor write successfully");
            uint8_t write_char_data[35];
            for (int i = 0; i < sizeof(write_char_data); ++i)
            {
                write_char_data[i] = i % 256;
            }
            esp_ble_gattc_write_char( gattc_if,
                                      client_profile.conn_id,
                                      client_profile.char_handle,
                                      sizeof(write_char_data),
                                      write_char_data,
                                      ESP_GATT_WRITE_TYPE_RSP,
                                      ESP_GATT_AUTH_REQ_NONE);
            break;
        case ESP_GATTC_SRVC_CHG_EVT: {
            esp_bd_addr_t bda;
            memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
            ESP_LOGI(GATTC_TAG, "Service change from "ESP_BD_ADDR_STR"", ESP_BD_ADDR_HEX(bda));
            break;
        }
        case ESP_GATTC_WRITE_CHAR_EVT:
            if (p_data->write.status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "Characteristic write failed, status %x)", p_data->write.status);
                break;
            }
            ESP_LOGI(GATTC_TAG, "Characteristic write successfully");
            break;
        case ESP_GATTC_DISCONNECT_EVT:
            connect = false;
            get_server = false;
            ESP_LOGI(GATTC_TAG, "Disconnected, remote "ESP_BD_ADDR_STR", reason 0x%02x",
                     ESP_BD_ADDR_HEX(p_data->disconnect.remote_bda), p_data->disconnect.reason);
            break;
        default:
            break;
    }
}

static void gattc_client_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            client_profile.gattc_if = gattc_if;
        } else {
            ESP_LOGI(GATTC_TAG, "reg app failed, app_id %04x, status %d",
                     param->reg.app_id,
                     param->reg.status);
            return;
        }
    }

    client_event_handler(event, gattc_if, param);
}

void ble_main(char device_name[ESP_BLE_ADV_NAME_LEN_MAX], message_handler_t message_handler, scan_handler_t scan_handler) {
    memcpy(_device_name, device_name, ESP_BLE_ADV_NAME_LEN_MAX);
    _message_handler = message_handler;
    _scan_handler = scan_handler;
    
    esp_err_t ret;

    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    if (ret) {
        ESP_LOGE(TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_ble_gap_register_callback(handle_general_access_profile_event);
    if (ret) {
        ESP_LOGE(TAG, "gap register error, error code = %x", ret);
        return;
    }

    // Note: Avoid performing time-consuming operations within callback functions.
    ret = esp_ble_gatts_register_callback(gatts_server_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "gatts register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gatts_app_register(0);
    if (ret) {
        ESP_LOGE(TAG, "gatts app register error, error code = %x", ret);
        return;
    }

    // Register the callback function to the gattc module
    ret = esp_ble_gattc_register_callback(gattc_client_event_handler);
    if(ret){
        ESP_LOGE(GATTC_TAG, "%s gattc register failed, error code = %x", __func__, ret);
        return;
    }

    ret = esp_ble_gattc_app_register(1);
    if (ret){
        ESP_LOGE(GATTC_TAG, "%s gattc app register failed, error code = %x", __func__, ret);
    }

    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret) {
        ESP_LOGE(TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }
}
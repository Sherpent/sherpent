//
// Created by gabriel on 2/19/25.
//

#include "ble.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <esp_gap_ble_api.h>
#include "nvs.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#define GATTC_TAG "GATTC_DEMO"
#define REMOTE_SERVICE_UUID        0x00FF
#define REMOTE_NOTIFY_UUID    0xFF01
#define APP_ID 0
#define INVALID_HANDLE   0

static char remote_device_name[ESP_BLE_ADV_NAME_LEN_MAX] = "ESP_GATTS_DEMO";
static bool connect    = false;
static bool get_service = false;
static esp_gattc_char_elem_t *char_elem_result   = NULL;
static esp_gattc_descr_elem_t *descr_elem_result = NULL;

/* Declare static functions */
static void scan_init();
static void scan_result_cb(esp_ble_gap_cb_param_t *scan_result);
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

// ================================ CALLBACKS ================================
static conn_callback_t conn_callback_table[] = {
        [CONNECTION] = NULL,
        [DISCONNECTION] = NULL,
};

static scan_callback_t scan_callback_table[] = {
        [START] = NULL,
        [STOP] = NULL,
};

static message_callback_t message_callback = NULL;

static event_callback_t setup_complete_callback = NULL;

// ================================== QUEUE ==================================
// Queue to hold incoming messages
QueueHandle_t message_queue;

static esp_bt_uuid_t remote_filter_service_uuid = {
        .len = ESP_UUID_LEN_16,
        .uuid = {.uuid16 = REMOTE_SERVICE_UUID,},
};

static esp_bt_uuid_t remote_filter_char_uuid = {
        .len = ESP_UUID_LEN_16,
        .uuid = {.uuid16 = REMOTE_NOTIFY_UUID,},
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
    esp_gattc_cb_t gattc_cb;
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t notify_char_handle;
    esp_bd_addr_t remote_bda;
};

/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT */
static struct gattc_profile_inst profile = {
        .gattc_cb = gattc_profile_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
};

static const char *esp_key_type_to_str(esp_ble_key_type_t key_type)
{
    const char *key_str = NULL;
    switch(key_type) {
        case ESP_LE_KEY_NONE:
            key_str = "ESP_LE_KEY_NONE";
            break;
        case ESP_LE_KEY_PENC:
            key_str = "ESP_LE_KEY_PENC";
            break;
        case ESP_LE_KEY_PID:
            key_str = "ESP_LE_KEY_PID";
            break;
        case ESP_LE_KEY_PCSRK:
            key_str = "ESP_LE_KEY_PCSRK";
            break;
        case ESP_LE_KEY_PLK:
            key_str = "ESP_LE_KEY_PLK";
            break;
        case ESP_LE_KEY_LLK:
            key_str = "ESP_LE_KEY_LLK";
            break;
        case ESP_LE_KEY_LENC:
            key_str = "ESP_LE_KEY_LENC";
            break;
        case ESP_LE_KEY_LID:
            key_str = "ESP_LE_KEY_LID";
            break;
        case ESP_LE_KEY_LCSRK:
            key_str = "ESP_LE_KEY_LCSRK";
            break;
        default:
            key_str = "INVALID BLE KEY TYPE";
            break;

    }
    return key_str;
}

static char *esp_auth_req_to_str(esp_ble_auth_req_t auth_req)
{
    char *auth_str = NULL;
    switch(auth_req) {
        case ESP_LE_AUTH_NO_BOND:
            auth_str = "ESP_LE_AUTH_NO_BOND";
            break;
        case ESP_LE_AUTH_BOND:
            auth_str = "ESP_LE_AUTH_BOND";
            break;
        case ESP_LE_AUTH_REQ_MITM:
            auth_str = "ESP_LE_AUTH_REQ_MITM";
            break;
        case ESP_LE_AUTH_REQ_BOND_MITM:
            auth_str = "ESP_LE_AUTH_REQ_BOND_MITM";
            break;
        case ESP_LE_AUTH_REQ_SC_ONLY:
            auth_str = "ESP_LE_AUTH_REQ_SC_ONLY";
            break;
        case ESP_LE_AUTH_REQ_SC_BOND:
            auth_str = "ESP_LE_AUTH_REQ_SC_BOND";
            break;
        case ESP_LE_AUTH_REQ_SC_MITM:
            auth_str = "ESP_LE_AUTH_REQ_SC_MITM";
            break;
        case ESP_LE_AUTH_REQ_SC_MITM_BOND:
            auth_str = "ESP_LE_AUTH_REQ_SC_MITM_BOND";
            break;
        default:
            auth_str = "INVALID BLE AUTH REQ";
            break;
    }

    return auth_str;
}

static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event) {
        case ESP_GATTC_REG_EVT:
            ESP_LOGI(GATTC_TAG, "GATT client register, status %u, app_id %u, gattc_if %d", p_data->reg.status, p_data->reg.app_id, gattc_if);
            esp_ble_gap_config_local_privacy(true);
            break;
        case ESP_GATTC_CONNECT_EVT:
            ESP_LOGI(GATTC_TAG, "Connected, conn_id %d, remote "ESP_BD_ADDR_STR"", p_data->connect.conn_id,
                     ESP_BD_ADDR_HEX(p_data->connect.remote_bda));
            if (conn_callback_table[CONNECTION] != NULL) conn_callback_table[CONNECTION](p_data->connect.conn_id);
            break;
        case ESP_GATTC_OPEN_EVT:
            if (param->open.status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "Open failed, status %x", p_data->open.status);
                break;
            }
            ESP_LOGI(GATTC_TAG, "Open successfully, MTU %d", p_data->open.mtu);
            profile.conn_id = p_data->open.conn_id;
            memcpy(profile.remote_bda, p_data->open.remote_bda, sizeof(esp_bd_addr_t));
            esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
            if (mtu_ret){
                ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
            }
            break;
        case ESP_GATTC_CFG_MTU_EVT:
            ESP_LOGI(GATTC_TAG, "MTU exchange, status %d, MTU %d", param->cfg_mtu.status, param->cfg_mtu.mtu);
            esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
            break;
        case ESP_GATTC_SEARCH_RES_EVT: {
            ESP_LOGI(GATTC_TAG, "Service search result, conn_id %x, is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
            ESP_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
            if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID) {
                ESP_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
                get_service = true;
                profile.service_start_handle = p_data->search_res.start_handle;
                profile.service_end_handle = p_data->search_res.end_handle;
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
                ESP_LOGI(GATTC_TAG, "unknown service source");
            }
            ESP_LOGI(GATTC_TAG, "Service search complete");
            if (get_service){
                uint16_t count  = 0;
                uint16_t offset = 0;
                esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count(gattc_if,
                                                                            profile.conn_id,
                                                                            ESP_GATT_DB_CHARACTERISTIC,
                                                                            profile.service_start_handle,
                                                                            profile.service_end_handle,
                                                                            INVALID_HANDLE,
                                                                            &count);
                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error, %d", __LINE__);
                    break;
                }
                if (count > 0){
                    char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                    if (!char_elem_result){
                        ESP_LOGE(GATTC_TAG, "gattc no mem");
                        break;
                    }else{
                        ret_status = esp_ble_gattc_get_all_char(gattc_if,
                                                                profile.conn_id,
                                                                profile.service_start_handle,
                                                                profile.service_end_handle,
                                                                char_elem_result,
                                                                &count,
                                                                offset);
                        if (ret_status != ESP_GATT_OK){
                            ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_all_char error, %d", __LINE__);
                            free(char_elem_result);
                            char_elem_result = NULL;
                            break;
                        }
                        if (count > 0){

                            for (int i = 0; i < count; ++i)
                            {
                                if (char_elem_result[i].uuid.len == ESP_UUID_LEN_16 && char_elem_result[i].uuid.uuid.uuid16 == REMOTE_NOTIFY_UUID && (char_elem_result[i].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY))
                                {
                                    profile.notify_char_handle = char_elem_result[i].char_handle;
                                    esp_ble_gattc_register_for_notify (gattc_if,
                                                                       profile.remote_bda,
                                                                       char_elem_result[i].char_handle);
                                    break;
                                }
                            }
                        }
                    }
                    free(char_elem_result);
                    char_elem_result = NULL;
                }
            }

            break;
        case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
            if (p_data->reg_for_notify.status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "Notification register failed, status %x", p_data->reg_for_notify.status);
                break;
            } else {
                ESP_LOGI(GATTC_TAG, "Notification register successfully");
            }
            break;
        }
        case ESP_GATTC_NOTIFY_EVT:
            ESP_LOGI(GATTC_TAG, "Notification received, value ");
            ESP_LOG_BUFFER_HEX(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);

            // Allocate memory for message
            struct Message *message = (struct Message *) malloc(param->notify.value[0]);
            if (message == NULL) {
                ESP_LOGE(GATTC_TAG, "Failed to allocate memory for message");
                return;
            }
            memcpy(message, param->notify.value, param->notify.value[0]);

            // Prepare queue item
            message_queue_item_t queue_item = {
                    .conn_id = param->write.conn_id,
                    .message = message
            };

            // Try to send the message to the queue
            if (message_queue != NULL) {
                if (xQueueSendToBack(message_queue, ( void * ) &queue_item, 0) != pdPASS) {
                    ESP_LOGE(GATTC_TAG, "Message queue full, dropping message");
                    free(message);  // Free memory if queue is full
                }
            }
            break;
        case ESP_GATTC_WRITE_DESCR_EVT:
            if (p_data->write.status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "Descriptor write failed, status %x", p_data->write.status);
                break;
            }
            ESP_LOGI(GATTC_TAG, "Descriptor write successfully");
            break;
        case ESP_GATTC_SRVC_CHG_EVT: {
            esp_bd_addr_t bda;
            memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
            ESP_LOGI(GATTC_TAG, "Service change from "ESP_BD_ADDR_STR"", ESP_BD_ADDR_HEX(bda));
            break;
        }
        case ESP_GATTC_WRITE_CHAR_EVT:
            if (p_data->write.status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "Characteristic write failed, status %x", p_data->write.status);
                break;
            }
            ESP_LOGI(GATTC_TAG, "Characteristic write successfully");
            break;
        case ESP_GATTC_DISCONNECT_EVT:
            ESP_LOGI(GATTC_TAG, "Disconnected, remote "ESP_BD_ADDR_STR", reason 0x%02x",
                     ESP_BD_ADDR_HEX(p_data->disconnect.remote_bda), p_data->disconnect.reason);
            connect = false;
            get_service = false;
            if (conn_callback_table[DISCONNECTION] != NULL) conn_callback_table[DISCONNECTION](p_data->disconnect.conn_id);
            break;
        default:
            break;
    }
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;
    switch (event) {
        case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT:
            if (param->local_privacy_cmpl.status != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(GATTC_TAG, "Local privacy config failed, status %x", param->local_privacy_cmpl.status);
                break;
            }
            break;
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
            if (setup_complete_callback != NULL) setup_complete_callback();
            break;
        }
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            //scan start complete event to indicate scan start successfully or failed
            if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTC_TAG, "Scanning start failed, status %x", param->scan_start_cmpl.status);
                break;
            }
            ESP_LOGI(GATTC_TAG, "Scanning start successfully");
            if (scan_callback_table[START] != NULL) scan_callback_table[START]();
            break;
        case ESP_GAP_BLE_PASSKEY_REQ_EVT:                           /* passkey request event */
            /* Call the following function to input the passkey which is displayed on the remote device */
            //esp_ble_passkey_reply(profile.remote_bda, true, 0x00);
            ESP_LOGI(GATTC_TAG, "Passkey request");
            break;
        case ESP_GAP_BLE_OOB_REQ_EVT: {
            ESP_LOGI(GATTC_TAG, "OOB request");
            uint8_t tk[16] = {1}; //If you paired with OOB, both devices need to use the same tk
            esp_ble_oob_req_reply(param->ble_security.ble_req.bd_addr, tk, sizeof(tk));
            break;
        }
        case ESP_GAP_BLE_LOCAL_IR_EVT:                               /* BLE local IR event */
            ESP_LOGI(GATTC_TAG, "Local identity root");
            break;
        case ESP_GAP_BLE_LOCAL_ER_EVT:                               /* BLE local ER event */
            ESP_LOGI(GATTC_TAG, "Local encryption root");
            break;
        case ESP_GAP_BLE_SEC_REQ_EVT:
            /* send the positive(true) security response to the peer device to accept the security request.
            If not accept the security request, should send the security response with negative(false) accept value*/
            esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
            break;
        case ESP_GAP_BLE_NC_REQ_EVT:
            /* The app will receive this evt when the IO has DisplayYesNO capability and the peer device IO also has DisplayYesNo capability.
            show the passkey number to the user to confirm it with the number displayed by peer device. */
            esp_ble_confirm_reply(param->ble_security.ble_req.bd_addr, true);
            ESP_LOGI(GATTC_TAG, "Numeric Comparison request, passkey %" PRIu32, param->ble_security.key_notif.passkey);
            break;
        case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:  ///the app will receive this evt when the IO  has Output capability and the peer device IO has Input capability.
            ///show the passkey number to the user to input it in the peer device.
            ESP_LOGI(GATTC_TAG, "Passkey notify, passkey %06" PRIu32, param->ble_security.key_notif.passkey);
            break;
        case ESP_GAP_BLE_KEY_EVT:
            //shows the ble key info share with peer device to the user.
            ESP_LOGI(GATTC_TAG, "Key exchanged, key_type %s", esp_key_type_to_str(param->ble_security.ble_key.key_type));
            break;
        case ESP_GAP_BLE_AUTH_CMPL_EVT: {
            esp_bd_addr_t bd_addr;
            memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
            ESP_LOGI(GATTC_TAG, "Authentication complete, addr_type %d, addr "ESP_BD_ADDR_STR"",
                     param->ble_security.auth_cmpl.addr_type, ESP_BD_ADDR_HEX(bd_addr));
            if (!param->ble_security.auth_cmpl.success) {
                ESP_LOGI(GATTC_TAG, "Pairing failed, reason 0x%x",param->ble_security.auth_cmpl.fail_reason);
            } else {
                ESP_LOGI(GATTC_TAG, "Pairing successfully, auth mode %s", esp_auth_req_to_str(param->ble_security.auth_cmpl.auth_mode));
            }
            break;
        }
        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
            switch (scan_result->scan_rst.search_evt) {
                case ESP_GAP_SEARCH_INQ_RES_EVT:
                    adv_name = esp_ble_resolve_adv_data_by_type(scan_result->scan_rst.ble_adv,
                                                                scan_result->scan_rst.adv_data_len + scan_result->scan_rst.scan_rsp_len,
                                                                ESP_BLE_AD_TYPE_NAME_CMPL,
                                                                &adv_name_len);
                    ESP_LOGI(GATTC_TAG, "Scan result, device "ESP_BD_ADDR_STR", name len %u", ESP_BD_ADDR_HEX(scan_result->scan_rst.bda), adv_name_len);
                    ESP_LOG_BUFFER_CHAR(GATTC_TAG, adv_name, adv_name_len);
                    if (adv_name != NULL) {
                        if (strlen(remote_device_name) == adv_name_len && strncmp((char *)adv_name, remote_device_name, adv_name_len) == 0) {
                            // Note: If there are multiple devices with the same device name, the device may connect to an unintended one.
                            // It is recommended to change the default device name to ensure it is unique.
                            ESP_LOGI(GATTC_TAG, "Device found %s", remote_device_name);
                            if (connect == false) {
                                connect = true;
                                ESP_LOGI(GATTC_TAG, "Connect to the remote device");
                                esp_ble_gap_stop_scanning();
                                esp_ble_gatt_creat_conn_params_t creat_conn_params = {0};
                                memcpy(&creat_conn_params.remote_bda, scan_result->scan_rst.bda, ESP_BD_ADDR_LEN);
                                creat_conn_params.remote_addr_type = scan_result->scan_rst.ble_addr_type;
                                creat_conn_params.own_addr_type = BLE_ADDR_TYPE_RPA_PUBLIC;
                                creat_conn_params.is_direct = true;
                                creat_conn_params.is_aux = false;
                                creat_conn_params.phy_mask = 0x0;
                                esp_ble_gattc_enh_open(profile.gattc_if,
                                                       &creat_conn_params);
                            }
                        }
                    }
                    break;
                case ESP_GAP_SEARCH_INQ_CMPL_EVT:
                    break;
                default:
                    break;
            }
            break;
        }

        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
            if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(GATTC_TAG, "Scanning stop failed, status %x", param->scan_stop_cmpl.status);
                break;
            }
            ESP_LOGI(GATTC_TAG, "Scanning stop successfully");
            if (scan_callback_table[STOP] != NULL) scan_callback_table[STOP]();
            break;

        default:
            break;
    }
}

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            profile.gattc_if = gattc_if;
        } else {
            ESP_LOGI(GATTC_TAG, "reg app failed, app_id %04x, status %d",
                     param->reg.app_id,
                     param->reg.status);
            return;
        }
    }

    /* If the gattc_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
        gattc_if == profile.gattc_if) {
        if (profile.gattc_cb) {
            profile.gattc_cb(event, gattc_if, param);
        }
    }
}

static void scan_init() {
    uint32_t duration = 30;
    esp_ble_gap_start_scanning(duration);
}

static void scan_result_cb(esp_ble_gap_cb_param_t *scan_result) {
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;

    if (scan_result->scan_rst.search_evt != ESP_GAP_SEARCH_INQ_RES_EVT) return;

    adv_name = esp_ble_resolve_adv_data_by_type(scan_result->scan_rst.ble_adv,
                                                scan_result->scan_rst.adv_data_len + scan_result->scan_rst.scan_rsp_len,
                                                ESP_BLE_AD_TYPE_NAME_CMPL,
                                                &adv_name_len);

    ESP_LOGI(GATTC_TAG, "Scan result, device "ESP_BD_ADDR_STR", name len %u", ESP_BD_ADDR_HEX(scan_result->scan_rst.bda), adv_name_len);
    ESP_LOG_BUFFER_CHAR(GATTC_TAG, adv_name, adv_name_len);

    if (adv_name == NULL) return;
    if (strlen(remote_device_name) == adv_name_len && strncmp((char *)adv_name, remote_device_name, adv_name_len) == 0) {
        // Note: If there are multiple devices with the same device name, the device may connect to an unintended one.
        // It is recommended to change the default device name to ensure it is unique.
        ESP_LOGI(GATTC_TAG, "Device found %s", remote_device_name);
        if (connect == false) {
            connect = true;
            ESP_LOGI(GATTC_TAG, "Connect to the remote device");
            esp_ble_gap_stop_scanning();
            esp_ble_gatt_creat_conn_params_t creat_conn_params = {0};
            memcpy(&creat_conn_params.remote_bda, scan_result->scan_rst.bda, ESP_BD_ADDR_LEN);
            creat_conn_params.remote_addr_type = scan_result->scan_rst.ble_addr_type;
            creat_conn_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
            creat_conn_params.is_direct = true;
            creat_conn_params.is_aux = false;
            creat_conn_params.phy_mask = 0x0;
            esp_ble_gattc_enh_open(profile.gattc_if,
                                   &creat_conn_params);
        }
    }
}

void message_task(void *pvParameters) {
    message_queue_item_t queueItem;

    for (;;) {
        if (xQueueReceive(message_queue, &queueItem, 10) == pdPASS) {
            if (message_callback != NULL) {
                message_callback(queueItem.conn_id, queueItem.message);
            }
            free(queueItem.message);
        }
    }
}

static void init_nvs() {
    // Initialize NVS.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
}

void init_ble()
{
    init_nvs();

    message_queue = xQueueCreate(50, sizeof(message_queue_item_t));
    if (message_queue == NULL) {
        ESP_LOGE(GATTC_TAG, "Failed to create message queue");
    } else {
        xTaskCreate(message_task, "MessageTask", 2048, NULL, 1, NULL);
    }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);

    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    esp_bluedroid_init();
    esp_bluedroid_enable();
    esp_ble_gap_register_callback(esp_gap_cb);

    //register the callback function to the gattc module
    esp_ble_gattc_register_callback(esp_gattc_cb);
    esp_ble_gattc_app_register(APP_ID);
    esp_ble_gatt_set_local_mtu(500);
}

bool is_connected() {
    return connect;
}

bool send_message(struct Message *message) {
    uint8_t notify_data[message->msg_size];
    memcpy(notify_data, message, message->msg_size);

    return esp_ble_gattc_write_char(profile.gattc_if, profile.conn_id, profile.notify_char_handle, sizeof(notify_data), notify_data, ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE) == ESP_OK;
}

bool start_scan(uint32_t duration) {
    return esp_ble_gap_start_scanning(duration) == ESP_OK;
}

void register_conn_callback(enum conn_callback_type type, conn_callback_t callback) { conn_callback_table[type] = callback; }
void register_scan_callback(enum scan_callback_type type, scan_callback_t callback) { scan_callback_table[type] = callback; }
void register_msg_callback(message_callback_t callback) { message_callback = callback; }
void register_setup_complete_callback(event_callback_t callback) { setup_complete_callback = callback; }


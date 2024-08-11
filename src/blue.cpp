#include "blue.h"

#include "comx.h"

#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>

#define TAG "bluetooth"
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <esp_log.h>

uint32_t Blue::client = 0;
string Blue::buffer = "";

void Blue::setup() {

    char bda_str[18] = {0};
    esp_err_t ret;
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    bluedroid_cfg.ssp_en = false;

    if ((ret = esp_bluedroid_init_with_cfg(&bluedroid_cfg)) != ESP_OK) {
        ESP_LOGE(TAG, "%s initialize bluedroid failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(TAG, "%s enable bluedroid failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_gap_register_callback(security_callback)) != ESP_OK) {
        ESP_LOGE(TAG, "%s gap register failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_spp_register_callback(callback)) != ESP_OK) {
        ESP_LOGE(TAG, "%s spp register failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    esp_spp_cfg_t bt_spp_cfg = {
        .mode = ESP_SPP_MODE_CB,
        .enable_l2cap_ertm = true,
        .tx_buffer_size = 0, /* Only used for ESP_SPP_MODE_VFS mode */
    };

    if ((ret = esp_spp_enhanced_init(&bt_spp_cfg)) != ESP_OK) {
        ESP_LOGE(TAG, "%s spp init failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    /*
     * Set default parameters for Legacy Pairing
     * Use variable pin, input pin code when pairing
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);

    ESP_LOGI(TAG, "Own address:[%s]", bda2str((uint8_t *)esp_bt_dev_get_address(), bda_str, sizeof(bda_str)));
}

void Blue::parse_buffer() {
    size_t first_newline = buffer.find_first_of('\n');

    while (first_newline != -1) {
        string line = buffer.substr(0, first_newline + 1);
        buffer.erase(0, first_newline + 1);        

        string response = Comx::interpret(line);

        if (!response.empty() && client != 0) {
            esp_spp_write(client, response.length(), (uint8_t*) response.c_str());
        }

        first_newline = buffer.find_first_of('\n');
    }
}

void Blue::callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    char bda_str[18] = {0};
    uint8_t* input;
    uint16_t len;

    switch (event) {
        case ESP_SPP_INIT_EVT:
            if (param->init.status == ESP_SPP_SUCCESS) {
                ESP_LOGI(TAG, "ESP_SPP_INIT_EVT");
                esp_spp_start_srv(ESP_SPP_SEC_AUTHENTICATE, ESP_SPP_ROLE_SLAVE, 0, SPP_SERVER_NAME);
            } else {
                ESP_LOGE(TAG, "ESP_SPP_INIT_EVT status:%d", param->init.status);
            }
            break;
        case ESP_SPP_DISCOVERY_COMP_EVT:
            ESP_LOGI(TAG, "ESP_SPP_DISCOVERY_COMP_EVT");
            break;
        case ESP_SPP_OPEN_EVT:
            ESP_LOGI(TAG, "ESP_SPP_OPEN_EVT");
            break;
        case ESP_SPP_CLOSE_EVT:
            ESP_LOGI(TAG, "ESP_SPP_CLOSE_EVT status:%d handle:%" PRIu32 " close_by_remote:%d", param->close.status,
                     param->close.handle, param->close.async);
            client = 0;
            break;
        case ESP_SPP_START_EVT:
            if (param->start.status == ESP_SPP_SUCCESS) {
                ESP_LOGI(TAG, "ESP_SPP_START_EVT handle:%" PRIu32 " sec_id:%d scn:%d", param->start.handle, param->start.sec_id,
                         param->start.scn);
                esp_bt_dev_set_device_name(BT_DEVICE_NAME);
                esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            } else {
                ESP_LOGE(TAG, "ESP_SPP_START_EVT status:%d", param->start.status);
            }
            break;
        case ESP_SPP_CL_INIT_EVT:
            ESP_LOGI(TAG, "ESP_SPP_CL_INIT_EVT");
            break;
        case ESP_SPP_DATA_IND_EVT:
    
            ESP_LOGI(TAG, "ESP_SPP_DATA_IND_EVT len:%d handle:%" PRIu32,
                     param->data_ind.len, param->data_ind.handle);
            if (param->data_ind.len < 128) {
                esp_log_buffer_hex("", param->data_ind.data, param->data_ind.len);
            }

            input = param->data_ind.data;
            len = param->data_ind.len;

            for (int i = 0; i < len; i++) {
                buffer += (char) *input;
                input++;
            }

            parse_buffer();

            break;
        case ESP_SPP_CONG_EVT:
            ESP_LOGI(TAG, "ESP_SPP_CONG_EVT");
            break;
        case ESP_SPP_WRITE_EVT:
            ESP_LOGI(TAG, "ESP_SPP_WRITE_EVT");
            break;
        case ESP_SPP_SRV_OPEN_EVT:
            ESP_LOGI(TAG, "ESP_SPP_SRV_OPEN_EVT status:%d handle:%" PRIu32 ", rem_bda:[%s]", param->srv_open.status,
                     param->srv_open.handle, bda2str(param->srv_open.rem_bda, bda_str, sizeof(bda_str)));
            client = param->open.handle;
            break;
        case ESP_SPP_SRV_STOP_EVT:
            ESP_LOGI(TAG, "ESP_SPP_SRV_STOP_EVT");
            break;
        case ESP_SPP_UNINIT_EVT:
            ESP_LOGI(TAG, "ESP_SPP_UNINIT_EVT");
            break;
        default:
            break;
    }
}

void Blue::security_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    char bda_str[18] = {0};

    switch (event) {
        case ESP_BT_GAP_AUTH_CMPL_EVT:{
            if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "authentication success: %s bda:[%s]", param->auth_cmpl.device_name,
                         bda2str(param->auth_cmpl.bda, bda_str, sizeof(bda_str)));
            } else {
                ESP_LOGE(TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
            }
            break;
        }
        case ESP_BT_GAP_PIN_REQ_EVT:{
            ESP_LOGI(TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
            if (param->pin_req.min_16_digit) {
                ESP_LOGI(TAG, "Input pin code: 0000 0000 0000 0000");
                esp_bt_pin_code_t pin_code = {0};
                esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
            } else {
                ESP_LOGI(TAG, "Input pin code: 1234");
                esp_bt_pin_code_t pin_code;
                pin_code[0] = '1';
                pin_code[1] = '2';
                pin_code[2] = '3';
                pin_code[3] = '4';
                esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
            }
            break;
        }

        case ESP_BT_GAP_MODE_CHG_EVT:
            ESP_LOGI(TAG, "ESP_BT_GAP_MODE_CHG_EVT mode:%d bda:[%s]", param->mode_chg.mode,
                     bda2str(param->mode_chg.bda, bda_str, sizeof(bda_str)));
            break;

        default: {
            ESP_LOGI(TAG, "event: %d", event);
            break;
        }
    }
    return;
}

char* Blue::bda2str(uint8_t * bda, char *str, size_t size) {
    if (bda == NULL || str == NULL || size < 18) {
        return NULL;
    }

    uint8_t *p = bda;
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5]);
    return str;
}

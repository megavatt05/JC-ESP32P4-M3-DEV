/*
 * JC1060 / JC-ESP32P4 Zigbee Coordinator + LVGL HMI
 * API: esp-zigbee-lib 2.x (IDF 6.0.3)
 */
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "nvs_flash.h"

#include "esp_lvgl_port.h"
#include "lvgl.h"

#include "esp_zigbee.h"
#include "ezbee/zha.h"

static const char *TAG = "JC1060_ZB_GW";

#define ZB_COORDINATOR_ENDPOINT 1
#define RELAY_EP_L1             1
#define RELAY_EP_L2             2

static uint16_t s_bound_relay_short_addr = 0xFFFF;
static lv_obj_t *s_sw_l1 = NULL;
static lv_obj_t *s_sw_l2 = NULL;
static lv_obj_t *s_lbl_status = NULL;

static esp_err_t zb_send_relay_cmd(uint16_t short_addr, uint8_t endpoint, bool state)
{
    if (short_addr == 0xFFFF) {
        ESP_LOGW(TAG, "Устройство не привязано! Команда проигнорирована.");
        return ESP_ERR_INVALID_STATE;
    }

    ezb_zcl_on_off_cmd_t cmd_req = {
        .cmd_ctrl =
            {
                .dst_addr =
                    {
                        .addr_mode    = EZB_ADDR_MODE_SHORT,
                        .u.short_addr = short_addr,
                    },
                .src_ep = ZB_COORDINATOR_ENDPOINT,
                .dst_ep = endpoint,
            },
    };

    ESP_LOGI(TAG, "ZCL: канал %d -> %s (0x%04X)", endpoint, state ? "ВКЛ" : "ВЫКЛ", short_addr);

    esp_zigbee_lock_acquire(portMAX_DELAY);
    ezb_err_t zret = state ? ezb_zcl_on_off_on_cmd_req(&cmd_req) : ezb_zcl_on_off_off_cmd_req(&cmd_req);
    esp_zigbee_lock_release();

    return (zret == EZB_ERR_NONE) ? ESP_OK : ESP_FAIL;
}

static void event_sw_l1_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool state = lv_obj_has_state(sw, LV_STATE_CHECKED);
    zb_send_relay_cmd(s_bound_relay_short_addr, RELAY_EP_L1, state);
}

static void event_sw_l2_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool state = lv_obj_has_state(sw, LV_STATE_CHECKED);
    zb_send_relay_cmd(s_bound_relay_short_addr, RELAY_EP_L2, state);
}

static void event_btn_pair_cb(lv_event_t *e)
{
    (void)e;
    ESP_LOGI(TAG, "Открытие сети Zigbee на 180 с...");
    esp_zigbee_lock_acquire(portMAX_DELAY);
    ezb_bdb_open_network(180);
    esp_zigbee_lock_release();
    if (s_lbl_status) {
        lv_label_set_text(s_lbl_status, "Статус: Поиск устройств (180с)...");
    }
}

static void create_hmi_gui(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x181B20), 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Шлюз Zigbee: JC1060-P470C");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t *card = lv_obj_create(scr);
    lv_obj_set_size(card, 600, 260);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x23272F), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x3B4252), 0);
    lv_obj_set_style_radius(card, 16, 0);

    lv_obj_t *lbl_l1 = lv_label_create(card);
    lv_label_set_text(lbl_l1, "Канал 1 (L1 - Люстра):");
    lv_obj_set_style_text_font(lbl_l1, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_l1, lv_color_hex(0xE5E9F0), 0);
    lv_obj_align(lbl_l1, LV_ALIGN_TOP_LEFT, 30, 35);

    s_sw_l1 = lv_switch_create(card);
    lv_obj_set_size(s_sw_l1, 70, 36);
    lv_obj_align(s_sw_l1, LV_ALIGN_TOP_RIGHT, -30, 30);
    lv_obj_add_event_cb(s_sw_l1, event_sw_l1_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *lbl_l2 = lv_label_create(card);
    lv_label_set_text(lbl_l2, "Канал 2 (L2 - Бра):");
    lv_obj_set_style_text_font(lbl_l2, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_l2, lv_color_hex(0xE5E9F0), 0);
    lv_obj_align(lbl_l2, LV_ALIGN_TOP_LEFT, 30, 115);

    s_sw_l2 = lv_switch_create(card);
    lv_obj_set_size(s_sw_l2, 70, 36);
    lv_obj_align(s_sw_l2, LV_ALIGN_TOP_RIGHT, -30, 110);
    lv_obj_add_event_cb(s_sw_l2, event_sw_l2_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *btn_pair = lv_button_create(scr);
    lv_obj_set_size(btn_pair, 280, 50);
    lv_obj_align(btn_pair, LV_ALIGN_BOTTOM_LEFT, 100, -50);
    lv_obj_set_style_bg_color(btn_pair, lv_color_hex(0x2E7D32), 0);
    lv_obj_add_event_cb(btn_pair, event_btn_pair_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_btn = lv_label_create(btn_pair);
    lv_label_set_text(lbl_btn, "Сопряжение (180с)");
    lv_obj_center(lbl_btn);

    s_lbl_status = lv_label_create(scr);
    lv_label_set_text(s_lbl_status, "Статус: ожидание сети Zigbee...");
    lv_obj_set_style_text_font(s_lbl_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_status, lv_color_hex(0x88C0D0), 0);
    lv_obj_align(s_lbl_status, LV_ALIGN_BOTTOM_RIGHT, -100, -65);
}

static bool esp_zigbee_app_signal_handler(const ezb_app_signal_t *app_signal)
{
    ezb_app_signal_type_t signal_type = ezb_app_signal_get_type(app_signal);

    switch (signal_type) {
    case EZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Инициализация Zigbee координатора...");
        ezb_bdb_start_top_level_commissioning(EZB_BDB_MODE_INITIALIZATION);
        break;

    case EZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case EZB_BDB_SIGNAL_DEVICE_REBOOT: {
        ezb_bdb_comm_status_t status = *((ezb_bdb_comm_status_t *)ezb_app_signal_get_params(app_signal));
        if (status == EZB_BDB_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "Формирование сети Zigbee 3.0...");
            ezb_bdb_start_top_level_commissioning(EZB_BDB_MODE_NETWORK_FORMATION);
        } else {
            ESP_LOGW(TAG, "BDB init status 0x%02x", status);
        }
    } break;

    case EZB_BDB_SIGNAL_FORMATION: {
        ezb_bdb_comm_status_t status = *((ezb_bdb_comm_status_t *)ezb_app_signal_get_params(app_signal));
        if (status == EZB_BDB_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "Сеть сформирована! PAN 0x%04hx, канал %d",
                     ezb_nwk_get_panid(), ezb_nwk_get_current_channel());
            ezb_bdb_open_network(180);
            if (s_lbl_status) {
                char buf[80];
                snprintf(buf, sizeof(buf), "Статус: сеть OK, PAN 0x%04X ch %d",
                         ezb_nwk_get_panid(), ezb_nwk_get_current_channel());
                lv_label_set_text(s_lbl_status, buf);
            }
        } else {
            ESP_LOGW(TAG, "Formation failed 0x%02x", status);
        }
    } break;

    case EZB_ZDO_SIGNAL_DEVICE_ANNCE: {
        const ezb_zdo_signal_device_annce_params_t *ann =
            (const ezb_zdo_signal_device_annce_params_t *)ezb_app_signal_get_params(app_signal);
        s_bound_relay_short_addr = ann->device_short_addr;
        ESP_LOGI(TAG, "Устройство подключено: 0x%04hx", s_bound_relay_short_addr);
        if (s_lbl_status) {
            char buf[64];
            snprintf(buf, sizeof(buf), "Реле подключено: 0x%04X", s_bound_relay_short_addr);
            lv_label_set_text(s_lbl_status, buf);
        }
    } break;

    case EZB_NWK_SIGNAL_PERMIT_JOIN_STATUS: {
        uint8_t duration = *(uint8_t *)ezb_app_signal_get_params(app_signal);
        if (duration) {
            ESP_LOGI(TAG, "Сеть открыта %d с", duration);
        } else {
            ESP_LOGI(TAG, "Сеть закрыта для join");
        }
    } break;

    default:
        ESP_LOGD(TAG, "Signal %s (0x%02x)", ezb_app_signal_to_string(signal_type), signal_type);
        break;
    }
    return true;
}

static esp_err_t create_coordinator_device(void)
{
    ezb_af_device_desc_t dev_desc = ezb_af_create_device_desc();
    ezb_zha_on_off_switch_config_t switch_cfg = EZB_ZHA_ON_OFF_SWITCH_CONFIG();
    ezb_af_ep_desc_t ep_desc = ezb_zha_create_on_off_switch(ZB_COORDINATOR_ENDPOINT, &switch_cfg);

    ESP_ERROR_CHECK(ezb_af_device_add_endpoint_desc(dev_desc, ep_desc));
    ESP_ERROR_CHECK(ezb_af_device_desc_register(dev_desc));
    return ESP_OK;
}

static void zigbee_task(void *pvParameters)
{
    (void)pvParameters;

    /* Dedicated NVS partition for Zigbee 2.x (optional but recommended) */
    esp_err_t nvs_zb = nvs_flash_init_partition("zb_storage");
    if (nvs_zb != ESP_OK) {
        ESP_LOGW(TAG, "zb_storage NVS init: %s (using default nvs)", esp_err_to_name(nvs_zb));
    }

    esp_zigbee_config_t config = ESP_ZIGBEE_DEFAULT_CONFIG();
    /* Coordinator role */
    config.device_config.device_type = EZB_NWK_DEVICE_TYPE_COORDINATOR;
    config.device_config.install_code_policy = false;
    config.device_config.zczr_config.max_children = 32;

    ESP_ERROR_CHECK(esp_zigbee_init(&config));
    ESP_ERROR_CHECK(ezb_app_signal_add_handler(esp_zigbee_app_signal_handler));
    ESP_ERROR_CHECK(ezb_bdb_set_primary_channel_set((1U << 13))); /* channel 13 */
    ESP_ERROR_CHECK(create_coordinator_device());
    ESP_ERROR_CHECK(esp_zigbee_start(false));

    esp_zigbee_launch_mainloop();
    esp_zigbee_deinit();
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Запуск шлюза Zigbee Coordinator (SDK 2.x)...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    lvgl_port_lock(0);
    create_hmi_gui();
    lvgl_port_unlock();

    xTaskCreate(zigbee_task, "zigbee_main", 8192, NULL, 5, NULL);
}

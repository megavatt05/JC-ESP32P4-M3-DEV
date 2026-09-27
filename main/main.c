#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "driver/i2c_master.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "esp_zigbee_core.h"

static const char *TAG = "JC1060_ZB_GW";

#define I2C_PORT_NUM            I2C_NUM_0
#define I2C_SDA_PIN             GPIO_NUM_7
#define I2C_SCL_PIN             GPIO_NUM_8

#define ZB_COORDINATOR_ENDPOINT 1
#define RELAY_EP_L1             1
#define RELAY_EP_L2             2

static uint16_t s_bound_relay_short_addr = 0xFFFF;
static lv_obj_t *s_sw_l1 = NULL;
static lv_obj_t *s_sw_l2 = NULL;
static lv_obj_t *s_lbl_status = NULL;

esp_err_t zb_send_relay_cmd(uint16_t short_addr, uint8_t endpoint, bool state)
{
      if (short_addr == 0xFFFF) {
        ESP_LOGW(TAG, "Устройство не привяано! Команда проигнорирована.");
        return ESP_ERR_INVALID_STATE;
      }

    esp_zb_zcl_on_off_cmd_t cmd_req;
    memset(&cmd_req, 0, sizeof(cmd_req));
    cmd_req.zcl_basic_cmd.src_endpoint = ZB_COORDINATOR_ENDPOINT;
    cmd_req.zcl_basic_cmd.dst_endpoint = endpoint;
    cmd_req.zcl_basic_cmd.dst_addr_u.addr_short = short_addr;
    cmd_req.address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    cmd_req.on_off_cmd_id = state ? ESP_ZB_ZCL_CMD_ON_OFF_ON_ID : ESP_ZB_ZCL_CMD_ON_OFF_OFF_ID;

    ESP_LOGI(TAG, "ZCL отправка: Кана %d -> %s (Адрес: 0x%04X)", endpoint, state ? "ВКЛ" : "ВЫКЛ", short_addr);

    esp_zb_lock_acquire(portMAX_DELAY);
    esp_err_t ret = esp_zb_zcl_on_off_cmd_req(&cmd_req);
    esp_zb_lock_release();

    return ret;
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
      ESP_LOGI(TAG, "Открытие сети Zigbee на 180 секунд...");
    esp_zb_lock_acquire(portMAX_DELAY);
    esp_zb_bdb_open_network(180);
    esp_zb_lock_release();
    if (s_lbl_status) {
        lv_label_set_text(s_lbl_status, "Статус: Поиск устойств (180с)...");
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
    lv_label_set_text(lbl_l2, "Кнал 2 (L2 - Бра):");
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
    lv_label_set_text(lbl_btn, "Соряжение (180с)");
    lv_obj_center(lbl_btn);

    s_lbl_status = lv_label_create(scr);
    lv_label_set_text(s_lbl_status, "Статус: Сеть Zigbee активна (Канал 13)");
    lv_obj_set_style_text_font(s_lbl_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_status, lv_color_hex(0x88C0D0), 0);
    lv_obj_align(s_lbl_status, LV_ALIGN_BOTTOM_RIGHT, -100, -65);
}

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
      ESP_ERROR_CHECK(esp_zb_bdb_start_top_level_commissioning(mode_mask));
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
      uint32_t *p_sg_p = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = (esp_zb_app_signal_type_t)*p_sg_p;

    switch (sig_type) {
case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Инициализация Zigbee координатора...");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;
case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK) {
            ESP_LOGI(TAG, "Формироване сети Zigbee 3.0...");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_FORMATION);
        }
        break;
case ESP_ZB_BDB_SIGNAL_FORMATION:
        if (err_status == ESP_OK) {
            ESP_LOGI(TAG, "Сеть сформирована! PAN ID: 0x%04X, Кана: %d",
                     esp_zb_get_pan_id(), esp_zb_get_current_channel());
            esp_zb_bdb_open_network(180);
        }
        break;
case ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE: {
        esp_zb_zdo_signal_device_annce_params_t *dev_annce_params =
            (esp_zb_zdo_signal_device_annce_params_t *)esp_zb_app_signal_get_params(p_sg_p);
        s_bound_relay_short_addr = dev_annce_params->device_short_addr;
        ESP_LOGI(TAG, "Новое Zigbee устройство подключено! Короткий адрес: 0x%04X", s_bound_relay_short_addr);
        if (s_lbl_status) {
            char buf[64];
            snprintf(buf, sizeof(buf), "Реле одключено: 0x%04X", s_bound_relay_short_addr);
            lv_label_set_text(s_lbl_status, buf);
        }
        break;
}
default:
        ESP_LOGD(TAG, "Сигнал ZDO: 0x%x, татус: %s", sig_type, esp_err_to_name(err_status));
        break;
    }
}

static void zigbee_task(void *pvParameters)
{
      esp_zb_cfg_t zb_nwk_cfg = {
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_COORDINATOR,
        .install_code_policy = false,
        .nwk_cfg.zczr_cfg = {
            .max_children = 32,
},
};
    esp_zb_init(&zb_nwk_cfg);

    esp_zb_on_off_switch_cfg_t switch_cfg = ESP_ZB_DEFAULT_ON_OFF_SWITCH_CONFIG();
    esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
    esp_zb_endpoint_config_t ep_cfg = {
        .endpoint = ZB_COORDINATOR_ENDPOINT,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_ON_OFF_SWITCH_DEVICE_ID,
        .app_device_version = 0,
};
    esp_zb_ep_list_add_ep(ep_list, esp_zb_on_off_switch_ep_create(ZB_COORDINATOR_ENDPOINT, &switch_cfg), ep_cfg);
    esp_zb_device_register(ep_list);

    esp_zb_set_primary_network_channel_set(1 << 13);
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_main_loop_iteration();
}

void app_main(void)
{
      ESP_LOGI(TAG, "Запуск шлюза JC1060P470C Zigbee Coordinator...");

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

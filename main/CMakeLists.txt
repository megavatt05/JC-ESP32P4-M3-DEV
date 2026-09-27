idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash driver esp_lcd esp_lcd_touch
    PRIV_REQUIRES esp-zigbee-lib
)

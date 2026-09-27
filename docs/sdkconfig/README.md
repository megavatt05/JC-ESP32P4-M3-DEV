# Примеры sdkconfig для JC-ESP32P4-M3-DEV

Файлы в этой папке — **фрагменты / defaults**, которые можно копировать в проект  
`esp_zigbee_gateway` или `ot_rcp` и донастраивать через `idf.py menuconfig`.

| Файл | Назначение |
|------|------------|
| `sdkconfig.defaults.gateway_ethernet` | Gateway (P4) + Ethernet + UART RCP |
| `sdkconfig.defaults.ot_rcp_c6` | ot_rcp для встроенного ESP32-C6 |
| `sdkconfig.defaults.gateway_notes.md` | Пояснения к ключевым опциям |

## Как использовать

```bash
# Gateway (P4)
cd $IDF_PATH/examples/zigbee/esp_zigbee_gateway
cp /path/to/this/repo/docs/sdkconfig/sdkconfig.defaults.gateway_ethernet sdkconfig.defaults
idf.py set-target esp32p4
idf.py menuconfig   # проверьте пины UART и Ethernet
idf.py build

# RCP (C6)
cd $IDF_PATH/examples/openthread/ot_rcp
cp /path/to/this/repo/docs/sdkconfig/sdkconfig.defaults.ot_rcp_c6 sdkconfig.defaults
idf.py set-target esp32c6
idf.py menuconfig   # OPENTHREAD_NCP_VENDOR_HOOK должен быть включён
idf.py build
```

**Важно:** пины UART (TX/RX/RST/BOOT) зависят от разводки expansion header.  
Проверьте схему в `5-Schematic` и при необходимости поправьте значения в menuconfig.

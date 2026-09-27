# jc1060-zigbee-gateway

Ветка **`c6-via-p4`**: Zigbee 3.0 Gateway + LVGL HMI для **JC1060P470C / JC-ESP32P4-M3-DEV**.

| Роль | Чип | Прошивка |
|------|-----|----------|
| Host (stack, UI, Ethernet) | **ESP32-P4** | этот проект |
| Radio (802.15.4) | **ESP32-C6** | `ot_rcp` |

Связь: **UART + Spinel** (`CONFIG_ZB_RADIO_SPINEL_UART`).

## Документация

- [docs/P4_C6_RCP_Setup.md](./docs/P4_C6_RCP_Setup.md) — пошагово P4 + C6 RCP

## Требования

- ESP-IDF **v6.0.3** (или v5.5.4+)
- `espressif/esp-zigbee-lib` **^2.0.4** (API `esp_zigbee.h` / `ezb_*`)
- Flash **≥ 4 MB** (на JC1060 обычно 16 MB)

## Файлы конфигурации

| Файл | Назначение |
|------|------------|
| `sdkconfig.defaults` | P4 + Spinel UART, flash 16 MB |
| `sdkconfig.defaults.esp32p4` | target-specific P4 |
| `sdkconfig.defaults.esp32c6` | C6 native (тест без RCP) |
| `partitions.csv` | app ~2 MB + `zb_storage` (nvs) |
| `main/main.c` | SDK 2.x coordinator + LVGL |

## Быстрый старт

```powershell
# C6 → ot_rcp (один раз)
cd $env:IDF_PATH\examples\openthread\ot_rcp
idf.py set-target esp32c6
idf.py menuconfig   # OPENTHREAD_NCP_VENDOR_HOOK=y
idf.py build && idf.py -p COMx flash

# P4 → gateway
cd path\to\this\project
idf.py set-target esp32p4
idf.py build && idf.py -p COMy flash monitor
```

Тест только на C6 (native radio):

```powershell
idf.py set-target esp32c6
idf.py build
```

## Зависимости (`main/idf_component.yml`)

```yaml
dependencies:
  idf: ">=5.1.0"
  espressif/esp-zigbee-lib: "^2.0.4"
  lvgl/lvgl: "^9.2.0"
  espressif/esp_lvgl_port: "^2.4.0"
```

## Важно

- После `ot_rcp` на C6 нет Wi-Fi ESP-Hosted → сеть на P4 через **Ethernet**.
- SDIO P4↔C6 не заменяет UART для Zigbee Spinel.
- `build/`, `sdkconfig`, `managed_components/` не коммитить (см. `.gitignore`).

# jc1060-zigbee-gateway

Zigbee 3.0 Gateway + LVGL HMI на **Guition JC1060P470C / JC-ESP32P4-M3-DEV**.

| Роль | Чип | Прошивка |
|------|-----|----------|
| Host (stack, UI, Ethernet) | **ESP32-P4** | этот проект |
| Radio (802.15.4) | **ESP32-C6** | `ot_rcp` (OpenThread RCP) |

Связь P4 ↔ C6: **UART + Spinel** (`CONFIG_ZB_RADIO_SPINEL_UART`).

## Документация

- **[docs/P4_C6_RCP_Setup.md](./docs/P4_C6_RCP_Setup.md)** — пошаговая настройка P4 + C6 RCP
- ESP-IDF: **v6.0.3** (или v5.5.4+)
- Zigbee: **esp-zigbee-lib ^2.0.4** (API `esp_zigbee.h` / `ezb_*`)

## Быстрый старт

```powershell
# 1) C6 → ot_rcp (один раз, USB-TTL)
cd $env:IDF_PATH\examples\openthread\ot_rcp
idf.py set-target esp32c6
idf.py menuconfig   # OPENTHREAD_NCP_VENDOR_HOOK = y
idf.py build && idf.py -p COMx flash

# 2) P4 → этот проект
cd C:\Users\megav\Downloads\gateway
idf.py set-target esp32p4
idf.py menuconfig   # Zigbee → Radio = Spinel UART
idf.py build && idf.py -p COMy flash monitor
```

Подробности: [docs/P4_C6_RCP_Setup.md](./docs/P4_C6_RCP_Setup.md).

## Зависимости (`idf_component.yml`)

```yaml
dependencies:
  espressif/esp-zigbee-lib: "^2.0.4"
  idf: ">=5.1.0"
```

## Важно

- После `ot_rcp` на C6 **нет Wi-Fi ESP-Hosted** — сеть на P4 через **Ethernet**.
- SDIO между P4 и C6 не заменяет UART для Zigbee Spinel.
- `zb_storage` в `partitions.csv` — subtype **nvs** (SDK 2.x).

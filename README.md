# JC-ESP32P4-M3-DEV (fork)

Плата **GUITION JC-ESP32P4-M3-DEV**: ESP32-P4 + ESP32-C6, 32 MB PSRAM, 16 MB Flash, Ethernet, ES8311, microSD.

Оригинал: [DRubioG/JC-ESP32P4-M3-DEV](https://github.com/DRubioG/JC-ESP32P4-M3-DEV)  
Ветка: **`feature/zigbee-gateway-configs`** — прошивка C6 и gateway.

---

## Главное: как прошить C6

**Отдельного USB-порта у C6 нет.** USB-C на плате идут только на **P4**.

| Способ | Как |
|--------|-----|
| **Внешний** (обязателен хотя бы раз) | USB-TTL 3.3 V → `C6_U0TXD` / `C6_U0RXD` / BOOT / EN на expansion header |
| **Через P4** | После `ot_rcp` на C6 — приложение на P4 с `AUTO_UPDATE_RCP` по UART |

Подробно: **[docs/C6_Programming.md](./docs/C6_Programming.md)**  
Пошаговый проект: **[firmware/c6_via_p4/](./firmware/c6_via_p4/)**

---

## Документация ветки

| Путь | Содержание |
|------|------------|
| [docs/C6_Programming.md](./docs/C6_Programming.md) | Порты, UART0 C6, download mode, Hosted vs RCP |
| [docs/Zigbee_Gateway_Adaptation.md](./docs/Zigbee_Gateway_Adaptation.md) | Zigbee Gateway + Ethernet + SDIO-конфликт |
| [docs/sdkconfig/](./docs/sdkconfig/) | Фрагменты sdkconfig |
| [firmware/c6_via_p4/](./firmware/c6_via_p4/) | Порядок прошивки C6 через / с помощью P4 |

Оригинальные папки платы (`1-Demo` … `8-Burn operation`, схемы) сохранены без изменений.

---

## Ограничения железа

- **SDIO**: C6 (Wi-Fi) и microSD делят GPIO 39–44 — одновременно нельзя.
- Для сети + SD → **Ethernet** (IP101G).
- ESP-IDF: **v5.5.x** (для gateway/RCP); v6.0.x — с учётом миграции.

---

## Быстрый старт C6 → P4 gateway

```bash
# 1) C6 = ot_rcp (внешний USB-TTL)
cd $IDF_PATH/examples/openthread/ot_rcp && idf.py set-target esp32c6 build
idf.py -p /dev/ttyUSB0 erase-flash flash

# 2) P4 = gateway + Ethernet + UART RCP
cd $IDF_PATH/examples/zigbee/esp_zigbee_gateway && idf.py set-target esp32p4
# menuconfig: Ethernet, AUTO_UPDATE_RCP, пины UART
idf.py build flash monitor
```

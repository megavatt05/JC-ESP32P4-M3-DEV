# Zigbee Gateway на JC-ESP32P4-M3-DEV

Руководство по запуску **ESP Zigbee Gateway** на плате GUITION JC-ESP32P4-M3-DEV  
(ESP32-P4 + встроенный ESP32-C6).

Оригинальный пример:  
https://github.com/espressif/esp-idf/tree/master/examples/zigbee/esp_zigbee_gateway

---

## 1. Архитектура платы

| Компонент        | Роль                              | Интерфейс          |
|------------------|-----------------------------------|--------------------|
| **ESP32-P4**     | Host (Zigbee stack + приложение)  | —                  |
| **ESP32-C6**     | Радио 802.15.4 / Wi-Fi / BLE      | SDIO (GPIO 39–44)  |
| **Ethernet**     | Сетевой интерфейс                 | RMII (IP101G)      |
| **microSD**      | Хранилище                         | SDIO (те же линии) |

**Критический конфликт:**  
ESP32-C6 (Wi-Fi/Zigbee) и слот microSD используют **одну и ту же SDIO-шину**.  
Одновременно работать нельзя.

---

## 2. Почему нельзя просто «включить Zigbee по SDIO»

- Официальный `esp_zigbee_gateway` общается с RCP **только по UART + протокол Spinel**.
- Spinel поверх SDIO в ESP Zigbee SDK / ESP-IDF **не реализован**.
- В ESP-Hosted (v2/v3) control-plane идёт по SDIO, но **данные 802.15.4 (Spinel) всегда требуют отдельный UART**.

Поэтому «чистая» работа только по SDIO без UART — это кастомная разработка.

---

## 3. Рекомендуемые варианты запуска

### Вариант A (рекомендуемый) — ot_rcp на встроенном C6 + UART + Ethernet

1. Прошиваем встроенный **ESP32-C6** прошивкой `ot_rcp`.
2. Связываем P4 ↔ C6 по **UART** (пины с expansion-разъёма).
3. На P4 используем **Ethernet** (Wi-Fi через C6 становится недоступен).

**Плюсы:** работает «из коробки», SD-карта свободна, стабильно.  
**Минусы:** теряется Wi-Fi C6.

#### Шаги

```bash
# 1. RCP (C6)
cd $IDF_PATH/examples/openthread/ot_rcp
idf.py set-target esp32c6
idf.py menuconfig   # включить OPENTHREAD_NCP_VENDOR_HOOK
idf.py build
# Прошить C6 через его UART0 (expansion header: C6_U0TXD / C6_U0RXD + BOOT + EN)

# 2. Gateway (P4)
cd $IDF_PATH/examples/zigbee/esp_zigbee_gateway
idf.py set-target esp32p4
idf.py menuconfig
# - Example Connection → Ethernet
# - ESP Zigbee gateway rcp update → указать TX/RX/RST/BOOT пины
idf.py build
idf.py -p PORT erase-flash flash monitor
```

Типичные пины UART на похожих платах (проверьте схему):

| Сигнал     | GPIO на P4 (пример) |
|------------|---------------------|
| RX (к TX C6) | 4                   |
| TX (к RX C6) | 5                   |
| RST / EN   | 7 или 54            |
| BOOT       | 8                   |

Точные пины берите из `5-Schematic` и expansion header платы.

---

### Вариант B — Wi-Fi на C6 + внешний RCP

- C6 остаётся в режиме **ESP-Hosted** (Wi-Fi/BT по SDIO).
- Zigbee-радио — **внешний** ESP32-C6 или ESP32-H2, подключённый по UART к P4.

Полностью поддерживается официально, конфликтов SDIO нет.

---

### Вариант C — кастомный Spinel over SDIO (сложный)

Требует:

1. Специальной прошивки C6 (Hosted + 802.15.4 RCP одновременно).
2. Реализации Spinel-транспорта поверх SDIO / CustomRpc в `esp_zigbee_radio_spinel_*`.
3. Значительных доработок кода.

На текущий момент готового решения нет. Не рекомендуется как первый шаг.

---

## 4. Ожидаемый лог успешного запуска (Вариант A)

```
I (...) ESP_ZIGBEE_RCP: Running RCP version: openthread-esp32/...; esp32c6; ...
I (...) example_connect: Got IPv4 event: ... (Ethernet)
I (...) ZIGBEE_GATEWAY: Formed network successfully: PAN ID(0x....), Channel(13), Short Address(0x0000)
I (...) ZIGBEE_GATEWAY: Network(...) is open for 180 seconds
I (...) ZIGBEE_GATEWAY: Network steering completed
```

---

## 5. Полезные ссылки

- [ESP Zigbee Gateway example](https://github.com/espressif/esp-idf/tree/master/examples/zigbee/esp_zigbee_gateway)
- [ot_rcp example](https://github.com/espressif/esp-idf/tree/master/examples/openthread/ot_rcp)
- [ESP Zigbee SDK](https://github.com/espressif/esp-zigbee-sdk)
- [ESP Thread BR для P4 + C6](https://github.com/espressif/esp-thread-br/blob/main/examples/basic_thread_border_router/README_esp32p4.md)
- [ESP-Hosted](https://github.com/espressif/esp-hosted)

---

## 6. Рекомендации по IDF

- **ESP-IDF v5.5.x** (рекомендуется для P4 + C6 и Zigbee SDK v2.x).
- Перед первой прошивкой: `idf.py erase-flash`.
- При использовании SD-карты + Ethernet — Wi-Fi C6 не активировать.

---

*Документация добавлена в форк [megavatt05/JC-ESP32P4-M3-DEV](https://github.com/megavatt05/JC-ESP32P4-M3-DEV).*

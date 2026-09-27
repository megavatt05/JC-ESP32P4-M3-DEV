# Прошивка ESP32-C6 через ESP32-P4 (JC-ESP32P4-M3-DEV)

Минимальный рабочий путь: **один раз** прошить C6 снаружи, дальше P4 обновляет RCP по UART.

Полная справка по портам: [docs/C6_Programming.md](../../docs/C6_Programming.md)

## Важно про порты

- **Отдельного USB для C6 нет.**
- USB-C на плате → только **P4**.
- C6: expansion header `C6_U0TXD` / `C6_U0RXD` / `C6_IO9` (BOOT) / EN + внешний USB-TTL 3.3 V.

## Шаг 1 — первая прошивка C6 (внешний UART)

```bash
cd $IDF_PATH/examples/openthread/ot_rcp
idf.py set-target esp32c6
idf.py menuconfig   # OPENTHREAD_NCP_VENDOR_HOOK = y
idf.py build
# USB-TTL на C6_U0*, download mode (BOOT+EN)
idf.py -p /dev/ttyUSB0 erase-flash flash
```

Альтернатива (Wi-Fi slave): образ ESP-Hosted `network_adapter_esp32c6.bin` тем же UART.

## Шаг 2 — P4 как host с AUTO_UPDATE_RCP

Используйте официальный пример (не копируйте весь IDF в этот репозиторий):

```bash
cd $IDF_PATH/examples/zigbee/esp_zigbee_gateway
# или Thread BR: esp-thread-br / examples/basic_thread_border_router

idf.py set-target esp32p4
idf.py menuconfig
```

Настройки:

| Раздел | Значение |
|--------|----------|
| Example Connection | **Ethernet** (IP101G) |
| ESP Zigbee gateway rcp update | пины UART к C6, `AUTO_UPDATE_RCP=y` |

Фрагменты defaults: [docs/sdkconfig/](../../docs/sdkconfig/)

```bash
idf.py build
idf.py -p PORT erase-flash flash monitor   # PORT = USB-UART платы (P4)
```

При старте P4 читает версию RCP по UART; при несовпадении заливает упакованный образ.

## Пины UART P4 ↔ C6 (проверить по schematic.pdf)

| Функция | GPIO P4 (ориентир) | C6 |
|---------|-------------------|-----|
| RX | 4 | U0TXD |
| TX | 5 | U0RXD |
| RST | 54 | EN / CHIP_PU |
| BOOT | 8 | IO9 |

Если на вашей ревизии платы UART C6 не разведён на эти GPIO — используйте только внешний USB-TTL (шаг 1) и не включайте auto-update.

## SDIO

C6 Hosted (Wi-Fi) занимает GPIO 39–44 вместе с microSD.  
После прошивки `ot_rcp` Wi-Fi C6 недоступен — используйте **Ethernet** на P4.

## Структура этой папки

| Файл | Назначение |
|------|------------|
| README.md | этот файл |
| sdkconfig.defaults | опциональные defaults для gateway на P4 |

Код приложения — из ESP-IDF examples; сюда не дублируется.

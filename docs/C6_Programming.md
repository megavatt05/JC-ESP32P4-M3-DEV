# Прошивка ESP32-C6 на JC-ESP32P4-M3-DEV

## Есть ли отдельный порт для программирования C6?

**Нет.** На плате **нет** отдельного USB-порта, подключённого к ESP32-C6.

| Порт на плате | Куда подключён |
|---------------|----------------|
| USB-C UART (обычно средний) | ESP32-**P4** (через USB-UART мост) |
| USB-C Full-Speed | ESP32-P4 USB FS |
| USB-C High-Speed | ESP32-P4 USB HS OTG |

ESP32-C6 прошивается **только** так:

1. **Внешний USB-TTL** → контакты **UART0 C6** на expansion header модуля/платы  
2. Либо **через P4 по UART** (если линии C6 UART0 разведены на GPIO P4 и используется RCP auto-update / slave flasher)

---

## Контакты C6 для внешней прошивки (UART0)

На модуле JC-ESP32P4-M3-C6 и expansion header DEV-платы выведены:

| Сигнал C6 | Назначение | Типичная метка на плате |
|-----------|------------|-------------------------|
| U0TXD | TX → к RX адаптера | `C6_U0TXD` |
| U0RXD | RX ← от TX адаптера | `C6_U0RXD` |
| IO9 | BOOT (strapping, pull-down для download) | `C6_IO9` / BOOT |
| CHIP_PU / EN | Reset | `C6_CHIP_PU` / EN |
| GND | Общий | GND |

**Подключение USB-TTL (3.3 V):**

```
USB-TTL TX  →  C6_U0RXD
USB-TTL RX  →  C6_U0TXD
USB-TTL GND →  GND
USB-TTL 3V3 →  НЕ подключать (питание от платы)
```

**Вход в download mode:**
1. Удержать BOOT (IO9 → GND)
2. Кратко нажать EN/RST (CHIP_PU)
3. Отпустить BOOT
4. `esptool.py` / `idf.py -p /dev/ttyUSB0 flash`

Стандартные пины C6: UART0 TX=GPIO16, RX=GPIO17 (на кристалле); на разъёме платы они выведены как C6_U0*.

---

## Прошивка C6 «через P4»

Два рабочих сценария Espressif:

### A. RCP Auto-Update (рекомендуется для Zigbee/Thread)

1. Один раз прошить C6 внешней UART-прошивкой `ot_rcp` (см. выше).
2. На P4 запустить приложение с `CONFIG_AUTO_UPDATE_RCP` (как в `esp_zigbee_gateway` / Thread BR).
3. P4 по UART (пины RX/TX/RST/BOOT к C6) сам заливает новый образ RCP при несовпадении версии.

Нужна **физическая разводка UART** между P4 GPIO и C6 U0TX/U0RX/EN/BOOT.  
На многих Guition-платах эти линии доступны на header; проверьте `5-Schematic/schematic.pdf`.

Ориентиры GPIO P4 (уточнять по схеме):

| Роль на P4 | Пример GPIO |
|------------|-------------|
| RX (← C6 TX) | 4 |
| TX (→ C6 RX) | 5 |
| RST C6 | 54 |
| BOOT C6 | 8 |

### B. ESP-Hosted slave firmware

C6 по умолчанию часто идёт с **ESP-Hosted** (Wi-Fi по SDIO).  
Обновление hosted-образа C6 — снова через **внешний UART0**, не через SDIO download.

Файлы: https://esphome.github.io/esp-hosted-firmware/ (network_adapter_esp32c6.bin).

---

## Что НЕ работает

- Прошивка C6 через USB-порт платы (они только к P4).
- Прошивка C6 «по SDIO» штатным esptool — SDIO у C6 в режиме slave для Hosted, не ROM download.
- Одновременно Wi-Fi (C6 Hosted) и microSD — конфликт SDIO GPIO39–44.

---

## Минимальный порядок для Zigbee Gateway (Вариант A)

```bash
# 1) Внешний USB-TTL → C6 UART0
cd $IDF_PATH/examples/openthread/ot_rcp
idf.py set-target esp32c6
# menuconfig: OPENTHREAD_NCP_VENDOR_HOOK=y
idf.py build
idf.py -p /dev/ttyUSB0 erase-flash flash

# 2) P4: gateway + Ethernet + UART к C6
cd $IDF_PATH/examples/zigbee/esp_zigbee_gateway
idf.py set-target esp32p4
# menuconfig: Ethernet, AUTO_UPDATE_RCP, пины UART
idf.py build flash monitor
```

Подробнее: [Zigbee_Gateway_Adaptation.md](./Zigbee_Gateway_Adaptation.md)

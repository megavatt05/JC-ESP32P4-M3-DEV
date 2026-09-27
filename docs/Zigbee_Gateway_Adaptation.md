# Zigbee Gateway на JC-ESP32P4-M3-DEV

Руководство по запуску **ESP Zigbee Gateway** на плате GUITION JC-ESP32P4-M3-DEV  
(ESP32-P4 + встроенный ESP32-C6).

Оригинальный пример:  
https://github.com/espressif/esp-idf/tree/master/examples/zigbee/esp_zigbee_gateway

Примеры `sdkconfig`: см. папку [`docs/sdkconfig/`](./sdkconfig/).

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

### Вариант B — Wi-Fi на C6 + внешний RCP

- C6 остаётся в режиме **ESP-Hosted** (Wi-Fi/BT по SDIO).
- Zigbee-радио — **внешний** ESP32-C6 или ESP32-H2, подключённый по UART к P4.

### Вариант C — кастомный Spinel over SDIO (сложный)

Требует специальной прошивки C6 и своего транспорта Spinel. Не рекомендуется как первый шаг.

---

## 4. Прошивка встроенного ESP32-C6 (ot_rcp)

### 4.1. Подготовка

- ESP-IDF **v5.5.x**
- USB-TTL адаптер 3.3 V (TX/RX/GND)
- Пины C6 на expansion header платы (типичные названия):  
  `C6_U0TXD`, `C6_U0RXD`, `C6_IO9` (BOOT), `C6_CHIP_PU` / EN (reset)

### 4.2. Подключение USB-TTL

| USB-TTL | → | Разъём платы (C6) |
|---------|---|-------------------|
| TX      | → | C6_U0RXD          |
| RX      | → | C6_U0TXD          |
| GND     | → | GND               |
| 3V3     | ✗ | не подключать (плата питает C6) |

Для входа в download mode: удерживайте BOOT, кратковременно нажмите EN/RST, отпустите BOOT.

### 4.3. Сборка и прошивка

```bash
cd $IDF_PATH/examples/openthread/ot_rcp

# Подставьте defaults из этого репозитория (по желанию)
# cp /path/to/JC-ESP32P4-M3-DEV/docs/sdkconfig/sdkconfig.defaults.ot_rcp_c6 sdkconfig.defaults

idf.py set-target esp32c6
idf.py menuconfig
# Component config → OpenThread → ... → OPENTHREAD_NCP_VENDOR_HOOK = y

idf.py build
idf.py -p /dev/ttyUSB0 erase-flash flash   # порт вашего USB-TTL
```

**Важно:** прошивка `ot_rcp` **затирает** ESP-Hosted. Wi-Fi через C6 перестанет работать, пока не вернёте hosted-прошивку.

### 4.4. Возврат ESP-Hosted на C6 (если нужно)

Скачайте актуальный `network_adapter_esp32c6.bin` (например с  
https://esphome.github.io/esp-hosted-firmware/) и прошейте тем же способом через UART0 C6.

---

## 5. Ethernet на ESP32-P4

На JC-ESP32P4-M3-DEV установлен PHY **IP101G** (100 Mbps, RMII).

### 5.1. Зачем Ethernet для Gateway

- Не занимает SDIO → microSD остаётся доступной.
- Стабильнее Wi-Fi для координатора Zigbee.
- Не конфликтует с RCP на C6 (после прошивки ot_rcp Wi-Fi всё равно недоступен).

### 5.2. Типичные пины Ethernet (уточняйте по схеме)

| Сигнал     | GPIO (ориентир) |
|------------|-----------------|
| MDC        | 31              |
| MDIO       | 52              |
| PHY power  | 51              |
| REF_CLK    | 50              |

В menuconfig gateway:

```
Example Connection Configuration
  → Connect using Ethernet
  → Internal EMAC + external PHY (IP101)
```

Либо используйте готовый фрагмент:  
[`docs/sdkconfig/sdkconfig.defaults.gateway_ethernet`](./sdkconfig/sdkconfig.defaults.gateway_ethernet)

### 5.3. Проверка

После `flash monitor` ожидайте:

```
I (...) example_connect: Got IPv4 event: Interface "..." address: 192.168.x.x
```

Если IP нет — проверьте кабель, питание PHY и пины MDC/MDIO/CLK в menuconfig.

---

## 6. Сборка Zigbee Gateway (P4)

```bash
cd $IDF_PATH/examples/zigbee/esp_zigbee_gateway

# Опционально: defaults из репозитория
# cp /path/to/JC-ESP32P4-M3-DEV/docs/sdkconfig/sdkconfig.defaults.gateway_ethernet sdkconfig.defaults

idf.py set-target esp32p4
idf.py menuconfig
```

Ключевые пункты menuconfig:

1. **Example Connection Configuration** → Ethernet (не Wi-Fi).
2. **ESP Zigbee gateway rcp update** → пины TX / RX / RST / BOOT к C6.
3. `CONFIG_AUTO_UPDATE_RCP` — включить (host сможет обновлять C6).

```bash
idf.py build
idf.py -p PORT erase-flash flash monitor
```

Порт — USB UART платы (обычно средний Type-C, помеченный UART).

### Пины UART P4 ↔ C6 (примеры)

| Сигнал на P4     | Назначение        | Пример GPIO |
|------------------|-------------------|-------------|
| RX               | ← TX C6           | 4           |
| TX               | → RX C6           | 5           |
| RST / EN         | Reset C6          | 54 или 7    |
| BOOT             | Boot C6           | 8           |

**Обязательно сверьте** с `5-Schematic` и подписями expansion header.

---

## 7. Ожидаемый лог успешного запуска (Вариант A)

```
I (...) ESP_ZIGBEE_RCP: Running RCP version: openthread-esp32/...; esp32c6; ...
I (...) example_connect: Got IPv4 event: ... (Ethernet)
I (...) ZIGBEE_GATEWAY: Formed network successfully: PAN ID(0x....), Channel(13), Short Address(0x0000)
I (...) ZIGBEE_GATEWAY: Network(...) is open for 180 seconds
I (...) ZIGBEE_GATEWAY: Network steering completed
```

Сеть открыта 180 секунд — в это время присоединяйте Zigbee-устройства.

---

## 8. Примеры sdkconfig

| Файл | Описание |
|------|----------|
| [sdkconfig.defaults.gateway_ethernet](./sdkconfig/sdkconfig.defaults.gateway_ethernet) | Gateway (P4) + Ethernet + UART RCP |
| [sdkconfig.defaults.ot_rcp_c6](./sdkconfig/sdkconfig.defaults.ot_rcp_c6) | ot_rcp для ESP32-C6 |
| [sdkconfig/README.md](./sdkconfig/README.md) | Как подключать defaults |

---

## 9. Полезные ссылки

- [ESP Zigbee Gateway example](https://github.com/espressif/esp-idf/tree/master/examples/zigbee/esp_zigbee_gateway)
- [ot_rcp example](https://github.com/espressif/esp-idf/tree/master/examples/openthread/ot_rcp)
- [ESP Zigbee SDK](https://github.com/espressif/esp-zigbee-sdk)
- [ESP Thread BR для P4 + C6](https://github.com/espressif/esp-thread-br/blob/main/examples/basic_thread_border_router/README_esp32p4.md)
- [ESP-Hosted](https://github.com/espressif/esp-hosted)
- [ESP-Hosted firmware (C6)](https://esphome.github.io/esp-hosted-firmware/)

---

## 10. Рекомендации по IDF и плате

- **ESP-IDF v5.5.x**
- Ревизия чипа P4: при необходимости в menuconfig выберите поддержку rev < 3.0
- Перед первой прошивкой: `idf.py erase-flash`
- Ethernet + SD-карта — предпочтительная комбинация для production
- Wi-Fi C6 и microSD одновременно — нельзя

---

*Документация и примеры sdkconfig: форк [megavatt05/JC-ESP32P4-M3-DEV](https://github.com/megavatt05/JC-ESP32P4-M3-DEV), ветка `feature/zigbee-gateway-configs`.*

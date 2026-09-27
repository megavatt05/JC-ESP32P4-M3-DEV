# P4 + C6 RCP: пошаговая настройка Zigbee Gateway

Плата: **JC-ESP32P4-M3-DEV / JC1060P470C**  
Хост: **ESP32-P4** (Zigbee stack, LVGL, Ethernet)  
Радио: **ESP32-C6** в режиме **OpenThread RCP** (`ot_rcp`) по **UART + Spinel**

> P4 **не имеет** IEEE 802.15.4. Радио всегда на C6.

---

## Архитектура

```
┌─────────────────────┐  UART Spinel   ┌──────────────────┐
│  ESP32-P4 (host)    │◄──────────────►│  ESP32-C6 (RCP)  │
│  Zigbee SDK 2.x     │  TX/RX/RST/BOOT│  ot_rcp          │
│  Coordinator        │                │  802.15.4 radio  │
│  LVGL / Ethernet    │                │                  │
└─────────────────────┘                └──────────────────┘
```

---

## Шаг 0. Окружение

- ESP-IDF **v6.0.3** (или v5.5.4+)
- VS Code + Espressif extension (по желанию)
- USB-C платы → прошивка **только P4**
- Для **первой** прошивки C6: USB-TTL на `C6_U0TXD` / `C6_U0RXD` + BOOT/EN

```powershell
& 'C:\Espressif\tools\Microsoft.v6.0.3.PowerShell_profile.ps1'
cd C:\Users\megav\Downloads\gateway
```

---

## Шаг 1. Прошить C6 как `ot_rcp`

C6 должен работать как Radio Co-Processor (Spinel), **не** как ESP-Hosted Wi-Fi slave.

```powershell
cd $env:IDF_PATH\examples\openthread\ot_rcp
idf.py set-target esp32c6
idf.py menuconfig
```

В menuconfig:

1. **Component config → OpenThread**
   - включить **OpenThread**
2. **OpenThread RCP Example** (или Analog / NCP)
   - **OPENTHREAD_NCP_VENDOR_HOOK** = **y** (обязательно для Zigbee gateway)
3. UART пины C6 (если «Configure RCP UART pin manually»):
   - по умолчанию часто UART0: TX/RX штатные
   - на JC-плате выведите UART0 C6 на header (`C6_U0TXD` / `C6_U0RXD`)

Сборка и прошивка C6 (через USB-TTL):

```powershell
idf.py build
# Подключите USB-TTL: GND, TX→C6_RX, RX→C6_TX, BOOT к GND при старте
idf.py -p COMx flash
```

Проверка: после сброса C6 в мониторе не должно быть «hosted» логов Wi-Fi — только RCP/Spinel.

---

## Шаг 2. Соединение UART P4 ↔ C6

На **JC-ESP32P4-M3-DEV** встроенный C6 связан с P4 в основном по **SDIO** (ESP-Hosted).  
Для Zigbee RCP нужен **UART** (как в официальном gateway).

Типовая схема (проверьте схему платы / header):

| Функция        | P4 (host)     | C6 (RCP)        |
|----------------|---------------|-----------------|
| GND            | GND           | GND             |
| UART RX (P4)   | GPIO4*        | C6 U0 TX        |
| UART TX (P4)   | GPIO5*        | C6 U0 RX        |
| RESET (опц.)   | GPIO7*        | C6 EN / RST     |
| BOOT (опц.)    | GPIO8*        | C6 BOOT (GPIO9) |

\* Пины по умолчанию как у ESP Thread BR / Zigbee Gateway — **замените на реальные** с вашей распиновки JC-платы.

Документация платы: репозиторий / `5-Schematic`, header с `C6_U0TXD`, `C6_U0RXD`.

> После перевода C6 в `ot_rcp` **Wi-Fi через C6 (ESP-Hosted) недоступен**.  
> Для сети на P4 используйте **Ethernet**.

---

## Шаг 3. Настроить проект Gateway на P4

```powershell
cd C:\Users\megav\Downloads\gateway

# Чистая конфигурация под P4
Remove-Item -Force sdkconfig -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue

idf.py set-target esp32p4
```

### menuconfig (обязательно)

```powershell
idf.py menuconfig
```

**Component config → Zigbee**

| Опция | Значение |
|--------|----------|
| Zigbee Enable | **y** |
| Zigbee SDK 1.x.x | **n** |
| Device type | **Coordinator or Router (ZCZR)** |
| Radio type | **Connect to 15.4 radio via Radio Spinel UART** |

**Не** выбирайте *Native 15.4 radio* — у P4 его нет.

**Component config → OpenThread** (появляется из-за Spinel):

- Spinel / RCP host side — оставить по умолчанию для UART

Пины UART к RCP (если есть меню *ESP Zigbee gateway rcp* / *Radio Spinel*):

- RX pin → GPIO, соединённый с **TX C6**
- TX pin → GPIO, соединённый с **RX C6**
- baudrate **115200** (или как в `ot_rcp`)

Сохранить (`S`) → Exit.

---

## Шаг 4. Зависимости и сборка P4

В `idf_component.yml` должно быть:

```yaml
dependencies:
  espressif/esp-zigbee-lib: "^2.0.4"
  idf: ">=5.1.0"
```

(без `esp-zboss-lib`)

```powershell
Remove-Item -Recurse -Force managed_components -ErrorAction SilentlyContinue
Remove-Item -Force dependencies.lock -ErrorAction SilentlyContinue

idf.py reconfigure
idf.py build
idf.py -p COMy flash monitor
```

`COMy` — порт **USB-C платы (P4)**.

---

## Шаг 5. Ожидаемые логи

Успешный старт Spinel + RCP:

```text
ESP_RADIO_SPINEL: spinel UART interface initialization completed
ESP_RADIO_SPINEL: Spinel UART interface has been successfully enabled
ESP_ZIGBEE_RADIO_SPINEL_UART: Spinel UART interface enable successfully
OPENTHREAD: co-processor reset / Software reset RCP successfully
Running RCP version: ... esp32c6 ...
```

Далее — формирование сети координатора (канал, PAN ID).

Если таймаут / нет ответа RCP:

1. Проверить перекрёст TX↔RX и GND  
2. C6 реально прошит `ot_rcp` + `NCP_VENDOR_HOOK`  
3. Одинаковый baudrate  
4. RESET C6 (линия EN)

---

## Шаг 6 (опционально). AUTO_UPDATE_RCP

Официальный `esp_zigbee_gateway` умеет упаковывать образ `ot_rcp` в прошивку P4 и обновлять C6 по UART.

Для этого:

1. Собрать `ot_rcp` под **esp32c6**  
2. В gateway включить **CONFIG_AUTO_UPDATE_RCP**  
3. Указать пути/пины RST+BOOT  

См. `$IDF_PATH/examples/zigbee/esp_zigbee_gateway`.

На старте достаточно **один раз** прошить C6 вручную (шаг 1).

---

## Краткая шпаргалка команд

```powershell
# --- C6 RCP (один раз) ---
cd $env:IDF_PATH\examples\openthread\ot_rcp
idf.py set-target esp32c6
idf.py menuconfig   # NCP_VENDOR_HOOK=y
idf.py build
idf.py -p COMx flash   # USB-TTL на C6

# --- P4 Gateway ---
cd C:\Users\megav\Downloads\gateway
idf.py set-target esp32p4
idf.py menuconfig   # ZB_RADIO_SPINEL_UART=y, ZCZR=y
idf.py build
idf.py -p COMy flash monitor
```

---

## Важно

| Тема | Решение |
|------|--------|
| Нет Wi-Fi на C6 после ot_rcp | Нормально; используйте **Ethernet** на P4 |
| Сборка под C6 с NATIVE | Только если Zigbee целиком на C6, не gateway на P4 |
| SDIO C6 | Занят Hosted/microSD; для Zigbee RCP — **UART**, не SDIO |
| SDK | `esp-zigbee-lib` **^2.0.4**, API `esp_zigbee.h` / `ezb_*` |

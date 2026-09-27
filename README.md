# JC-ESP32P4-M3-DEV

Репозиторий платы **GUITION JC-ESP32P4-M3-DEV**  
(ESP32-P4 + ESP32-C6, 32 MB PSRAM, 16 MB Flash, Ethernet, ES8311, microSD и др.)

Форк оригинала: [DRubioG/JC-ESP32P4-M3-DEV](https://github.com/DRubioG/JC-ESP32P4-M3-DEV)

---

## Содержимое

| Папка / файл | Описание |
|--------------|----------|
| [1-Demo](./1-Demo/) | Демонстрационные проекты |
| [2-Specification](./2-Specification/) | Спецификации |
| [3-Structure_Diagram](./3-Structure_Diagram/) | Структурные схемы |
| [4-Driver_IC_Data_Sheet](./4-Driver_IC_Data_Sheet/) | Даташиты драйверов |
| [5-Schematic](./5-Schematic/) | Схемы платы |
| [6-User_Manual](./6-User_Manual/) | Руководство пользователя |
| [8-Burn operation](./8-Burn%20operation/) | Инструкции по прошивке |
| **[docs/Zigbee_Gateway_Adaptation.md](./docs/Zigbee_Gateway_Adaptation.md)** | **Адаптация Zigbee Gateway под встроенный C6 (SDIO / UART)** |

---

## Важные аппаратные ограничения

- **SDIO-конфликт**: ESP32-C6 (Wi-Fi / 802.15.4) и microSD используют одни и те же линии (GPIO 39–44).  
  При активном Wi-Fi/Zigbee через C6 карту SD использовать нельзя.
- Для проектов, где нужны и сеть, и SD-карта — предпочтителен **Ethernet**.
- Рекомендуемая версия ESP-IDF: **v5.5.x**.

---

## Быстрый старт Zigbee Gateway

См. подробное руководство:  
**[docs/Zigbee_Gateway_Adaptation.md](./docs/Zigbee_Gateway_Adaptation.md)**

Кратко:

1. **Вариант A (рекомендуется)** — прошить C6 как `ot_rcp`, связать с P4 по UART, использовать Ethernet.
2. **Вариант B** — оставить C6 в ESP-Hosted (Wi-Fi) + внешний RCP по UART.
3. Чистый Spinel поверх SDIO пока официально не поддерживается.

---

## Полезные ссылки

- [ESP Zigbee Gateway example](https://github.com/espressif/esp-idf/tree/master/examples/zigbee/esp_zigbee_gateway)
- [ESP Zigbee SDK](https://github.com/espressif/esp-zigbee-sdk)
- [ESP-IDF v5.5](https://github.com/espressif/esp-idf)

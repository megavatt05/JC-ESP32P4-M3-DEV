# JC-ESP32P4-M3-DEV

Репозиторий платы **GUITION JC-ESP32P4-M3-DEV**  
(ESP32-P4 + ESP32-C6, 32 MB PSRAM, 16 MB Flash, Ethernet, ES8311, microSD и др.)

Форк оригинала: [DRubioG/JC-ESP32P4-M3-DEV](https://github.com/DRubioG/JC-ESP32P4-M3-DEV)

Ветка с примерами Zigbee Gateway: **`feature/zigbee-gateway-configs`**

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
| **[docs/Zigbee_Gateway_Adaptation.md](./docs/Zigbee_Gateway_Adaptation.md)** | **Zigbee Gateway: адаптация, Ethernet, прошивка C6** |
| **[docs/sdkconfig/](./docs/sdkconfig/)** | **Примеры sdkconfig (gateway + ot_rcp)** |

---

## Важные аппаратные ограничения

- **SDIO-конфликт**: ESP32-C6 (Wi-Fi / 802.15.4) и microSD используют одни и те же линии (GPIO 39–44).  
  При активном Wi-Fi/Zigbee через C6 карту SD использовать нельзя.
- Для проектов, где нужны и сеть, и SD-карта — предпочтителен **Ethernet**.
- Рекомендуемая версия ESP-IDF: **v5.5.x**.

---

## Быстрый старт Zigbee Gateway

Полное руководство:  
**[docs/Zigbee_Gateway_Adaptation.md](./docs/Zigbee_Gateway_Adaptation.md)**

В нём есть:
- варианты запуска (A / B / C);
- **прошивка встроенного C6** (`ot_rcp`) через UART0;
- **настройка Ethernet** (IP101G);
- сборка gateway на P4;
- ссылки на готовые `sdkconfig.defaults`.

Кратко (Вариант A):

1. Прошить C6 → `ot_rcp` (см. раздел 4 в гайде).
2. На P4: Ethernet + UART к C6 + `esp_zigbee_gateway`.
3. Использовать defaults из [`docs/sdkconfig/`](./docs/sdkconfig/).

---

## Полезные ссылки

- [ESP Zigbee Gateway example](https://github.com/espressif/esp-idf/tree/master/examples/zigbee/esp_zigbee_gateway)
- [ot_rcp](https://github.com/espressif/esp-idf/tree/master/examples/openthread/ot_rcp)
- [ESP Zigbee SDK](https://github.com/espressif/esp-zigbee-sdk)
- [ESP-IDF v5.5](https://github.com/espressif/esp-idf)

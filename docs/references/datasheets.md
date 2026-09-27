# Datasheets and References

Key reference documents for LoRaLine hardware and software components. All links verified as of September 2026.

---

## Radio

| Component | Document | Link |
|---|---|---|
| SX1262 | Semtech SX1262 Datasheet (DS.SX1261-2.W.APP, v2.1) | https://semtech.com/products/wireless-rf/lora-connect/sx1262 |
| SX1262 | Semtech AN1200.36: SX1261/2 Power Consumption and Link Budget | https://semtech.com/uploads/documents/an1200.36_sx126x_power_consumption_and_link_budget.pdf |
| RadioLib | RadioLib Library Documentation (JGROMES) | https://jgromes.github.io/RadioLib/ |
| Meshtastic | Meshtastic Device Firmware Source (reference for SX1262 init) | https://github.com/meshtastic/firmware |

---

## Microcontrollers

| Component | Document | Link |
|---|---|---|
| ESP32-WROOM-32 | Espressif ESP32-WROOM-32 Datasheet | https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32_datasheet_en.pdf |
| ESP32 Technical Reference | Espressif ESP32 Technical Reference Manual | https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf |
| ESP32-C3 Mini | Espressif ESP32-C3-MINI-1 Datasheet | https://www.espressif.com/sites/default/files/documentation/esp32-c3-mini-1_datasheet_en.pdf |

---

## GPS

| Component | Document | Link |
|---|---|---|
| NEO-6M | u-blox NEO-6 Product Summary | https://www.u-blox.com/en/product/neo-6-series |
| NEO-6M | u-blox NEO-6M Hardware Integration Manual | https://content.u-blox.com/sites/default/files/products/documents/NEO-6_HardwareIntegrationManual_(GPS.G6-HW-09007).pdf |

---

## Sensors

| Component | Document | Link |
|---|---|---|
| MQ-2 | MQ-2 Gas Sensor Datasheet (Hanwei Electronics) | https://www.pololu.com/file/0J309/MQ2.pdf |

---

## TinyML and Firmware Libraries

| Component | Document | Link |
|---|---|---|
| TensorFlow Lite Micro | TFLite Micro Overview and Get Started | https://www.tensorflow.org/lite/microcontrollers |
| TFLite Micro | TFLM GitHub Repository | https://github.com/tensorflow/tflite-micro |
| TFLite Micro | Person Detection Example (reference for arena sizing) | https://github.com/tensorflow/tflite-micro/tree/main/tensorflow/lite/micro/examples/person_detection |
| PlatformIO | PlatformIO ESP32 Platform Documentation | https://docs.platformio.org/en/latest/platforms/espressif32.html |

---

## Regulatory

| Topic | Document | Link |
|---|---|---|
| India ISM Band (865-867 MHz) | WPC/DoT Short Range Devices Guidelines | https://dot.gov.in (search: Short Range Device spectrum allocation) |
| LoRa India Frequency | The Things Network India Frequency Plan | https://www.thethingsnetwork.org/docs/lorawan/frequencies/frequency-plans/ |

---

## Background References

| Topic | Document | Notes |
|---|---|---|
| BSNL coverage gap, Arunachal Pradesh | TRAI Connectivity Report, Northeast India | Cited in project rationale; confirms official absence of coverage in Dibang Valley and Upper Siang districts |
| AP Police VHF/HF Network | Arunachal Pradesh Police Department (public statements, state assembly records) | Legitimacy anchor: police relay network already exists because BSNL is inadequate |
| LoRa Forest Penetration | "LoRa Signal Propagation in Forested Environments" (various IEEE papers) | See https://ieeexplore.ieee.org; search term: LoRa forest propagation path loss |

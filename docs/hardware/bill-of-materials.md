# Bill of Materials

**Status: Design stage. No components have been ordered. All items listed as "Not yet ordered."**

Prices are indicative estimates based on listings observed on Robu.in, Thingbits.in, and Amazon India as of September 2026. Actual prices at time of purchase may differ. Quantities listed are for a 2-node bench prototype. Scale quantities accordingly for field pilot.

---

## Core Components

| Part | Purpose | Qty (bench) | Price Est. (INR each) | Total Est. (INR) | Source / Reference | Status |
|---|---|---|---|---|---|---|
| SX1262 LoRa module, SPI interface (e.g., Waveshare SX1262 LoRa HAT or equivalent bare module, NOT EBYTE E22 UART variant) | LoRa radio transceiver for mesh communication, Meshtastic-compatible | 2 | 900 | 1800 | [Waveshare SX1262](https://www.waveshare.com/sx1262-lorawan-node.htm) / Robu.in search: "SX1262 SPI" | Not yet ordered |
| SMA antenna, 868 MHz, 3 dBi (pigtail + antenna, matched to SX1262 connector) | Antenna for 865-867 MHz ISM band transmission | 2 | 200 | 400 | Robu.in / local RF supplier | Not yet ordered |
| ESP32-WROOM-32 development board (38-pin) | Sensing node MCU: dual-core 240 MHz, 520 KB SRAM (module exposes 320 KB), SPI/UART/ADC | 2 | 450 | 900 | [Robu.in ESP32 DevKit](https://robu.in/product/esp32-devkitc-v4/) | Not yet ordered |
| ESP32-C3 Mini development board | Relay-only node MCU: lighter footprint, single-core RISC-V, sufficient for radio relay without sensor stack | 1 | 350 | 350 | [Robu.in ESP32-C3](https://robu.in/product/esp32-c3-mini-development-board/) | Not yet ordered |
| NEO-6M GPS module (with ceramic patch antenna) | GPS location tagging for each node packet, UART interface | 2 | 350 | 700 | [Robu.in NEO-6M](https://robu.in/product/ublox-neo-6m-gps-module/) | Not yet ordered |
| MQ-2 gas/smoke sensor module | Analog output, detects LPG, CO, smoke; primary fire/gas hazard sensor | 2 | 120 | 240 | [Robu.in MQ-2](https://robu.in/product/mq-2-gas-sensor-module/) | Not yet ordered |
| Analog water-level sensor (resistive strip, 5 cm or 10 cm) | Flood water-rise detection, analog output on ADC pin | 2 | 80 | 160 | Robu.in / Amazon India | Not yet ordered |

## Power System

| Part | Purpose | Qty (bench) | Price Est. (INR each) | Total Est. (INR) | Source / Reference | Status |
|---|---|---|---|---|---|---|
| Solar panel, 6V 2W (or 6V 3W) | Primary energy source for outdoor nodes | 1 | 300 | 300 | Robu.in / Amazon India | Not yet ordered |
| LiFePO4 cell, 3.2V 6000 mAh (single cell, or 2 x 3000 mAh) | Primary energy storage, stable chemistry, safe for outdoor enclosure, targeting 7-day no-sun autonomy | 1 | 700 | 700 | Amazon India / EVE/CATL cells via local supplier | Not yet ordered |
| MPPT charge controller (with explicit LiFePO4 support, adjustable charge voltage to 3.65 V) | Solar charge management. Must support LiFePO4 charge profile, NOT Li-ion profile (see known-unknowns.md risk #7) | 1 | 450 | 450 | Amazon India / search "MPPT LiFePO4 solar charger 3.2V" | Not yet ordered |
| TP4056 Li-ion charger module (for bench testing only) | Bench power supply stand-in while MPPT is not set up. NOTE: TP4056 charges to 4.2V (Li-ion profile), do NOT use it with LiFePO4 cells in the field | 2 | 60 | 120 | Robu.in / Amazon India | Not yet ordered |

## Estimated Bench Prototype Total

| Category | Estimated Cost (INR) |
|---|---|
| Core components (2 nodes) | ~4,550 |
| Power system | ~1,570 |
| Miscellaneous (jumper wires, breadboard, enclosure, resistors, capacitors) | ~500 |
| **Total bench estimate** | **~6,620** |

This is a rough order-of-magnitude estimate for budgeting purposes. It does not include shipping, import duties (if applicable), or tools.

---

## Notes

1. The SX1262 SPI module must be distinguished from UART/AT-command modules sold under the EBYTE E22 label. The SPI variant is required for direct RadioLib/Meshtastic integration. Confirm before purchasing.
2. The TP4056 module is listed for bench convenience only. It will overcharge a LiFePO4 cell to 4.2 V. Never use it as the field power supply for LiFePO4 cells.
3. All GPS modules in the NEO-6M family use 3.3 V logic, matching the ESP32 GPIO voltage. No level shifting required.
4. MQ-2 heater current is approximately 150-300 mA at 5V. The ESP32 3.3V rail cannot supply this reliably. The MQ-2 heater pin must be connected to 5V/VIN, not 3.3V. See pin-mapping.md for detail.

# Pin Mapping: ESP32-WROOM-32 (Sensing Node)

**Status: Design stage. No physical board has been wired. This mapping has not been validated against real hardware.**

All GPIO assignments below are planned. The mapping follows the conventions used by RadioLib (for SX1262) and the TinyGPS++ / NeoGPS library (for NEO-6M). Before wiring hardware, cross-reference this table against the exact library constructor arguments in use.

---

## SX1262 LoRa Transceiver (SPI Interface)

The SX1262 uses standard SPI plus three additional control lines: NSS (chip select), RESET, BUSY (radio busy indicator), and DIO1 (interrupt output from radio).

| Signal | ESP32 GPIO | Pin Label | Notes |
|---|---|---|---|
| SCK (SPI clock) | GPIO18 | IO18 | Hardware SPI VSPI SCK |
| MISO (SPI data in) | GPIO19 | IO19 | Hardware SPI VSPI MISO |
| MOSI (SPI data out) | GPIO23 | IO23 | Hardware SPI VSPI MOSI |
| NSS (chip select, active low) | GPIO5 | IO5 | Software-controlled CS, held high when not transmitting |
| RESET (active low) | GPIO14 | IO14 | Pulled high externally via 10K resistor |
| BUSY | GPIO27 | IO27 | Input from SX1262; wait for LOW before issuing SPI commands |
| DIO1 (interrupt) | GPIO26 | IO26 | Interrupt line from SX1262, signals TX done / RX done / timeout |

RadioLib SX1262 constructor call (design intent, not yet compiled):

```cpp
SX1262 radio = new Module(5, 26, 14, 27);
// Module(NSS, DIO1, RESET, BUSY)
```

---

## NEO-6M GPS Module (UART)

The NEO-6M communicates over UART at 9600 baud by default. The ESP32-WROOM-32 has three hardware UART controllers. UART2 is used here to avoid conflict with the programming UART (UART0) and to leave UART1 available if needed.

| Signal | ESP32 GPIO | UART | Notes |
|---|---|---|---|
| RX (ESP32 receives GPS TX) | GPIO16 | UART2 RX2 | Connect to NEO-6M TX pin |
| TX (ESP32 transmits to GPS) | GPIO17 | UART2 TX2 | Connect to NEO-6M RX pin (rarely used, GPS typically read-only) |

Power: NEO-6M VCC to 3.3V. Logic is 3.3V compatible, no level shifting needed.

---

## MQ-2 Gas/Smoke Sensor (Analog)

GPIO34 and GPIO35 on the ESP32-WROOM-32 are input-only ADC pins with no internal pull-up/pull-down. They are suitable for analog sensor reads.

| Signal | ESP32 GPIO | Notes |
|---|---|---|
| MQ-2 analog output (AOUT) | GPIO34 | ADC1 channel 6; input-only, 12-bit ADC, 0-3.3V range |

**IMPORTANT: MQ-2 heater power.** The MQ-2 internal heater draws approximately 150-300 mA at 5V (0.75-1.5 W). The ESP32 3.3V LDO regulator cannot supply this current reliably and will either droop or shut down. Connect the MQ-2 VCC (heater) pin to the 5V/VIN rail of the development board (or directly to battery positive through the MPPT output), not to the 3.3V rail. The AOUT signal is still read on the 3.3V-referenced ADC pin. Confirm whether your specific MQ-2 module includes a voltage divider on the AOUT line before connecting to the ADC.

---

## Analog Water-Level Sensor

| Signal | ESP32 GPIO | Notes |
|---|---|---|
| Water sensor analog output | GPIO35 | ADC1 channel 7; input-only. Sensor output varies from 0V (no water contact) toward VCC as submersion depth increases |

Water sensor VCC: 3.3V is acceptable for most resistive strip water sensors. Confirm datasheet of specific sensor purchased.

---

## Deliberately Avoided GPIOs

The following GPIOs are avoided in this pin assignment:

| GPIO | Reason Avoided |
|---|---|
| GPIO0 | Boot strapping: must be HIGH during normal boot, LOW for flash mode. Avoid using as a general output |
| GPIO2 | Boot strapping: must be LOW (or floating) during upload. Often connected to onboard LED on dev boards |
| GPIO12 | Boot strapping: controls flash voltage (MTDI). Setting HIGH on boot configures 1.8V flash, which bricks boards expecting 3.3V flash. Do not drive HIGH at boot |
| GPIO15 | Boot strapping: controls boot log output. Pulling LOW silences the ROM log |
| GPIO6-11 | Connected to the internal flash SPI bus on WROOM-32 modules. Using these as GPIOs corrupts flash access and causes crashes |

---

## Summary Diagram (Text)

```
ESP32-WROOM-32
  GPIO18 ----[SCK ]----> SX1262
  GPIO19 ----[MISO]<---- SX1262
  GPIO23 ----[MOSI]----> SX1262
  GPIO5  ----[NSS ]----> SX1262
  GPIO14 ----[RST ]----> SX1262
  GPIO27 ----[BUSY]<---- SX1262
  GPIO26 ----[DIO1]<---- SX1262

  GPIO16 ----[RX2 ]<---- NEO-6M TX
  GPIO17 ----[TX2 ]----> NEO-6M RX

  GPIO34 ----[ADC ]<---- MQ-2 AOUT
  GPIO35 ----[ADC ]<---- Water sensor AOUT

  VIN/5V ----------------> MQ-2 VCC (heater)
  3.3V  -----------------> NEO-6M VCC, Water sensor VCC, SX1262 VCC
  GND   -----------------> All sensor GNDs
```

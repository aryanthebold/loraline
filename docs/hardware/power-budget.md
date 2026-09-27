# Power Budget: Sensing Node (ESP32-WROOM-32 + SX1262 + Sensors)

**Status: Design stage. All figures below are ESTIMATED from component datasheets and published application notes. None have been measured on real hardware. Real figures will replace these in Phase 2 after bench power profiling.**

---

## Component Current Draw Estimates

All figures are per-component and at the specified operating condition. Actual board-level draw will differ due to regulator efficiency losses and parasitic loads.

| Component | Operating State | Current Draw (est.) | Source |
|---|---|---|---|
| ESP32-WROOM-32 | Active (both cores, 240 MHz, WiFi OFF, BT OFF) | 80 mA | Espressif ESP32 datasheet Table 8 |
| ESP32-WROOM-32 | Modem sleep (one core, 80 MHz, radio off) | 20 mA | Espressif ESP32 datasheet |
| ESP32-WROOM-32 | Deep sleep | 0.01 mA (10 uA) | Espressif ESP32 datasheet |
| SX1262 | RX mode (listening) | 4.6 mA | Semtech SX1262 datasheet Table 4 |
| SX1262 | TX +22 dBm | 118 mA | Semtech SX1262 datasheet Table 4 |
| SX1262 | TX +14 dBm | 44 mA | Semtech SX1262 datasheet (lower power mode) |
| SX1262 | Sleep | 0.0005 mA | Semtech SX1262 datasheet |
| MQ-2 heater | Heater active (on 5V) | 150-300 mA | MQ-2 datasheet; wide range due to unit variation |
| MQ-2 heater | Heater off (duty cycled) | 0 mA | |
| NEO-6M GPS | Acquisition (cold start, tracking) | 45 mA | u-blox NEO-6M datasheet Table 16 |
| NEO-6M GPS | Tracking (fix acquired, continuous) | 36 mA | u-blox NEO-6M datasheet |
| NEO-6M GPS | Power save mode | 11 mA | u-blox NEO-6M datasheet |
| NEO-6M GPS | Backup (RTC only) | 0.015 mA | u-blox NEO-6M datasheet |
| Analog water sensor | Powered on, reading | 2 mA | Estimated from typical resistive strip sensors |
| Voltage regulator / board losses | Assumed 15% overhead | Proportional | Industry rule of thumb for LDO at light load |

---

## Operating Mode Definition

The node cycles through three states:

**SLEEP** (majority of time): ESP32 in deep sleep, SX1262 in sleep, MQ-2 heater off, GPS in backup mode. Wake via RTC timer every N seconds (N to be configured, initial design: 60 seconds for telemetry, immediate wake on interrupt from any sensor threshold).

**SENSE** (brief, periodic): ESP32 active, ADC reads from MQ-2 and water sensor, TFLite Micro inference. MQ-2 requires a preheat period (approximately 20-60 seconds from cold). GPS ping for location tag. Duration: approximately 30-90 seconds per cycle (dominated by MQ-2 preheat).

**TRANSMIT** (on alert or scheduled telemetry): SX1262 TX burst. SF12/125 kHz BW, a 20-byte packet takes approximately 1.5 seconds to transmit. Duration: 2-5 seconds including RadioLib overhead.

---

## Average Current Draw Estimate

Duty cycle assumptions (estimated, conservative):

- SLEEP fraction: 90% of time
- SENSE fraction: 9.5% of time (one 57-second sense cycle per 10-minute window)
- TRANSMIT fraction: 0.5% of time (one 3-second TX burst per 10-minute window)

| Mode | Dominant components | Current (est.) | Fraction | Weighted current |
|---|---|---|---|---|
| SLEEP | ESP32 deep sleep + SX1262 sleep + GPS backup | ~0.025 mA | 0.90 | 0.023 mA |
| SENSE | ESP32 active + MQ-2 heater + GPS tracking + water sensor | ~200 mA (MQ-2 dominates) | 0.095 | 19.0 mA |
| TRANSMIT | ESP32 active + SX1262 TX +14 dBm | ~124 mA | 0.005 | 0.62 mA |
| **Weighted average** | | | | **~19.6 mA** |

Note: The MQ-2 heater is the dominant power consumer. If MQ-2 heater duty is reduced (e.g., heat for 20 seconds per sense cycle rather than the full cycle), the weighted average drops substantially. A 50% heater duty cycle at 200 mA across 9.5% of time gives approximately 9.5 mA weighted. **This is the most effective single optimization for power reduction and should be evaluated in Phase 2.**

---

## Peak Current

Peak draw occurs during a SENSE+TRANSMIT overlap (ESP32 active + MQ-2 heater + SX1262 TX +14 dBm):

```
80 mA (ESP32) + 225 mA (MQ-2 heater, worst case) + 44 mA (SX1262 TX +14 dBm) = ~349 mA peak
```

This peak draw must be supplied by the LiFePO4 cell. LiFePO4 cells are well-suited to this (high continuous discharge current capability). The battery internal resistance at this current will cause a voltage sag, which must stay above the ESP32 brownout threshold (approximately 2.7V at the LDO input). Battery selection should confirm the cell can supply 350 mA continuously without significant sag.

---

## Autonomy Estimate (No Solar Input)

**Target battery: LiFePO4, 3.2V nominal, 6000 mAh (19.2 Wh)**

Using the weighted average current of 19.6 mA:

```
Autonomy = Capacity / Average current
         = 6000 mAh / 19.6 mA
         = ~306 hours
         = ~12.8 days
```

Using the optimistic case (MQ-2 duty-cycled to 50%):

```
Average current ~10 mA (rough)
Autonomy = 6000 / 10 = 600 hours = ~25 days
```

**This is an estimate based on datasheet figures. Real autonomy will be lower due to:**
- Regulator efficiency losses (~15% overhead not fully accounted above)
- Temperature effects on LiFePO4 capacity (capacity drops at low temperatures; Mechuka and Anini can reach 0-5 degrees C in winter)
- Self-discharge of the cell (~2-3% per month, negligible for weeks-long deployment)
- Actual MQ-2 heater current (may be higher than the 200 mA midpoint estimate)

**Conservative realistic estimate: 7-10 days no-sun autonomy with MQ-2 heater duty cycling enabled.**

The 7-day target stated in the project overview is considered achievable based on this estimate, but this must be confirmed by bench measurement in Phase 2.

---

## Solar Charging

A 6V 2W solar panel under good sun (India insolation at 3-4 peak-sun-hours per day) generates approximately:

```
2W / 3.2V (LiFePO4 charge voltage) = ~0.625 A MPPT output current
Over 4 peak-sun-hours: 0.625 A * 4 h = 2.5 Ah per day replenished
```

Daily consumption at 19.6 mA average: `19.6 mA * 24 h = 470 mAh per day`

Solar input (2.5 Ah) significantly exceeds daily consumption (0.47 Ah) under good conditions. The system should maintain charge indefinitely during dry season. Monsoon cloud cover may reduce solar input by 70-90%, bringing effective input down to 0.25-0.75 Ah per day, which still exceeds daily consumption. However, multi-day overcast periods (possible during heavy monsoon) will draw down the battery. The 7-day no-sun autonomy target provides the buffer for sustained overcast periods.

See [known-unknowns.md](../architecture/known-unknowns.md) risk #6 for the autonomy risk caveat.

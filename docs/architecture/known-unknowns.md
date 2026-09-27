# Known Unknowns

**Status: Design stage. This document catalogs technical risks that have not yet been resolved by field measurement or prototyping.**

These are not defects in the design. They are open questions that require real hardware and field testing to answer. Recording them explicitly prevents the team from treating estimates as facts. Each item notes the current basis for the assumption and what evidence is needed to retire the risk.

---

## Radio and Mesh

### 1. LoRa range in dense forest canopy is unvalidated

**Current basis:** SX1262 datasheet specifications and published LoRa deployment studies in open terrain or urban environments. Quoted typical range for 865 MHz LoRa at SF12/125 kHz BW is 5-15 km in line-of-sight conditions.

**The risk:** Dibang Valley and Mechuka are heavily forested. Dense tree cover causes significant signal attenuation (published figures suggest 15-30 dB extra loss compared to open terrain). The effective range between nodes may be 200-800 m in worst-case canopy, which changes the required node density substantially.

**What resolves it:** Field measurement with two SX1262 nodes (no sensors needed) walking a transect through representative terrain. Target: Phase 2 (bench prototype complete, first field trip).

---

### 2. Mesh self-healing convergence time is estimated, not simulated

**Current basis:** The flooding-with-deduplication protocol (similar to Meshtastic) is well-documented and convergence behavior is known theoretically. For a 15-20 node network, reconvergence after a node failure should be sub-second for networks this small.

**The risk:** Real-world timing depends on channel congestion, node duty cycle constraints (India ISM band allows approximately 1% duty cycle at higher EIRP or up to 10% at reduced power), and the actual radio latency of SX1262 transmit/receive cycles.

**What resolves it:** Software simulation of the mesh protocol first (no hardware needed), then bench test with 3 nodes.

---

### 3. Meshtastic SPI pin compatibility needs explicit verification

**Current basis:** Meshtastic documentation lists default SPI pins for ESP32-WROOM-32 that mostly align with the pin mapping in `docs/hardware/pin-mapping.md`. However, Meshtastic's pin definitions can vary by board variant and firmware version.

**The risk:** If the SX1262 SPI wiring does not match what the Meshtastic firmware or RadioLib library expects for the target board, the radio will not initialize and no link will form.

**What resolves it:** Cross-checking `pin-mapping.md` against the exact RadioLib SX1262 constructor arguments before hardware is ordered. This can be done with documentation alone, before physical components arrive. See GitHub Issue #2.

---

## TinyML and Sensor Fusion

### 4. TinyML model accuracy on real sensor noise is unvalidated

**Current basis:** The model architecture (a small 1D CNN or MLP, see `ml-model/model-architecture.md`) is designed based on published TFLite Micro examples for sensor classification. The expected accuracy on clean simulated data is tractable.

**The risk:** MQ-2 sensors are known to have significant unit-to-unit variation and are sensitive to temperature and humidity. In a high-humidity monsoon environment at 2000-3000 m elevation, the baseline resistance of the sensor element will drift. A model trained on one unit's readings may not generalize to another unit or to seasonal variation.

**What resolves it:** Collecting a small labeled dataset from at least 2 MQ-2 units under varied humidity/temperature conditions and evaluating inference accuracy. Target: Phase 2.

### 5. Tensor arena size fit in ESP32-WROOM-32 RAM is estimated

**Current basis:** The ESP32-WROOM-32 has 320 KB of SRAM. TFLite Micro examples for comparable sensor classifiers report tensor arenas of 20-80 KB. The target is to stay under 80 KB, leaving headroom for mesh buffers and stack.

**The risk:** Actual arena size depends on the specific model topology and number of layers chosen. A model that is accurate enough may require more memory than fits alongside the LoRa mesh buffers.

**What resolves it:** Compiling the model with TFLite Micro and measuring the required tensor arena size. This can be done in simulation before hardware arrives. See GitHub Issue #1.

---

## Power and Solar

### 6. Solar autonomy during monsoon cloud cover is estimated

**Current basis:** The power budget in `docs/hardware/power-budget.md` calculates autonomy based on a stated LiFePO4 capacity and estimated average current draw. The 7-day no-sun autonomy target is derived from this calculation.

**The risk:** The estimate uses an average duty-cycle current draw. In practice, alert bursts cause higher peak current, and the LoRa transmit burst (approximately 120 mA peak) creates brief voltage sag that could trigger an ESP32 brownout reset if the battery internal resistance is higher than expected.

**What resolves it:** Bench measurement of actual current draw on real hardware (ESP32 + SX1262, then full sensor stack). Target: Phase 2. Real figures will replace estimates in the power budget table once measured.

### 7. MPPT controller compatibility with the chosen solar panel and LiFePO4 chemistry is unconfirmed

**Current basis:** The BOM lists an MPPT controller. LiFePO4 requires a specific charge profile (constant current to approximately 3.65 V/cell, then constant voltage, no trickle charge). Not all MPPT modules sold on Indian electronics marketplaces explicitly support LiFePO4 chemistry.

**The risk:** Using a controller programmed for Li-ion (4.2 V/cell) on a LiFePO4 cell will either overcharge the cell (degrading it) or the BMS will cut off charging prematurely.

**What resolves it:** Selecting an MPPT controller with explicit LiFePO4 support (adjustable charge voltage setpoint), or pairing with an external BMS. This is a procurement decision that must be resolved before ordering.

---

## Deployment and Logistics

### 8. Physical mounting in terrain is unplanned

**Current basis:** The system design assumes nodes can be mounted on elevated structures (rooftops, poles) in each village. No site survey has been conducted.

**The risk:** Suitable mounting locations may not exist, requiring pole installation. Villager cooperation for installation and maintenance is assumed but not confirmed.

**What resolves it:** A site visit to at least one target village. This is outside the scope of Phase 1 (bench prototype) but must be planned before Phase 3 (field pilot).

### 9. GPS fix reliability at elevation is assumed, not tested

**Current basis:** NEO-6M is a standard GPS receiver. Published cold-start acquisition time is approximately 30 seconds in open sky.

**The risk:** In narrow valleys surrounded by ridges (typical terrain in Dibang Valley), GPS sky view is partially blocked. Cold-start acquisition may take longer or fail. Fix accuracy may also be reduced.

**What resolves it:** Testing a NEO-6M in the field or in a comparable valley terrain environment.

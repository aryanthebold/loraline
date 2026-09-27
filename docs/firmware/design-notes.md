# Firmware Design Notes

**Status: Design stage. No firmware has been compiled or run on physical hardware.**

This document describes the planned firmware structure, module responsibilities, build environment, and key design decisions for the sensing node (ESP32-WROOM-32) and relay node (ESP32-C3 Mini).

---

## Build Environment

- **Framework:** Arduino (via PlatformIO). Chosen over ESP-IDF directly because RadioLib, TensorFlow Lite Micro, and TinyGPS++ all have Arduino-compatible wrappers and this reduces integration effort at the prototype stage.
- **Build tool:** PlatformIO (CLI + VS Code extension). PlatformIO manages ESP32 board support packages and library dependencies declaratively via `platformio.ini`.
- **Target boards:**
  - Sensing node: `espressif32` board, `esp32dev` variant (covers ESP32-WROOM-32 dev boards)
  - Relay node: `espressif32c3` board (ESP32-C3 Mini)

A `platformio.ini` file will be added to each firmware subdirectory when the build environment is validated.

---

## Module Breakdown

The sensing node firmware is structured as four cooperating modules. Each module is defined in a header file under `firmware/sensing-node/src/`. The modules are intended to be independently testable where possible.

### sensor_reader.h

Responsible for: initializing ADC pins, reading MQ-2 analog output, reading water level analog output, applying a simple moving average to reduce ADC noise, and exposing a `SensorReading` struct to the caller.

Dependencies: ESP32 Arduino ADC API (`analogRead`). No external library needed for basic ADC reads.

Open question: whether to use `analogReadMilliVolts()` (available in ESP32 Arduino core 2.x) for better calibrated readings, or raw `analogRead()` with manual scaling. The ESP32 ADC is known to have significant non-linearity near rail voltages.

### tinyml_inference.h

Responsible for: loading a flatbuffer TFLite model, setting up the tensor arena, running inference on a `SensorReading` input, and returning a `HazardClass` enum.

The actual trained model (.tflite file converted to a C array) is not yet available. The function stubs return a hardcoded NOMINAL result. See `ml-model/model-architecture.md` for the planned model topology.

TFLite Micro integration note: The Arduino library for TFLite Micro is `tflite-micro` (available on PlatformIO). The tensor arena must be statically allocated. The current placeholder size is 40*1024 bytes (40 KB), which is conservative. Actual required size depends on the model topology.

### lora_mesh.h

Responsible for: initializing the SX1262 via RadioLib, constructing `MeshPacket` structs, transmitting packets, receiving packets, and maintaining a seen-ID buffer for duplicate suppression.

The radio transport layer is currently mocked (see `lora_mesh.h` source) to allow the mesh routing logic to be reasoned about without real hardware. The mock will be replaced by actual `RadioLib::SX1262` calls when hardware is available.

Library: RadioLib (JGROMES/RadioLib on PlatformIO). This library supports SX1262 SPI natively and is used by some Meshtastic implementations. The Meshtastic codebase itself is a reference for SX1262 initialization parameters.

### priority_router.h

Responsible for: given a queued set of outbound `MeshPacket` items and a set of available transport paths, deciding the transmission order and transport selection.

This module has NO hardware dependency. It operates purely on packet metadata (priority tag, destination) and path descriptors (path type, availability flag). It is fully unit-testable without an ESP32. Three to four unit tests are included in `priority_router.cpp`.

See the source file for the full priority logic and test cases.

---

## Sensing Loop Design

The intended control flow for the sensing node is:

```
Boot
  |
  +-- Initialize all modules (sensor_reader, tinyml_inference, lora_mesh)
  |
  +-- Loop:
        |
        +-- [Every 60 seconds or on external interrupt]
        |     |
        |     +-- Preheat MQ-2 heater (20-60 seconds, configurable)
        |     +-- Read sensors -> SensorReading struct
        |     +-- Run TFLite inference -> HazardClass
        |     +-- If HazardClass != NOMINAL: build SOS/ALERT packet, push to transmit queue
        |     +-- If scheduled telemetry time: build ROUTINE packet, push to transmit queue
        |
        +-- [Transmit queue not empty]
        |     |
        |     +-- priority_router selects next packet and transport
        |     +-- lora_mesh.transmit(packet)
        |
        +-- [No pending work]
              |
              +-- Enter deep sleep until next RTC wakeup or sensor interrupt
```

This is a design sketch. The actual implementation using FreeRTOS tasks (available on ESP32 Arduino) may split sensing and radio into separate tasks. That decision is deferred to the prototyping phase.

---

## Relay Node Firmware

The relay node (ESP32-C3 Mini) runs a simplified firmware with no sensor modules and no TFLite inference. It only runs `lora_mesh.h` and `priority_router.h`. Its loop is:

```
Boot
  |
  +-- Initialize lora_mesh (SX1262 SPI)
  |
  +-- Loop:
        |
        +-- Listen for incoming packets
        +-- On receive: check seen-ID buffer, if new, push to retransmit queue
        +-- priority_router selects next packet
        +-- Retransmit
```

The relay node design is in `firmware/relay-node/`.

---

## Naming and Coding Conventions

- All source files use `.h` / `.cpp` extension pairs.
- Structs are named in `PascalCase` (`MeshPacket`, `SensorReading`).
- Functions are named in `snake_case` (`read_sensors`, `run_inference`).
- Compile-time configuration constants are defined as `#define` macros in a shared `config.h` (not yet created; will be added in Phase 2).
- All files carry the UNTESTED status banner at the top. Update the banner when testing status changes.

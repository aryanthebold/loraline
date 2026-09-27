# Sensing Node Firmware

**Status: Design stage. No code has been compiled or run on physical hardware.**

This directory contains the firmware for sensing nodes (ESP32-WROOM-32 + SX1262 + MQ-2 + water sensor + NEO-6M GPS).

## Module Overview

| File | Responsibility | Has Hardware Dependency? |
|---|---|---|
| `src/main.cpp` | Wires all modules together, Arduino setup/loop | Yes (Arduino framework), guarded by `#ifdef ARDUINO` |
| `src/sensor_reader.h` | ADC reads from MQ-2 and water sensor | Yes (ESP32 analogRead), currently stubbed |
| `src/tinyml_inference.h` | TFLite Micro inference on sensor features | Yes (TFLite Micro library), currently stubbed |
| `src/lora_mesh.h` | LoRa packet structure and mesh relay protocol | Yes (RadioLib SX1262), currently mocked |
| `src/priority_router.h/.cpp` | Packet escalation order decision | NO hardware dependency, unit testable on host |

## Running the Priority Router Unit Tests (Host Machine, No Hardware Needed)

The priority router can be tested without any ESP32 or LoRa hardware:

```bash
# Linux / macOS / WSL
g++ -std=c++17 -DRUN_PRIORITY_ROUTER_TESTS src/priority_router.cpp -o router_test
./router_test

# Windows (MinGW g++)
g++ -std=c++17 -DRUN_PRIORITY_ROUTER_TESTS src\priority_router.cpp -o router_test.exe
.\router_test.exe
```

Expected output: 4 tests, all PASS.

## Build Environment (Phase 2 Target)

- PlatformIO CLI + VS Code extension
- Platform: `espressif32`
- Board: `esp32dev` (covers ESP32-WROOM-32 DevKit v4)
- Framework: `arduino`

A `platformio.ini` will be added in Phase 2 when the build is first attempted against real hardware.

## What Has Not Been Done

- No `platformio.ini` (build system not set up yet)
- `sensor_reader.h` stubs not replaced with real ADC calls
- `tinyml_inference.h` stub not replaced with TFLite Micro model
- `lora_mesh.h` mock radio not replaced with RadioLib SX1262 calls
- GPS integration (NEO-6M, TinyGPS++ or NeoGPS library) not started
- Deep sleep / RTC wakeup not implemented

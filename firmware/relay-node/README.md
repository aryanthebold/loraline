# Relay Node Firmware

**Status: Design stage. No code has been compiled or run on physical hardware.**

This directory contains the firmware for relay-only nodes (ESP32-C3 Mini + SX1262).

Relay nodes have no sensors and run no TFLite inference. They only receive incoming LoRa mesh packets and re-broadcast them in priority order toward the nearest gateway.

## Module Dependencies

The relay node reuses `lora_mesh.h` and `priority_router.h` from the sensing node. In a final PlatformIO project structure, these shared modules would live in a `lib/` directory accessible to both firmware targets. For now they are referenced by relative path.

## Key Differences from Sensing Node

| Feature | Sensing Node (WROOM-32) | Relay Node (C3 Mini) |
|---|---|---|
| Sensors | MQ-2 + water level + GPS | None |
| TFLite inference | Yes (stub) | No |
| LoRa mesh | Yes | Yes |
| Priority router | Yes | Yes |
| MCU | ESP32-WROOM-32 | ESP32-C3 Mini |

## Pin Mapping (C3 Mini)

The ESP32-C3 Mini has different GPIO numbers from the WROOM-32. The SX1262 SPI pin assignment for the C3 Mini has not been documented yet. This will be added to `docs/hardware/pin-mapping.md` in Phase 2 when the C3 Mini is procured and its pinout is confirmed.

## What Has Not Been Done

- Pin mapping for ESP32-C3 Mini + SX1262 not finalized
- `platformio.ini` not written
- Mock radio in `lora_mesh.h` not replaced with real RadioLib calls
- Deep sleep / wakeup not implemented

# System Architecture

**Status: Design stage. No hardware has been built or tested.**

This document describes the intended architecture of LoRaLine as designed for the Smart India Hackathon 2026, Problem Statement 26178 (Qualcomm, Hardware, Disaster Management). Figures, ranges, and topology descriptions are derived from component datasheets and published literature on LoRa deployments. None of this has been validated on real hardware in the target terrain.

---

## Problem Context

Dibang Valley (Anini), Upper Siang (Mechuka), and the Yibuk/Dogong Banggo cluster in Arunachal Pradesh have effectively zero BSNL cellular coverage. The Arunachal Pradesh Police operate a VHF/HF wireless relay network across dozens of stations precisely because commercial cellular is inadequate in these regions. When a flood, landslide, fire, or gas leak occurs in these villages, there is no standard civilian channel for alerting residents or emergency responders until cellular connectivity is restored (which may take hours to days after a disaster event).

Secondary comparison deployment cases studied during design: Lahaul-Spiti (Himachal Pradesh) and the Andaman and Nicobar Islands.

---

## Two-Layer Architecture

### Layer 1: On-Device Edge-AI Hazard Sensing

Each sensing node runs a TinyML sensor-fusion classifier compiled with TensorFlow Lite Micro. The classifier is quantized to int8 to fit within the ESP32-WROOM-32 RAM budget (320 KB SRAM total, targeting a tensor arena of 40-80 KB). The model reads three sensor channels:

- MQ-2 analog output (gas/smoke concentration, ADC read on GPIO34)
- Analog water-level sensor (ADC read on GPIO35)
- Derived feature: rate of change of water level over a rolling window

The classifier outputs one of four states:

| Output Class | Description |
|---|---|
| `NOMINAL` | All readings within normal bounds |
| `FLOOD_RISING` | Water level above threshold or rate-of-rise above threshold |
| `FIRE_SMOKE` | MQ-2 output above smoke/CO signature threshold |
| `POLLUTION_SPIKE` | Sustained elevated gas reading below fire threshold |

The model is described in [ml-model/model-architecture.md](../../ml-model/model-architecture.md). No trained model exists yet. Training data collection is a Phase 1 task.

Inference runs entirely on the microcontroller. No cloud call is made for the hazard decision. This is deliberate: the system must function when there is no connectivity at all.

### Layer 2: Decentralized LoRa Mesh

When the on-device classifier outputs a non-NOMINAL state above a configurable confidence threshold, the sensing node packages a priority-tagged alert packet and transmits it over the 865-867 MHz ISM band using the SX1262 LoRa transceiver (SPI interface).

The mesh protocol uses a store-and-forward hop-by-hop relay model. Each relay node (ESP32-C3 Mini + SX1262) receives a packet, checks whether it has already seen this packet ID (duplicate suppression via a rolling seen-ID buffer), and if it is a new packet, re-broadcasts it. The priority router (`priority_router.h`) determines escalation order when multiple packets are queued: SOS-class packets (FLOOD_RISING, FIRE_SMOKE) always transmit before ROUTINE telemetry regardless of arrival order.

The mesh does not require a pre-configured static routing table. It is a flooding-with-deduplication mesh, similar in concept to Meshtastic. The SX1262 module is chosen for Meshtastic compatibility, which gives a validated reference implementation of this protocol on the same radio hardware.

---

## Packet Structure (Design)

```
[ Packet ID (4B) | Source Node ID (2B) | Hop Count (1B) | Priority (1B) |
  Timestamp (4B) | Payload Type (1B) | Payload (variable, max ~220B) |
  CRC (2B) ]
```

Priority byte values:

- `0x01` = SOS (life-safety hazard, immediate relay)
- `0x02` = ALERT (elevated reading, relay on next available slot)
- `0x03` = ROUTINE (telemetry ping, relay only when channel is idle)

The CRC covers all fields from Packet ID through Payload. Nodes that receive a packet with a failed CRC drop it silently.

---

## Deployment Topology (Arunachal Pradesh, Design Intent)

Target: 15-20 nodes across 3-5 villages in a cluster, with one gateway node per cluster. The gateway connects to a satellite uplink (pending budget) or stores locally until a periodic mobile data connection becomes available. The gateway is also an ESP32-WROOM-32 but with a GSM/satellite modem in addition to the SX1262.

Sensing nodes are solar-powered with LiFePO4 batteries targeting 7 or more days of autonomy without sunlight (see [power-budget.md](../hardware/power-budget.md) for the estimate).

Relay-only nodes (ESP32-C3 Mini) are placed at elevated points (ridge lines, rooftops of government buildings) to extend mesh range. They do not carry sensors, only the SX1262 radio and GPS for location tagging.

---

## Backend and Dashboard (Design Stage)

When an alert reaches a gateway with connectivity, it is forwarded to:

- A FastAPI backend (Python) connected to a MongoDB instance storing node state, alert history, and GPS tracks.
- A React + Tailwind frontend dashboard showing a map of active nodes, alert status, and historical readings.
- Twilio for multi-channel alert broadcast (SMS to registered responders, WhatsApp fallback).

The dashboard stack does not include AWS, Docker, Kubernetes, or Jenkins. This is a deliberate choice for a 15-20 node hackathon-scale pilot. The backend runs on a single cloud VM or on-premise server at the district emergency operations center.

---

## LoRa Frequency Band

India permits ISM-band LoRa operation at 865-867 MHz under WPC guidelines (no individual license required for short-range devices under specified EIRP limits). All node firmware targets this band. The SX1262 supports this frequency range natively.

---

## References

See [docs/references/datasheets.md](../references/datasheets.md) for source links.

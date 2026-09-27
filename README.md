# LoRaLine

A distributed network of solar-powered, edge-AI sensor nodes for terrain-isolated villages in Arunachal Pradesh, designed for civilian hazard alerting when cellular infrastructure is absent or damaged.

![Status](https://img.shields.io/badge/Status-Pre--Fabrication%20%2F%20Design%20Stage-orange?style=flat-square)

---

## Project Status

**Pre-fabrication. No physical hardware has been ordered or assembled yet.**

This repository contains architecture design, hardware specifications, firmware skeletons, and planning documents. All code is design-stage and has not been run on any physical device. All cost and power figures are estimates based on datasheets and published literature, not measurements from real hardware.

Team: CHERISH | Competition: Smart India Hackathon 2026 | Problem Statement: 26178 | Category: Hardware | Theme: Disaster Management | Sponsor: Qualcomm

---

## What This Solves

Villages in districts like Dibang Valley (Anini, Mechuka) and the Yibuk/Dogong Banggo cluster in Arunachal Pradesh have no reliable cellular coverage. When a flood, fire, or gas leak occurs, there is no standard way for residents to send or receive an alert before infrastructure is restored. LoRaLine places low-power sensor nodes in these villages that detect hazards locally using an on-device classifier, then relay verified alerts hop by hop over a LoRa mesh until they reach a gateway or responder, without requiring the internet to be functioning at all.

---

## System Overview

LoRaLine operates in two layers.

The **primary layer** is on-device edge inference. Each sensing node runs a small quantized (int8) TinyML sensor-fusion classifier compiled with TensorFlow Lite Micro, targeting under 50-100 KB of flash. It reads three sensor channels (gas/smoke via MQ-2, water level via an analog sensor, and derived air quality metrics) and classifies the combined reading into one of several hazard states: flood rising, fire/smoke detected, pollution spike, or nominal. Inference happens entirely on the microcontroller with no cloud dependency.

The **secondary layer** is a decentralized LoRa mesh. When a node classifies a hazard above a threshold, it packages a priority-tagged alert packet and broadcasts it on the 865-867 MHz ISM band using an SX1262 transceiver. Neighboring nodes relay the packet hop by hop toward the nearest gateway, using a priority router that always escalates SOS-class packets ahead of routine telemetry. The mesh is self-healing: if one relay is offline, the routing layer finds an alternate path.

See [docs/architecture/system-architecture.md](docs/architecture/system-architecture.md) for the full architecture description including protocol decisions and deployment topology.

The Arunachal Pradesh Police already operate a VHF/HF wireless relay network across dozens of stations because BSNL coverage is officially acknowledged as inadequate in these areas. LoRaLine extends a similar relay concept to civilian hazard alerting and does not attempt to replace BharatNet or the police radio network.

---

## Repository Structure

| Folder | Contents | Status |
|---|---|---|
| `docs/architecture/` | System architecture, open unknowns | Design stage |
| `docs/hardware/` | Bill of materials, pin mapping, power budget | Design stage (no parts ordered) |
| `docs/firmware/` | Firmware design notes | Design stage |
| `docs/references/` | Datasheet links and citations | Ongoing |
| `firmware/sensing-node/` | ESP32-WROOM-32 firmware skeleton | Untested skeleton |
| `firmware/relay-node/` | ESP32-C3 Mini relay firmware skeleton | Untested skeleton |
| `ml-model/` | TinyML model architecture notes and training plan | Design stage |
| `dashboard/` | React + FastAPI dashboard notes | Design stage |
| `.github/ISSUE_TEMPLATE/` | Issue templates for design and hardware questions | Active |

---

## Hardware

Core components: ESP32-WROOM-32 (sensing nodes), ESP32-C3 Mini (relay-only nodes), SX1262 LoRa transceiver (SPI interface, Meshtastic-compatible), NEO-6M GPS module, MQ-2 gas/smoke sensor, analog water-level sensor, solar panel with MPPT charging, LiFePO4 battery targeting 7 or more days autonomy without sun.

See [docs/hardware/bill-of-materials.md](docs/hardware/bill-of-materials.md) for the full parts list with pricing and source links.

**No components have been ordered.** All items in the BOM have Status: Not yet ordered.

---

## Firmware

The firmware is structured around four modules: sensor reading, TinyML inference, LoRa mesh communication, and priority routing. Each module is defined in a header file with function signatures and data structures. None of these modules have been compiled or run against real hardware.

See [docs/firmware/design-notes.md](docs/firmware/design-notes.md) for rationale behind module boundaries and the build environment plan (Arduino framework via PlatformIO targeting ESP32).

**Everything in `firmware/` is untested, design-stage only.** The priority router (`priority_router.h`) includes standalone unit tests that run without any hardware dependency.

---

## Roadmap

- [ ] Component procurement (target: Phase 1)
- [ ] Two-node bench prototype (ESP32 + SX1262, no sensors, just LoRa link test)
- [ ] Field pilot, single village cluster (3-5 nodes, one gateway)
- [ ] District dashboard integration (gateway connects to FastAPI backend, alerts visible on React dashboard)

---

## Team

CHERISH (SIH 2026, Problem Statement 26178)

Members: Bhavya, Aryan, Ayush, Ravi, Meenakshi, Ayushman

Pitch materials and design documents are linked from the project board. Contact via GitHub Issues using the provided templates.

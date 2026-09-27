# PROGRESS.md

Dated changelog for LoRaLine development. Append new entries at the top in the same format.

---

## 2026-09-27

**Repository setup complete.**

- Created full directory structure: `docs/`, `firmware/`, `ml-model/`, `dashboard/`, `.github/ISSUE_TEMPLATE/`.
- Wrote `README.md` with project overview, honest status declaration, system architecture summary, hardware summary, and roadmap.
- Wrote `docs/architecture/system-architecture.md` covering the two-layer design (edge-AI sensing + LoRa mesh), deployment topology for Arunachal Pradesh, and protocol decisions.
- Wrote `docs/architecture/known-unknowns.md` listing open technical risks with no fabricated confidence levels.
- Wrote `docs/hardware/bill-of-materials.md` with full parts list. All items: Not yet ordered.
- Wrote `docs/hardware/pin-mapping.md` with full ESP32-WROOM-32 GPIO assignment for SX1262, NEO-6M, MQ-2, and water sensor.
- Wrote `docs/hardware/power-budget.md` with estimated current draw table and autonomy calculation. All figures are datasheet-based estimates, not measured values.
- Wrote `docs/firmware/design-notes.md` covering module structure, build environment plan (PlatformIO + Arduino), and TFLite Micro integration notes.
- Wrote `docs/references/datasheets.md` with links to SX1262, ESP32, NEO-6M, MQ-2, and TFLite Micro documentation.
- Wrote `ml-model/training-notes.md` and `ml-model/model-architecture.md` covering dataset plan, feature engineering, and int8 quantization target.
- Wrote `dashboard/README.md` covering React + Tailwind + FastAPI + MongoDB + Twilio stack design.
- Created firmware skeletons under `firmware/sensing-node/src/` and `firmware/relay-node/src/`: `main.cpp`, `sensor_reader.h`, `tinyml_inference.h`, `lora_mesh.h`, `priority_router.h`. All files carry the UNTESTED design-stage banner.
- Wrote standalone unit tests for `priority_router` (no hardware dependency).
- Created `.github/ISSUE_TEMPLATE/hardware-question.md` and `design-decision.md`.
- Created 6 specific GitHub Issues using the templates.
- Created GitHub Project board (Kanban) with columns: Design, Procurement, Bench Testing, Field Pilot.
- Created 3 milestones: Two-node bench prototype, Field pilot, District integration.

**Next step:** Component procurement (Phase 1). Begin with SX1262 LoRa module and ESP32-WROOM-32 development board to validate SPI wiring and Meshtastic library compatibility.

---

<!-- Add new entries above this line, newest first -->

# Dashboard

**Status: Design stage. No code has been written. No server has been provisioned.**

This directory will contain the React + Tailwind frontend source once development begins. For now it documents the planned stack and data flow.

---

## Planned Stack

| Layer | Technology | Role |
|---|---|---|
| Frontend | React (Vite), Tailwind CSS | Web dashboard: node map, alert feed, historical sensor readings |
| Backend API | FastAPI (Python) | REST endpoints for node state, alert history, GPS tracks |
| Database | MongoDB | Document store for alerts, node telemetry, and GPS logs |
| Alerts | Twilio | SMS and WhatsApp broadcast to registered emergency responders when a gateway forwards an alert |

No AWS, Docker, Kubernetes, or Jenkins. This is a 15-20 node pilot deployed on a single cloud VM or on-premise server at the district emergency operations center. The stack is sized for a hackathon-scale prototype.

---

## Key Views Planned

1. **Node map:** Leaflet.js map showing all registered nodes with last-seen time and battery level. Nodes are color-coded by status (nominal, alert, offline).
2. **Alert feed:** Chronological list of all alerts received from the mesh, with node ID, hazard class, confidence, GPS coordinates, and timestamp.
3. **Sensor history:** Time-series charts for MQ-2 and water level readings per node, last 24 hours.
4. **Node detail:** Per-node view showing firmware version, uptime, GPS track, and alert history.

---

## Alert Flow (Design)

```
LoRa Gateway node
  |
  +-- (when connectivity available)
  +-- POST /api/alert to FastAPI backend
      |
      +-- FastAPI stores alert in MongoDB
      +-- FastAPI triggers Twilio SMS/WhatsApp to registered responders
      +-- React frontend polls /api/alerts every 30 seconds (or WebSocket if implemented)
```

---

## Development Plan

Dashboard development is deferred to Phase 3 (after field pilot begins). Phase 1 and Phase 2 focus on hardware and firmware. The gateway's primary function in Phase 2 is to print received alerts to serial for inspection, not to forward them to a backend.

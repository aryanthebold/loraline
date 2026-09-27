/*
 * STATUS: UNTESTED -- DESIGN STAGE
 * This module has not been run on physical hardware. No ESP32, LoRa
 * radio, or sensors have been acquired as of 2026-09-27.
 * Logic validated only via [manual trace / not yet compiled against ESP32-C3 toolchain].
 * Update this line honestly as testing progresses.
 *
 * main.cpp -- Relay Node (ESP32-C3 Mini)
 *
 * The relay node has no sensors and no TFLite inference. Its only job is
 * to receive LoRa mesh packets and re-broadcast them toward the gateway,
 * using the priority router to order transmissions.
 *
 * Hardware target: ESP32-C3 Mini + SX1262 LoRa module.
 * Pin assignments for ESP32-C3 Mini will differ from the ESP32-WROOM-32
 * mapping in docs/hardware/pin-mapping.md. A separate relay-node pin map
 * document will be added in Phase 2 when the C3 Mini is procured.
 *
 * Build environment: PlatformIO, Arduino framework, espressif32c3 platform.
 */

#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <cstdint>
  #include <cstdio>
#endif

// Note: lora_mesh.h and priority_router.h are shared between sensing and relay
// node firmware. They are duplicated here as includes from the sensing-node src
// for the design sketch. In a real PlatformIO project, they would live in a
// shared lib/ directory.
//
// TODO (Phase 2): Restructure as a PlatformIO monorepo with shared libraries
// under lib/ accessible to both sensing-node and relay-node environments.
#include "../sensing-node/src/lora_mesh.h"
#include "../sensing-node/src/priority_router.h"

#include <vector>
#include <algorithm>

// ---------------------------------------------------------------------------
// Node configuration
// ---------------------------------------------------------------------------

static constexpr uint16_t THIS_NODE_ID = 0x0010;  // Relay node IDs start at 0x0010

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

static SeenIdBuffer              seen_ids;
static std::vector<QueuedPacket> outbound_queue;

static std::vector<TransportPath> transport_paths = {
    { TransportType::LORA_MESH, true,  -127 },  // LoRa mesh: primary relay transport
    { TransportType::CELLULAR,  false, -127 },  // Relay nodes do not have cellular
    { TransportType::SATELLITE, false, -127 },
};

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------

#ifdef ARDUINO
void setup() {
    Serial.begin(115200);
    Serial.println("[LoRaLine] Relay node starting -- DESIGN STAGE, untested");

    // TODO (Phase 2): Set ESP32-C3 Mini SPI pin assignments for SX1262.
    // The C3 Mini has different default SPI pins from the WROOM-32.
    // Confirm pin mapping in docs/hardware/ before wiring.

    if (!lora_mesh_setup()) {
        Serial.println("[ERROR] LoRa mesh setup failed.");
    }

    seen_id_init(seen_ids);
    Serial.println("[LoRaLine] Relay node setup complete.");
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------

void loop() {
    uint32_t now_ms = millis();

    // Receive incoming packets and queue for relay
    MeshPacket incoming;
    if (lora_mesh_receive(incoming)) {
        if (incoming.header.hop_count < MESH_MAX_HOP_COUNT) {
            QueuedPacket relay_item;
            relay_item.packet_id       = incoming.header.packet_id;
            relay_item.priority        = static_cast<PacketPriority>(incoming.header.priority);
            relay_item.hop_count       = incoming.header.hop_count;
            relay_item.enqueue_time_ms = now_ms;
            relay_item.payload_len     = incoming.header.payload_len;
            outbound_queue.push_back(relay_item);
        } else {
            Serial.println("[MESH] Packet hop count exceeded, dropping.");
        }
    }

    // Drain outbound queue
    if (!outbound_queue.empty()) {
        RouterDecision decision = select_next_transmission(outbound_queue, transport_paths);
        if (decision.has_decision) {
            // TODO (Phase 2): Look up the original MeshPacket by packet_id and relay it.
            // For now, log and remove.
            Serial.print("[RELAY TX] Packet ID: ");
            Serial.println(decision.packet_id);

            outbound_queue.erase(
                std::remove_if(outbound_queue.begin(), outbound_queue.end(),
                    [&](const QueuedPacket& p) { return p.packet_id == decision.packet_id; }),
                outbound_queue.end()
            );
        }
    }

    delay(50);
}
#endif  // ARDUINO

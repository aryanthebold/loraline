/*
 * STATUS: UNTESTED -- DESIGN STAGE
 * This module has not been run on physical hardware. No ESP32, LoRa
 * radio, or sensors have been acquired as of 2026-09-27.
 * Logic validated only via [manual trace / not yet compiled against ESP32 toolchain].
 * Update this line honestly as testing progresses.
 *
 * main.cpp -- Sensing Node (ESP32-WROOM-32)
 *
 * This file shows how the four modules (sensor_reader, tinyml_inference,
 * lora_mesh, priority_router) are wired together in the sensing node loop.
 *
 * It is a design sketch, not a production-ready firmware. Comments throughout
 * flag specific TODO items for Phase 2 (first hardware prototype).
 *
 * Build environment: PlatformIO, Arduino framework, espressif32 platform.
 * See docs/firmware/design-notes.md for build environment detail.
 *
 * NOT integrated with real hardware. All hardware calls go through
 * stub functions defined in their respective headers. Replace stubs
 * with real implementations as hardware becomes available.
 */

// ---------------------------------------------------------------------------
// Arduino framework includes (only available when compiled with PlatformIO
// against the ESP32 Arduino core -- will not compile on a plain host g++).
// ---------------------------------------------------------------------------
#ifdef ARDUINO
  #include <Arduino.h>
#else
  // Host compilation stub (for syntax checking only)
  #include <cstdint>
  #include <cstdio>
  #define Serial_println(x) printf("%s\n", x)
#endif

#include "sensor_reader.h"
#include "tinyml_inference.h"
#include "lora_mesh.h"
#include "priority_router.h"

// ---------------------------------------------------------------------------
// Node configuration
// TODO (Phase 2): Move to config.h and make configurable via serial command
//                 or hardcoded per physical node at flash time.
// ---------------------------------------------------------------------------

static constexpr uint16_t THIS_NODE_ID      = 0x0001;  // Unique per deployed node
static constexpr uint32_t SENSE_INTERVAL_MS = 60000;   // 60 seconds between sense cycles

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

static RawSensorReading   previous_reading   = {};
static uint32_t           last_sense_time_ms = 0;
static uint32_t           packet_id_counter  = 0;
static uint32_t           last_nominal_time_ms = 0;

static SeenIdBuffer       seen_ids;
static std::vector<QueuedPacket> outbound_queue;

// Available transport paths (updated at runtime when transports come up/down)
static std::vector<TransportPath> transport_paths = {
    { TransportType::LORA_MESH, true,  -127 },  // LoRa mesh: assumed available; RSSI unknown until radio init
    { TransportType::CELLULAR,  false, -127 },  // Cellular: not available on sensing nodes (gateway only)
    { TransportType::SATELLITE, false, -127 },  // Satellite: not planned for Phase 1
};

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------

#ifdef ARDUINO
void setup() {
    Serial.begin(115200);
    Serial.println("[LoRaLine] Sensing node starting -- DESIGN STAGE, untested");

    // Initialize sensor ADC pins
    sensor_reader_setup();

    // Initialize TFLite Micro interpreter
    // TODO (Phase 2): This will fail silently with the stub. Check return value.
    if (!setup_inference()) {
        Serial.println("[WARN] TFLite Micro setup failed. Inference will return NOMINAL stub.");
    }

    // Initialize LoRa mesh (currently uses mock radio)
    if (!lora_mesh_setup()) {
        Serial.println("[ERROR] LoRa mesh setup failed.");
        // TODO (Phase 2): Enter error state, blink LED, retry logic
    }

    // Initialize seen-ID buffer
    seen_id_init(seen_ids);

    last_nominal_time_ms = millis();
    Serial.println("[LoRaLine] Setup complete.");
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------

void loop() {
    uint32_t now_ms = millis();

    // ------------------------------------------------------------------
    // 1. Check for incoming mesh packets to relay
    // ------------------------------------------------------------------
    MeshPacket incoming;
    if (lora_mesh_receive(incoming)) {
        // Packet is new (not a duplicate; seen_id check is in lora_mesh_receive)
        Serial.print("[MESH RX] Packet ID: ");
        Serial.println(incoming.header.packet_id);

        // If this node is not the destination (all packets are broadcast in
        // this flooding mesh), relay it.
        // TODO (Phase 2): Add hop count guard here before queuing for relay.
        if (incoming.header.hop_count < MESH_MAX_HOP_COUNT) {
            // Queue for relay via priority router
            QueuedPacket relay_item;
            relay_item.packet_id       = incoming.header.packet_id;
            relay_item.priority        = static_cast<PacketPriority>(incoming.header.priority);
            relay_item.hop_count       = incoming.header.hop_count;
            relay_item.enqueue_time_ms = now_ms;
            relay_item.payload_len     = incoming.header.payload_len;
            outbound_queue.push_back(relay_item);
        }
    }

    // ------------------------------------------------------------------
    // 2. Periodic sense cycle
    // ------------------------------------------------------------------
    if ((now_ms - last_sense_time_ms) >= SENSE_INTERVAL_MS) {
        last_sense_time_ms = now_ms;

        // Read sensors (stub: returns nominal hardcoded values)
        RawSensorReading current_reading;
        sensor_read_raw(current_reading);

        // Build feature vector for TFLite inference
        uint16_t seconds_since_nominal = static_cast<uint16_t>(
            (now_ms - last_nominal_time_ms) / 1000
        );
        SensorFeatures features = sensor_build_features(
            current_reading, previous_reading, seconds_since_nominal
        );

        // Run inference (stub: always returns NOMINAL)
        InferenceResult result;
        if (run_inference(features, result)) {
            if (result.hazard != HazardClass::NOMINAL && result.above_threshold) {
                // Build and queue a hazard alert packet
                HazardAlertPayload alert_payload;
                alert_payload.hazard_class   = static_cast<uint8_t>(result.hazard);
                alert_payload.confidence_pct = static_cast<uint8_t>(result.confidence * 100.0f);
                alert_payload.latitude_1e6   = 0;      // TODO (Phase 2): get from GPS
                alert_payload.longitude_1e6  = 0;      // TODO (Phase 2): get from GPS
                alert_payload.altitude_m     = 0;      // TODO (Phase 2): get from GPS
                alert_payload.battery_pct    = 75;     // TODO (Phase 2): measure battery ADC

                uint8_t pkt_priority = (result.hazard == HazardClass::FLOOD_RISING ||
                                        result.hazard == HazardClass::FIRE_SMOKE)
                                        ? static_cast<uint8_t>(PacketPriority::SOS)
                                        : static_cast<uint8_t>(PacketPriority::ALERT);

                MeshPacket alert_pkt = build_hazard_alert(
                    THIS_NODE_ID,
                    ++packet_id_counter,
                    pkt_priority,
                    now_ms / 1000,  // TODO (Phase 2): use GPS Unix timestamp
                    alert_payload
                );

                // TODO: actual lora_mesh_send() call; for now, print to serial
                Serial.print("[ALERT] Hazard class: ");
                Serial.println(alert_payload.hazard_class);
                lora_mesh_send(alert_pkt);

            } else {
                last_nominal_time_ms = now_ms;
            }
        }

        // Update previous reading for next delta calculation
        previous_reading = current_reading;
    }

    // ------------------------------------------------------------------
    // 3. Drain outbound queue via priority router
    // ------------------------------------------------------------------
    if (!outbound_queue.empty()) {
        RouterDecision decision = select_next_transmission(outbound_queue, transport_paths);
        if (decision.has_decision) {
            // TODO (Phase 2): Look up and transmit the actual MeshPacket by packet_id.
            // For now, log and remove from queue.
            Serial.print("[TX] Transmitting packet ID: ");
            Serial.println(decision.packet_id);

            // Remove selected packet from queue
            outbound_queue.erase(
                std::remove_if(outbound_queue.begin(), outbound_queue.end(),
                    [&](const QueuedPacket& p) { return p.packet_id == decision.packet_id; }),
                outbound_queue.end()
            );
        }
    }

    // ------------------------------------------------------------------
    // 4. Sleep
    // TODO (Phase 2): Replace delay() with actual ESP32 deep sleep.
    // Deep sleep requires careful handling of WiFi/BT deinit and
    // RTC wakeup configuration. Not implemented in this skeleton.
    // ------------------------------------------------------------------
    delay(100);
}
#endif  // ARDUINO

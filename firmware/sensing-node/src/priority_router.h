/*
 * STATUS: UNTESTED -- DESIGN STAGE
 * This module has not been run on physical hardware. No ESP32, LoRa
 * radio, or sensors have been acquired as of 2026-09-27.
 * Logic validated only via [manual trace / unit test].
 *
 * priority_router.h
 * Decides escalation order and transport selection for outbound MeshPackets.
 *
 * This module has NO hardware dependency. It operates purely on packet
 * metadata and path descriptors. It is intentionally written to be
 * compilable and testable on any host machine with a standard C++ compiler,
 * without any ESP32 SDK or Arduino framework.
 *
 * Unit tests are in priority_router.cpp (look for run_priority_router_tests()).
 */

#pragma once

#include <cstdint>
#include <vector>

// ---------------------------------------------------------------------------
// Priority levels for outbound packets
// ---------------------------------------------------------------------------

enum class PacketPriority : uint8_t {
    SOS     = 0x01,  // Life-safety hazard: immediate relay, preempts all others
    ALERT   = 0x02,  // Elevated reading: relay on next available slot
    ROUTINE = 0x03   // Telemetry ping: relay only when channel is idle
};

// ---------------------------------------------------------------------------
// Transport path types available to the router
// ---------------------------------------------------------------------------

enum class TransportType : uint8_t {
    LORA_MESH  = 0x01,  // LoRa mesh hop-by-hop relay (always available if radio is up)
    CELLULAR   = 0x02,  // GSM/LTE direct uplink (gateway nodes only)
    SATELLITE  = 0x03   // Satellite modem (future; not implemented in Phase 1)
};

// ---------------------------------------------------------------------------
// Descriptor for one available transport path
// ---------------------------------------------------------------------------

struct TransportPath {
    TransportType type;
    bool          available;       // Is this transport currently usable?
    int8_t        rssi_estimate;   // Estimated link quality (-120 to 0 dBm); -127 = unknown
};

// ---------------------------------------------------------------------------
// A packet waiting for transmission in the outbound queue
// ---------------------------------------------------------------------------

struct QueuedPacket {
    uint32_t        packet_id;
    PacketPriority  priority;
    uint8_t         hop_count;
    uint32_t        enqueue_time_ms;  // millis() at enqueue time (or mock value in tests)
    uint8_t         payload_len;
    // Payload bytes are not included here for routing purposes; the router
    // only needs metadata to make its decision.
};

// ---------------------------------------------------------------------------
// Router decision: which packet to send next and on which transport
// ---------------------------------------------------------------------------

struct RouterDecision {
    bool          has_decision;    // false if queue is empty or no transport is available
    uint32_t      packet_id;       // ID of the packet selected for transmission
    TransportType transport;       // Transport to use
};

// ---------------------------------------------------------------------------
// Core router function
//
// Given a list of queued packets and a list of available transport paths,
// select the next packet to transmit and the best transport for it.
//
// Escalation rules (in decreasing priority):
//   1. SOS packets are always selected before ALERT or ROUTINE packets,
//      regardless of enqueue order or wait time.
//   2. ALERT packets are selected before ROUTINE packets.
//   3. Within the same priority tier, the packet with the lowest
//      enqueue_time_ms (oldest first) is selected.
//   4. Transport selection:
//      a. For SOS: prefer CELLULAR if available, else LORA_MESH, else SATELLITE.
//      b. For ALERT and ROUTINE: prefer LORA_MESH (lower power, always the
//         primary transport); fall back to CELLULAR if LoRa is unavailable.
//      c. If no transport is available, RouterDecision.has_decision = false.
//
// Returns a RouterDecision. Does not remove the packet from the queue;
// the caller is responsible for dequeuing after transmission.
// ---------------------------------------------------------------------------

RouterDecision select_next_transmission(
    const std::vector<QueuedPacket>& queue,
    const std::vector<TransportPath>& paths
);

// ---------------------------------------------------------------------------
// Helper: find the best available transport for a given priority
// Returns a pointer to the chosen TransportPath, or nullptr if none available.
// ---------------------------------------------------------------------------

const TransportPath* select_transport(
    PacketPriority priority,
    const std::vector<TransportPath>& paths
);

/*
 * STATUS: UNTESTED -- DESIGN STAGE
 * This module has not been run on physical hardware. No ESP32, LoRa
 * radio, or sensors have been acquired as of 2026-09-27.
 * Logic validated only via [manual trace / not yet unit tested].
 * Update this line honestly as testing progresses.
 *
 * lora_mesh.h
 * Packet structure and mesh hop protocol for the LoRaLine LoRa mesh layer.
 *
 * The radio transport is currently MOCKED. All actual SPI/SX1262 calls
 * (via RadioLib) are replaced by mock functions that print to serial and
 * return success. This allows the mesh protocol logic to be reasoned about
 * and structurally completed before hardware is available.
 *
 * When real hardware is available, replace mock_radio_transmit() and
 * mock_radio_receive() with actual RadioLib SX1262 calls. See the
 * RadioLib SX1262 constructor note in docs/hardware/pin-mapping.md.
 *
 * Protocol: flooding with deduplication. Each node rebroadcasts any packet
 * it receives for the first time (identified by packet_id). Duplicate
 * suppression via a fixed-size rolling seen-ID buffer prevents infinite loops.
 *
 * Frequency: 865.0 MHz (India ISM band, 865-867 MHz).
 * Modulation: LoRa SF12, BW 125 kHz, CR 4/5 (design default; tune for range/airtime tradeoff).
 * Max payload: ~220 bytes at SF12 (LoRa physical layer limit).
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

// ---------------------------------------------------------------------------
// LoRa radio configuration constants (design values, not yet validated)
// ---------------------------------------------------------------------------

static constexpr float  LORA_FREQUENCY_MHZ = 865.0f;
static constexpr uint8_t LORA_SF           = 12;     // Spreading factor
static constexpr float  LORA_BW_KHZ        = 125.0f; // Bandwidth kHz
static constexpr uint8_t LORA_CR           = 5;      // Coding rate denominator (4/CR)
static constexpr int8_t LORA_TX_POWER_DBM  = 14;     // TX power dBm (balance of range and power)
static constexpr uint8_t LORA_SYNC_WORD    = 0x12;   // Private network sync word (Meshtastic default)

// ---------------------------------------------------------------------------
// Packet structure
//
// Wire format is this struct packed (no padding). Total header = 15 bytes.
// Payload follows immediately after the header.
// CRC covers header + payload.
// ---------------------------------------------------------------------------

static constexpr uint8_t MESH_MAX_PAYLOAD  = 200;  // bytes
static constexpr uint8_t MESH_MAX_HOP_COUNT = 7;   // drop packets exceeding this

struct __attribute__((packed)) MeshPacketHeader {
    uint32_t packet_id;     // Unique per-originator ID; deduplication key
    uint16_t source_node;   // Node ID of the originator (not the relayer)
    uint8_t  hop_count;     // Incremented by each relay; packet dropped if > MESH_MAX_HOP_COUNT
    uint8_t  priority;      // See PacketPriority in priority_router.h (0x01=SOS, 0x02=ALERT, 0x03=ROUTINE)
    uint32_t timestamp;     // Unix timestamp from GPS, or millis() if GPS fix not available
    uint8_t  payload_type;  // 0x01 = hazard alert, 0x02 = telemetry ping, 0x03 = ack
    uint8_t  payload_len;   // Byte length of the payload that follows
    uint16_t crc;           // CRC-16 over bytes [0..(sizeof(header)-2) + payload]
};

struct MeshPacket {
    MeshPacketHeader header;
    uint8_t          payload[MESH_MAX_PAYLOAD];
};

// ---------------------------------------------------------------------------
// Hazard alert payload (payload_type = 0x01)
// Packed into payload[] field of MeshPacket.
// ---------------------------------------------------------------------------

struct __attribute__((packed)) HazardAlertPayload {
    uint8_t  hazard_class;   // HazardClass enum value from tinyml_inference.h
    uint8_t  confidence_pct; // Confidence * 100 (0-100), to avoid float on wire
    int32_t  latitude_1e6;   // Latitude * 1e6 (degrees, signed)
    int32_t  longitude_1e6;  // Longitude * 1e6 (degrees, signed)
    uint16_t altitude_m;     // Altitude in meters (GPS derived, 0 if no fix)
    uint8_t  battery_pct;    // Battery state of charge estimate (0-100)
};

// ---------------------------------------------------------------------------
// Seen-ID buffer for duplicate suppression
//
// Fixed-size circular buffer. New packet IDs are added on receipt.
// If the buffer is full, the oldest entry is overwritten.
// This is intentionally simple for low-RAM environments.
// ---------------------------------------------------------------------------

static constexpr uint16_t SEEN_ID_BUFFER_SIZE = 64;  // entries

struct SeenIdBuffer {
    uint32_t ids[SEEN_ID_BUFFER_SIZE];
    uint16_t head;   // Index of the next write position (circular)
    uint16_t count;  // Number of valid entries (up to SEEN_ID_BUFFER_SIZE)
};

// Initialize the buffer (call once during setup)
void seen_id_init(SeenIdBuffer& buf);

// Returns true if packet_id is already in the buffer
bool seen_id_contains(const SeenIdBuffer& buf, uint32_t packet_id);

// Add packet_id to the buffer (overwrites oldest if full)
void seen_id_add(SeenIdBuffer& buf, uint32_t packet_id);

// ---------------------------------------------------------------------------
// CRC-16 (CCITT-FALSE) over a byte array
// ---------------------------------------------------------------------------

uint16_t mesh_crc16(const uint8_t* data, uint16_t len);

// ---------------------------------------------------------------------------
// Mock radio transport layer
//
// These functions replace actual RadioLib SX1262 calls during design-stage
// development. They simulate transmission and reception without any SPI
// hardware.
//
// mock_radio_transmit: prints the packet header fields to serial (or stdout
//   in host test context) and returns true (simulating TX success).
//
// mock_radio_receive: populates a MeshPacket with a hardcoded test packet
//   and returns true (simulating RX of one packet). Returns false after the
//   first call (simulating that no more packets are waiting).
//
// Replace these with real RadioLib calls in Phase 2.
// ---------------------------------------------------------------------------

bool mock_radio_transmit(const MeshPacket& pkt);
bool mock_radio_receive(MeshPacket& pkt_out);

// ---------------------------------------------------------------------------
// Mesh protocol functions
// ---------------------------------------------------------------------------

/*
 * lora_mesh_setup()
 *
 * Initializes the radio (currently calls mock setup; replace with RadioLib
 * SX1262 initialization in Phase 2) and clears the seen-ID buffer.
 *
 * Returns true on success.
 */
bool lora_mesh_setup();

/*
 * lora_mesh_send(pkt)
 *
 * Validates the packet CRC, increments hop count, and transmits via the
 * radio transport (currently mock). Returns false if hop count exceeds
 * MESH_MAX_HOP_COUNT or if CRC is invalid.
 */
bool lora_mesh_send(MeshPacket& pkt);

/*
 * lora_mesh_receive(pkt_out)
 *
 * Checks for an incoming packet. If a packet is received, validates its
 * CRC and checks the seen-ID buffer. If the packet is new (not a duplicate),
 * adds the ID to the seen-ID buffer, populates pkt_out, and returns true.
 * Returns false if no packet, duplicate packet, or CRC failure.
 */
bool lora_mesh_receive(MeshPacket& pkt_out);

/*
 * lora_mesh_relay(pkt)
 *
 * Called when a packet is received that is not destined for this node but
 * should be relayed. Increments hop_count and recomputes CRC before
 * re-transmitting. Returns false if hop_count would exceed limit.
 */
bool lora_mesh_relay(MeshPacket& pkt);

// ---------------------------------------------------------------------------
// Helper: build a hazard alert packet
// ---------------------------------------------------------------------------

MeshPacket build_hazard_alert(
    uint16_t source_node,
    uint32_t packet_id,
    uint8_t  priority,
    uint32_t timestamp,
    const HazardAlertPayload& alert
);

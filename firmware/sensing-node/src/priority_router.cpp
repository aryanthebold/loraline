/*
 * STATUS: UNTESTED -- DESIGN STAGE
 * This module has not been run on physical hardware. No ESP32, LoRa
 * radio, or sensors have been acquired as of 2026-09-27.
 * Logic validated only via [unit test -- see run_priority_router_tests() below].
 * Unit tests pass on host (compiled with g++ -std=c++17, no Arduino SDK needed).
 *
 * priority_router.cpp
 * Implementation and standalone unit tests for the priority router.
 *
 * To run unit tests on a host machine (Linux/macOS/WSL):
 *   g++ -std=c++17 -DRUN_PRIORITY_ROUTER_TESTS priority_router.cpp -o router_test
 *   ./router_test
 *
 * On Windows (MSVC or MinGW):
 *   g++ -std=c++17 -DRUN_PRIORITY_ROUTER_TESTS priority_router.cpp -o router_test.exe
 *   .\router_test.exe
 */

#include "priority_router.h"
#include <algorithm>
#include <limits>

#ifdef RUN_PRIORITY_ROUTER_TESTS
#include <cstdio>
#include <cstdlib>
#endif

// ---------------------------------------------------------------------------
// select_transport
// ---------------------------------------------------------------------------

const TransportPath* select_transport(
    PacketPriority priority,
    const std::vector<TransportPath>& paths
) {
    // Preferred transport order depends on packet priority.
    // SOS: try CELLULAR first (fastest uplink to responders if available),
    //      then LORA_MESH, then SATELLITE.
    // ALERT/ROUTINE: try LORA_MESH first (lower power, primary transport),
    //                then CELLULAR, then SATELLITE.

    std::vector<TransportType> preference_order;

    if (priority == PacketPriority::SOS) {
        preference_order = { TransportType::CELLULAR, TransportType::LORA_MESH, TransportType::SATELLITE };
    } else {
        preference_order = { TransportType::LORA_MESH, TransportType::CELLULAR, TransportType::SATELLITE };
    }

    for (TransportType preferred : preference_order) {
        for (const TransportPath& path : paths) {
            if (path.type == preferred && path.available) {
                return &path;
            }
        }
    }

    return nullptr;  // No transport available
}

// ---------------------------------------------------------------------------
// select_next_transmission
// ---------------------------------------------------------------------------

RouterDecision select_next_transmission(
    const std::vector<QueuedPacket>& queue,
    const std::vector<TransportPath>& paths
) {
    RouterDecision decision;
    decision.has_decision = false;
    decision.packet_id    = 0;
    decision.transport    = TransportType::LORA_MESH;

    if (queue.empty()) {
        return decision;
    }

    // Find the highest-priority packet. Within same priority, oldest first.
    const QueuedPacket* selected = nullptr;

    for (const QueuedPacket& pkt : queue) {
        if (selected == nullptr) {
            selected = &pkt;
            continue;
        }

        // Lower numeric value of PacketPriority means higher urgency
        // (SOS=0x01 < ALERT=0x02 < ROUTINE=0x03).
        bool higher_priority = (static_cast<uint8_t>(pkt.priority) < static_cast<uint8_t>(selected->priority));
        bool same_priority_older = (pkt.priority == selected->priority) &&
                                   (pkt.enqueue_time_ms < selected->enqueue_time_ms);

        if (higher_priority || same_priority_older) {
            selected = &pkt;
        }
    }

    if (selected == nullptr) {
        return decision;
    }

    const TransportPath* transport = select_transport(selected->priority, paths);
    if (transport == nullptr) {
        // No transport available; cannot transmit anything right now.
        return decision;
    }

    decision.has_decision = true;
    decision.packet_id    = selected->packet_id;
    decision.transport    = transport->type;

    return decision;
}

// ---------------------------------------------------------------------------
// Unit tests (compiled only when RUN_PRIORITY_ROUTER_TESTS is defined)
// ---------------------------------------------------------------------------

#ifdef RUN_PRIORITY_ROUTER_TESTS

static int tests_run    = 0;
static int tests_passed = 0;

static void check(bool condition, const char* test_name) {
    tests_run++;
    if (condition) {
        tests_passed++;
        printf("  PASS: %s\n", test_name);
    } else {
        printf("  FAIL: %s\n", test_name);
    }
}

// ---------------------------------------------------------------------------
// Test 1: SOS packet is selected before ROUTINE packet regardless of order
//
// Setup: Queue has two packets. A ROUTINE packet enqueued first (older),
//        and an SOS packet enqueued later (newer). Only LoRa mesh available.
// Expected: SOS packet is selected, on LoRa mesh transport.
// ---------------------------------------------------------------------------
static void test_sos_preempts_routine() {
    printf("\nTest 1: SOS preempts ROUTINE regardless of enqueue order\n");

    QueuedPacket routine_pkt;
    routine_pkt.packet_id       = 101;
    routine_pkt.priority        = PacketPriority::ROUTINE;
    routine_pkt.hop_count       = 0;
    routine_pkt.enqueue_time_ms = 1000;  // older
    routine_pkt.payload_len     = 10;

    QueuedPacket sos_pkt;
    sos_pkt.packet_id       = 202;
    sos_pkt.priority        = PacketPriority::SOS;
    sos_pkt.hop_count       = 0;
    sos_pkt.enqueue_time_ms = 5000;  // newer
    sos_pkt.payload_len     = 10;

    std::vector<QueuedPacket> queue = { routine_pkt, sos_pkt };

    TransportPath lora_path;
    lora_path.type          = TransportType::LORA_MESH;
    lora_path.available     = true;
    lora_path.rssi_estimate = -80;

    std::vector<TransportPath> paths = { lora_path };

    RouterDecision d = select_next_transmission(queue, paths);

    check(d.has_decision,                 "has_decision is true");
    check(d.packet_id == 202,             "SOS packet (ID 202) is selected");
    check(d.transport == TransportType::LORA_MESH, "transport is LORA_MESH");
}

// ---------------------------------------------------------------------------
// Test 2: SOS packet prefers CELLULAR over LoRa mesh when both available
//
// Setup: Queue has one SOS packet. Both CELLULAR and LORA_MESH are available.
// Expected: CELLULAR is selected for SOS (fastest path to responders).
// ---------------------------------------------------------------------------
static void test_sos_prefers_cellular() {
    printf("\nTest 2: SOS prefers CELLULAR over LoRa mesh\n");

    QueuedPacket sos_pkt;
    sos_pkt.packet_id       = 303;
    sos_pkt.priority        = PacketPriority::SOS;
    sos_pkt.hop_count       = 0;
    sos_pkt.enqueue_time_ms = 2000;
    sos_pkt.payload_len     = 10;

    std::vector<QueuedPacket> queue = { sos_pkt };

    TransportPath lora_path;
    lora_path.type          = TransportType::LORA_MESH;
    lora_path.available     = true;
    lora_path.rssi_estimate = -85;

    TransportPath cell_path;
    cell_path.type          = TransportType::CELLULAR;
    cell_path.available     = true;
    cell_path.rssi_estimate = -70;

    std::vector<TransportPath> paths = { lora_path, cell_path };

    RouterDecision d = select_next_transmission(queue, paths);

    check(d.has_decision,                     "has_decision is true");
    check(d.packet_id == 303,                 "SOS packet (ID 303) is selected");
    check(d.transport == TransportType::CELLULAR, "transport is CELLULAR for SOS");
}

// ---------------------------------------------------------------------------
// Test 3: Empty queue returns no decision
//
// Setup: Empty queue, LoRa mesh available.
// Expected: has_decision is false.
// ---------------------------------------------------------------------------
static void test_empty_queue() {
    printf("\nTest 3: Empty queue returns has_decision = false\n");

    std::vector<QueuedPacket> queue = {};

    TransportPath lora_path;
    lora_path.type          = TransportType::LORA_MESH;
    lora_path.available     = true;
    lora_path.rssi_estimate = -80;

    std::vector<TransportPath> paths = { lora_path };

    RouterDecision d = select_next_transmission(queue, paths);

    check(!d.has_decision, "has_decision is false for empty queue");
}

// ---------------------------------------------------------------------------
// Test 4: No transport available returns no decision even with queued packets
//
// Setup: Queue has one ALERT packet. No transport paths available.
// Expected: has_decision is false (cannot transmit).
// ---------------------------------------------------------------------------
static void test_no_transport_available() {
    printf("\nTest 4: No transport available => has_decision = false\n");

    QueuedPacket alert_pkt;
    alert_pkt.packet_id       = 404;
    alert_pkt.priority        = PacketPriority::ALERT;
    alert_pkt.hop_count       = 0;
    alert_pkt.enqueue_time_ms = 3000;
    alert_pkt.payload_len     = 12;

    std::vector<QueuedPacket> queue = { alert_pkt };

    TransportPath lora_path;
    lora_path.type          = TransportType::LORA_MESH;
    lora_path.available     = false;  // LoRa radio is down
    lora_path.rssi_estimate = -127;

    TransportPath cell_path;
    cell_path.type          = TransportType::CELLULAR;
    cell_path.available     = false;  // No cellular coverage
    cell_path.rssi_estimate = -127;

    std::vector<TransportPath> paths = { lora_path, cell_path };

    RouterDecision d = select_next_transmission(queue, paths);

    check(!d.has_decision, "has_decision is false when no transport available");
}

// ---------------------------------------------------------------------------
// main: run all tests
// ---------------------------------------------------------------------------
int main() {
    printf("=== priority_router unit tests ===\n");
    printf("These tests run on the host with no hardware dependency.\n");

    test_sos_preempts_routine();
    test_sos_prefers_cellular();
    test_empty_queue();
    test_no_transport_available();

    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);

    return (tests_passed == tests_run) ? 0 : 1;
}

#endif  // RUN_PRIORITY_ROUTER_TESTS

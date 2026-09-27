/*
 * STATUS: UNTESTED -- DESIGN STAGE
 * This module has not been run on physical hardware. No ESP32, LoRa
 * radio, or sensors have been acquired as of 2026-09-27.
 * Logic validated only via [manual trace / not yet unit tested].
 * Update this line honestly as testing progresses.
 *
 * sensor_reader.h
 * Reads MQ-2 gas/smoke sensor and analog water-level sensor from ESP32 ADC.
 *
 * Pin assignments (from docs/hardware/pin-mapping.md):
 *   GPIO34 -- MQ-2 analog output (AOUT)
 *   GPIO35 -- Water level sensor analog output
 *
 * IMPORTANT: GPIO34 and GPIO35 are input-only pins. Do not configure them
 * as outputs. They do not have internal pull-ups.
 *
 * IMPORTANT: MQ-2 heater draws 150-300 mA at 5V. Connect MQ-2 VCC to 5V/VIN
 * rail, NOT to the 3.3V rail. The AOUT pin reads on the 3.3V ADC.
 * See docs/hardware/pin-mapping.md for detail.
 *
 * ESP32 ADC note: The ESP32 ADC has known non-linearity near 0V and near 3.3V.
 * Readings below approximately 100 and above approximately 3900 (out of 4095)
 * may not be accurate. Factor this into sensor calibration during Phase 2.
 */

#pragma once

#include <cstdint>

// ---------------------------------------------------------------------------
// Pin assignments
// ---------------------------------------------------------------------------

static constexpr uint8_t PIN_MQ2_AOUT        = 34;
static constexpr uint8_t PIN_WATER_LEVEL_AOUT = 35;

// ---------------------------------------------------------------------------
// MQ-2 preheat configuration
//
// The MQ-2 sensor element must be heated to operating temperature before
// readings are stable. The datasheet specifies a warm-up time of 20 seconds
// minimum (shorter for a sensor that has been recently powered). In a clean
// environment, the baseline resistance stabilizes after approximately 60 seconds.
//
// Strategy: power the heater for PREHEAT_DURATION_MS before taking a reading.
// To save power, the heater is powered off between reading cycles.
//
// NOTE: "heater on" means VCC is connected to the sensor. In this design,
// MQ-2 VCC is always connected to the 5V rail and the sensor is always
// drawing heater current. A future hardware revision should add a MOSFET
// switch to cut VCC between readings. This is noted here as a power
// optimization for Phase 2.
// ---------------------------------------------------------------------------

static constexpr uint32_t PREHEAT_DURATION_MS = 20000;  // 20 seconds minimum

// ---------------------------------------------------------------------------
// ADC moving average window
// ---------------------------------------------------------------------------

static constexpr uint8_t ADC_MOVING_AVG_WINDOW = 5;  // number of samples to average

// ---------------------------------------------------------------------------
// Raw sensor reading (before feature engineering)
// ---------------------------------------------------------------------------

struct RawSensorReading {
    uint16_t mq2_adc;         // 0-4095
    uint16_t water_level_adc; // 0-4095
    uint32_t timestamp_ms;    // millis() at time of read (or mock in tests)
    bool     preheat_complete; // Was MQ-2 preheat duration met before this reading?
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/*
 * sensor_reader_setup()
 *
 * Configures GPIO34 and GPIO35 as ADC inputs. Sets ADC resolution to 12 bits.
 * Call once during setup().
 *
 * On real hardware: calls analogReadResolution(12) and analogSetAttenuation()
 * to configure the ADC range to 0-3.3V.
 *
 * TODO (Phase 2): Replace with real Arduino analogRead setup calls when
 * compiling against the ESP32 Arduino core.
 */
void sensor_reader_setup();

/*
 * sensor_read_raw(reading_out)
 *
 * Takes one raw ADC reading from both sensors. Applies a simple N-sample
 * average to reduce ADC noise (N = ADC_MOVING_AVG_WINDOW).
 *
 * Sets reading_out.preheat_complete based on whether PREHEAT_DURATION_MS
 * has elapsed since power-on or since the last explicit heater power cycle.
 *
 * Populates reading_out.timestamp_ms with millis().
 *
 * STUB BEHAVIOR (current): returns hardcoded nominal values
 *   mq2_adc = 512 (low gas, nominal)
 *   water_level_adc = 100 (minimal water contact, nominal)
 *   preheat_complete = true (stub always reports preheat complete)
 *
 * TODO (Phase 2): Replace with actual analogRead(PIN_MQ2_AOUT) and
 * analogRead(PIN_WATER_LEVEL_AOUT) calls.
 */
void sensor_read_raw(RawSensorReading& reading_out);

/*
 * sensor_build_features(current, previous)
 *
 * Derives the SensorFeatures input vector from the current and previous
 * RawSensorReading. Computes delta values and the 5-sample moving average.
 *
 * Returns: SensorFeatures struct ready for tinyml_inference.h run_inference().
 *
 * NOTE: This function is defined here (inline) because it has no hardware
 * dependency and can be unit-tested on the host. See tinyml_inference.h
 * for the SensorFeatures definition.
 */
#include "tinyml_inference.h"

inline SensorFeatures sensor_build_features(
    const RawSensorReading& current,
    const RawSensorReading& previous,
    uint16_t                seconds_since_nominal
) {
    SensorFeatures f;
    f.mq2_raw   = static_cast<int16_t>(current.mq2_adc);
    f.mq2_delta = static_cast<int16_t>(current.mq2_adc) - static_cast<int16_t>(previous.mq2_adc);
    f.water_raw = static_cast<int16_t>(current.water_level_adc);
    f.water_delta = static_cast<int16_t>(current.water_level_adc) - static_cast<int16_t>(previous.water_level_adc);
    // Moving average: in the full implementation, keep a rolling buffer of 5 readings.
    // Stub: use current reading as the average.
    f.water_avg5 = static_cast<int16_t>(current.water_level_adc);
    f.seconds_since_nominal = seconds_since_nominal;
    return f;
}

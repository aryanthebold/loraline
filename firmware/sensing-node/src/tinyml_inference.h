/*
 * STATUS: UNTESTED -- DESIGN STAGE
 * This module has not been run on physical hardware. No ESP32, LoRa
 * radio, or sensors have been acquired as of 2026-09-27.
 * Logic validated only via [manual trace / not yet unit tested].
 * Update this line honestly as testing progresses.
 *
 * tinyml_inference.h
 * Function signature and input/output contract for the TinyML sensor-fusion
 * hazard classifier.
 *
 * The actual trained model (.tflite -> C array) does not exist yet.
 * The implementation stub (in tinyml_inference.cpp, to be created in Phase 2)
 * currently returns HazardClass::NOMINAL unconditionally.
 *
 * See ml-model/model-architecture.md for the planned model topology.
 * See ml-model/training-notes.md for training data collection plan.
 *
 * TFLite Micro dependency: this header is designed for use with the
 * TensorFlow Lite Micro library on PlatformIO (library name: "tflite-micro").
 * It does NOT include TFLite headers directly here to keep the header usable
 * in unit test contexts without the full TFLite Micro build.
 */

#pragma once

#include <cstdint>

// ---------------------------------------------------------------------------
// Input feature struct
//
// All raw ADC values are 12-bit (0-4095) from the ESP32 ADC.
// Delta fields are (current - previous reading); negative means decreasing.
// The moving average is computed over the last 5 readings.
//
// Input tensor shape (for TFLite): [1, 6] float32, normalized to [0.0, 1.0]
// before quantization to int8 inside the model.
//
// Normalization mapping (design intent, to be confirmed during training):
//   mq2_raw          : 0..4095 -> 0.0..1.0
//   mq2_delta        : -4095..+4095 -> 0.0..1.0 (centered at 0.5)
//   water_raw        : 0..4095 -> 0.0..1.0
//   water_delta      : -4095..+4095 -> 0.0..1.0 (centered at 0.5)
//   water_avg5       : 0..4095 -> 0.0..1.0
//   seconds_since_nominal : 0..3600 -> 0.0..1.0 (capped at 3600 seconds)
// ---------------------------------------------------------------------------

struct SensorFeatures {
    int16_t  mq2_raw;               // MQ-2 ADC reading, 0-4095
    int16_t  mq2_delta;             // Change from previous reading
    int16_t  water_raw;             // Water level ADC reading, 0-4095
    int16_t  water_delta;           // Change from previous reading
    int16_t  water_avg5;            // 5-sample moving average of water_raw
    uint16_t seconds_since_nominal; // Time since last NOMINAL classification, capped at 3600
};

// ---------------------------------------------------------------------------
// Output: hazard classification result
// ---------------------------------------------------------------------------

enum class HazardClass : uint8_t {
    NOMINAL         = 0,  // All readings within normal bounds
    FLOOD_RISING    = 1,  // Water level above threshold or rising rapidly
    FIRE_SMOKE      = 2,  // MQ-2 above fire/smoke signature threshold
    POLLUTION_SPIKE = 3   // Sustained elevated MQ-2, below fire threshold
};

struct InferenceResult {
    HazardClass hazard;
    float       confidence;   // Softmax output for the selected class, 0.0-1.0
    bool        above_threshold;  // true if confidence >= INFERENCE_CONFIDENCE_THRESHOLD
};

// Minimum confidence to act on a hazard classification.
// If below this, the result is treated as NOMINAL to avoid false positives.
// Value is a design estimate; tune after collecting validation data.
static constexpr float INFERENCE_CONFIDENCE_THRESHOLD = 0.70f;

// ---------------------------------------------------------------------------
// Tensor arena
//
// Static buffer for TFLite Micro interpreter scratch memory.
// Size 40 * 1024 bytes = 40 KB (design estimate; measure actual requirement
// after model is compiled -- see known-unknowns.md risk #5).
//
// This buffer is declared in the .cpp implementation file to avoid
// multiple-definition errors. Declared here only as a size constant.
// ---------------------------------------------------------------------------

static constexpr size_t TENSOR_ARENA_SIZE = 40 * 1024;  // bytes

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/*
 * setup_inference()
 *
 * Loads the TFLite model flatbuffer, allocates the tensor arena, and
 * initializes the TFLite Micro interpreter.
 *
 * Must be called once during setup() before run_inference() is used.
 *
 * Returns: true on success, false if model loading or interpreter
 * initialization fails (e.g., tensor arena too small).
 *
 * TODO (Phase 2): Replace the stub implementation with actual TFLite Micro
 * initialization using the model C array generated from model.tflite.
 */
bool setup_inference();

/*
 * run_inference(features, result)
 *
 * Runs one forward pass of the TFLite classifier on the given SensorFeatures.
 *
 * Parameters:
 *   features  -- input sensor data (see SensorFeatures above)
 *   result    -- output struct populated with HazardClass and confidence
 *
 * Returns: true if inference ran successfully, false on TFLite error.
 *
 * STUB BEHAVIOR (current): always writes HazardClass::NOMINAL with confidence
 * 1.0 and above_threshold = true. The stub exists so the rest of the firmware
 * can be structured and compiled without a real model.
 *
 * TODO (Phase 2): Replace stub with actual tflite::MicroInterpreter::Invoke()
 * call. Input tensor format: float32 [1, 6], values normalized to [0.0, 1.0].
 * Output tensor format: float32 [1, 4] softmax probabilities.
 */
bool run_inference(const SensorFeatures& features, InferenceResult& result);

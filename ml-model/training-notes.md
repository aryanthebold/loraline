# TinyML Training Notes

**Status: Design stage. No training has been performed. No dataset exists yet.**

This document records decisions and constraints that will guide the training process when data collection begins.

---

## Constraints from Hardware

1. **Tensor arena budget:** 40-80 KB of SRAM. The model must fit within this budget after int8 quantization, including all layer activations and intermediate buffers.
2. **Inference latency target:** under 50 ms per inference on the ESP32 at 240 MHz. For a small MLP with 200 parameters, this is easily achievable. The 50 ms budget allows 5-10 sensing cycles per second if needed, though in practice inference will run once per 60-second sense cycle.
3. **Input dimensionality:** 6 scalar features (see model-architecture.md). This is small enough for an MLP; a CNN adds overhead that is only justified if temporal features significantly improve accuracy.

---

## Toolchain

- Python 3.10+
- TensorFlow 2.13+ (or later) for model training
- `tf.lite.TFLiteConverter` for conversion to TFLite format
- `xxd` (Linux/WSL) for converting .tflite to a C byte array
- TFLite Micro on PlatformIO for on-device inference

---

## Training Strategy

- Split: 70% train, 15% validation, 15% test
- Loss: categorical crossentropy
- Optimizer: Adam with default learning rate (0.001), reduce on plateau
- Early stopping: patience 20 epochs on validation loss
- Epochs budget: up to 500 (expected to converge well before this for a small MLP)
- Class balancing: ensure roughly equal samples per class in training split to avoid the model defaulting to NOMINAL

---

## Validation Criteria Before Deployment

A model will not be flashed to field hardware until:

1. Test set accuracy >= 90% on held-out labeled samples from at least 2 different MQ-2 units (to test generalization across unit variation).
2. No individual class recall below 85% (a missed FIRE_SMOKE or FLOOD_RISING is worse than a false positive).
3. Quantized (int8) model accuracy is within 3 percentage points of the float32 baseline.
4. Tensor arena measured and confirmed to fit within the ESP32 SRAM budget alongside mesh buffers.

---

## Known Data Quality Risks

- MQ-2 requires a preheat period (up to 60 seconds from cold). Data collected before preheat completes will show systematically lower readings and must be discarded or labeled separately.
- MQ-2 output is sensitive to ambient temperature and humidity. Training data should be collected across a range of conditions, or the team must accept that the model will have reduced accuracy in environments (high monsoon humidity, low winter temperatures) not represented in the training set.
- Water level sensors from different manufacturers may have different output voltage ranges. Document the specific sensor used during data collection.

---

## Next Steps

1. Procure 2 MQ-2 sensors and a water level sensor (see bill-of-materials.md).
2. Write a data collection script that reads both ADC channels at 1 Hz, logs to SD card or serial, and records a manual label for each session (NOMINAL, FIRE_SMOKE, FLOOD_RISING, POLLUTION_SPIKE).
3. Run 30-minute labeled sessions for each class condition.
4. Clean and balance the dataset.
5. Train and validate the MLP model.
6. Convert and flash to one ESP32 for latency measurement.

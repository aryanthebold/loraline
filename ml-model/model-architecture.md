# TinyML Model Architecture

**Status: Design stage. No model has been trained. No training dataset exists yet.**

This document describes the planned architecture for the on-device hazard classification model used in the LoRaLine sensing node.

---

## Goal

Produce a binary sensor-fusion classifier that runs on the ESP32-WROOM-32 using TensorFlow Lite Micro, quantized to int8, fitting within a tensor arena of 40-80 KB of SRAM, and achieving acceptable classification accuracy on real sensor readings from MQ-2 and analog water-level sensors.

---

## Input Feature Vector

| Index | Feature | Derivation |
|---|---|---|
| 0 | MQ-2 ADC raw (0-4095) | Direct 12-bit ADC read |
| 1 | MQ-2 ADC delta (current - previous) | Derived: rate of change |
| 2 | Water level ADC raw (0-4095) | Direct 12-bit ADC read |
| 3 | Water level delta | Derived: rate of change |
| 4 | Water level 5-sample moving average | Smoothed trend |
| 5 | Elapsed time since last NOMINAL reading (seconds, capped at 3600) | State feature |

Input shape: `[1, 6]` (batch size 1, 6 features). All features normalized to float32 range [0.0, 1.0] before quantization.

---

## Output Classes

| Index | Label | Description |
|---|---|---|
| 0 | NOMINAL | All readings within normal bounds |
| 1 | FLOOD_RISING | Water level above threshold or rate of rise exceeds limit |
| 2 | FIRE_SMOKE | MQ-2 output above fire/smoke signature threshold |
| 3 | POLLUTION_SPIKE | Sustained elevated MQ-2 reading below fire threshold |

Output: softmax over 4 classes. Inference result is the argmax of the output tensor. A confidence threshold (initially 0.7) is applied: if max confidence is below threshold, the result is treated as NOMINAL to avoid false positives.

---

## Planned Model Topology

Target: a small multilayer perceptron (MLP). A 1D CNN is considered as an alternative if temporal sequence features are incorporated (i.e., feeding a window of readings rather than a single reading + derived features).

**MLP option (primary plan):**

```
Input [6] -> Dense(16, relu) -> Dense(8, relu) -> Dense(4, softmax)
```

Approximate parameter count: ~200 weights. Well within memory budget after int8 quantization (~200 bytes for weights, plus activation buffers).

**1D CNN option (secondary plan, if temporal features improve accuracy):**

Input shape changes to `[1, T, 2]` where T is the sequence length (e.g., 10 time steps, 2 channels: MQ-2 and water level).

```
Input [1, 10, 2] -> Conv1D(8 filters, kernel 3, relu) -> GlobalAveragePooling -> Dense(4, softmax)
```

This adds complexity but may be necessary if the simple MLP cannot distinguish POLLUTION_SPIKE from NOMINAL using point-in-time readings.

---

## Quantization Plan

- Train in float32 using TensorFlow/Keras.
- Apply TFLite post-training quantization (integer-only quantization, `DEFAULT` optimization with representative dataset).
- Target: int8 weights and activations.
- Convert using `tf.lite.TFLiteConverter.from_keras_model()` with `optimizations = [tf.lite.Optimize.DEFAULT]`.
- Validate quantized model accuracy against a held-out test set before deploying.
- Convert .tflite file to C array using `xxd -i model.tflite > model_data.cpp`.

---

## Training Data Plan (Not Yet Collected)

Phase 1 task: collect labeled sensor readings from at least 2 MQ-2 units and a water level sensor under the following conditions:

- Normal indoor air (NOMINAL baseline)
- LPG lighter gas near sensor (FIRE_SMOKE proxy)
- Sustained smoke from incense or a small controlled flame (FIRE_SMOKE)
- Sensor submerged to varying depths in water (FLOOD_RISING classes)
- Partial sensor contamination or dust (potential false positive scenario)

Temperature and humidity will be recorded alongside each reading to understand sensor drift under varying conditions.

Minimum dataset target: 1000 labeled samples per class before training.

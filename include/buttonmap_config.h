#pragma once

#include <stdint.h>

namespace ButtonmapConfig {

// Atomic CAN Base A103 is stacked below AtomS3R: G5 = CAN_TX, G6 = CAN_RX.
// Unit 3.96 U020 is connected to the Grove port: G2 and G1 receive the two
// FTU contacts through its screw terminal.
constexpr uint8_t kButton1Pin = 2;  // JQ FTU #1, red/white
constexpr uint8_t kButton2Pin = 1;  // JQ FTU #2, yellow/green
constexpr uint8_t kCanTxPin = 5;
constexpr uint8_t kCanRxPin = 6;

constexpr uint32_t kDebounceMs = 25;
constexpr uint32_t kTransmitTimeoutMs = 20;
}  // namespace ButtonmapConfig

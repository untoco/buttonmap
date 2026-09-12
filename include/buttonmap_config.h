#pragma once

#include <stdint.h>

namespace ButtonmapConfig {

// Atomic DIY Proto Kit A077 is stacked below AtomS3R. During its one-time
// assembly, route G5, G6 and GND to its VH3.96 connector; confirm continuity
// before powering the module. The AtomS3R Grove port is reserved for Unit CAN
// U085: G2 = CAN_TX, G1 = CAN_RX.
constexpr uint8_t kPreviousButtonPin = 5;  // JQ FTU #1, red/white
constexpr uint8_t kNextButtonPin = 6;      // JQ FTU #2, yellow/green
constexpr uint8_t kCanTxPin = 2;
constexpr uint8_t kCanRxPin = 1;

// K-CAN2 is configured as Classical CAN at 100 kbit/s. Confirm this setting
// with a passive capture on the target car before connecting in normal mode.
constexpr uint32_t kCanBitRate = 100000;
constexpr uint32_t kMediaCanId = 0x0A3;
constexpr uint8_t kMediaDlc = 2;
constexpr uint8_t kMediaSecondByte = 0xFF;
constexpr uint8_t kPreviousCommand = 0xFD;
constexpr uint8_t kNextCommand = 0xFE;
constexpr uint8_t kNeutralCommand = 0xFC;

constexpr uint32_t kDebounceMs = 25;
constexpr uint32_t kReleaseDelayMs = 80;
constexpr uint32_t kTransmitTimeoutMs = 20;
}  // namespace ButtonmapConfig

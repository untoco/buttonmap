#pragma once

#include <stdint.h>

namespace ButtonmapConfig {

// Atomic CAN Base A103 is stacked below AtomS3R: G5 = CAN_TX, G6 = CAN_RX.
// Unit 3.96 U020 is connected to the Grove port: G2 and G1 receive the two
// FTU contacts through its screw terminal.
constexpr uint8_t kPreviousButtonPin = 2;  // JQ FTU #1, red/white
constexpr uint8_t kNextButtonPin = 1;      // JQ FTU #2, yellow/green
constexpr uint8_t kCanTxPin = 5;
constexpr uint8_t kCanRxPin = 6;

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

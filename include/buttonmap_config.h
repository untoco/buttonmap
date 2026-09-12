#pragma once

#include <stdint.h>

namespace ButtonmapConfig {

// AtomS3R-Ext proto board: buttons use the dedicated G3/G4 pads. The Grove
// port is reserved for Unit CAN U085: G2 = CAN_TX and G1 = CAN_RX.
constexpr uint8_t kPreviousButtonPin = 3;  // JQ FTU #1, red/white
constexpr uint8_t kNextButtonPin = 4;      // JQ FTU #2, yellow/green
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

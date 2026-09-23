#pragma once

#include <stdint.h>

namespace ButtonmapCan {

struct Frame {
  uint32_t id;
  bool extended;
  uint8_t length;
  uint8_t data[8];
};

struct Preset {
  const char* name;
  uint32_t bitrate;
  Frame button1;
  Frame button2;
  Frame neutral;
  uint32_t releaseDelayMs;
  const char* button1Label;
  const char* button2Label;
};

// Кадры переключения треков предоставлены владельцем автомобиля.
// Скорость 100 кбит/с и задержка 80 мс пока требуют стендовой проверки.
constexpr Preset kMusic = {
    "music",
    100000,
    {0x0A3, false, 2, {0xFD, 0xFF}},
    {0x0A3, false, 2, {0xFE, 0xFF}},
    {0x0A3, false, 2, {0xFC, 0xFF}},
    80,
    "PREVIOUS",
    "NEXT",
};

// Добавляйте сюда только пресеты с проверенными кадрами и назначением кнопок.
constexpr Preset kPresets[] = {kMusic};
constexpr uint8_t kPresetCount = sizeof(kPresets) / sizeof(kPresets[0]);
constexpr uint8_t kActivePresetIndex = 0;
static_assert(kActivePresetIndex < kPresetCount, "Unknown CAN preset");
constexpr const Preset& kActivePreset = kPresets[kActivePresetIndex];

}  // namespace ButtonmapCan

#include <M5Unified.h>
#include <Preferences.h>
#include <buttonmap_config.h>
#include <can_presets.h>
#include "driver/twai.h"

namespace {

using namespace ButtonmapConfig;
using ButtonmapCan::Frame;
using ButtonmapCan::Preset;

enum class Action : uint8_t { Button1, Button2 };

struct Button {
  uint8_t pin;
  Action action;
  bool stablePressed;
  bool sampledPressed;
  uint32_t lastSampleChangeMs;

  Button(uint8_t pinValue, Action actionValue)
      : pin(pinValue), action(actionValue), stablePressed(false), sampledPressed(false),
        lastSampleChangeMs(0) {}
};

Button firstButton{kButton1Pin, Action::Button1};
Button secondButton{kButton2Pin, Action::Button2};
bool canReady = false;
bool canDriverInstalled = false;
uint32_t canDriverBitrate = 0;
bool neutralPending = false;
uint32_t neutralDueMs = 0;
uint8_t activePresetIndex = ButtonmapCan::kDefaultPresetIndex;
bool presetSwitchPending = false;

const Preset& activePreset() {
  return ButtonmapCan::kPresets[activePresetIndex];
}

void displayStatus(const char* top, uint16_t colour) {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(colour, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.drawString(top, M5.Display.width() / 2, 32);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(1);
  M5.Display.drawString(activePreset().name, M5.Display.width() / 2, 76);
}

void displayPresetStatus() {
  if (!activePreset().canConfigured) {
    displayStatus("NO CODES", TFT_YELLOW);
  } else {
    displayStatus(canReady ? "READY" : "CAN ERROR", canReady ? TFT_GREEN : TFT_RED);
  }
}

uint8_t loadPresetIndex() {
  Preferences preferences;
  if (!preferences.begin("buttonmap", true)) {
    Serial.println("Preset storage unavailable; using MUSIC");
    return ButtonmapCan::kDefaultPresetIndex;
  }
  const uint8_t index = preferences.getUChar("preset", ButtonmapCan::kDefaultPresetIndex);
  preferences.end();
  if (index >= ButtonmapCan::kPresetCount) {
    Serial.println("Stored preset is unknown; using MUSIC");
    return ButtonmapCan::kDefaultPresetIndex;
  }
  return index;
}

bool savePresetIndex(uint8_t index) {
  Preferences preferences;
  if (!preferences.begin("buttonmap", false)) return false;
  const bool saved = preferences.putUChar("preset", index) == 1;
  preferences.end();
  return saved;
}

bool validFrame(const Frame& frame) {
  return frame.length <= 8 &&
         (frame.extended ? frame.id <= 0x1FFFFFFF : frame.id <= 0x7FF);
}

bool transmitFrame(const Frame& frame) {
  if (!validFrame(frame)) return false;
  twai_message_t message = {};
  message.identifier = frame.id;
  message.extd = frame.extended;
  message.data_length_code = frame.length;
  for (uint8_t i = 0; i < frame.length; ++i) message.data[i] = frame.data[i];
  const esp_err_t result = twai_transmit(&message, pdMS_TO_TICKS(kTransmitTimeoutMs));
  if (result != ESP_OK) {
    Serial.printf("TX %03lX failed: %s\n", static_cast<unsigned long>(frame.id),
                  esp_err_to_name(result));
    displayStatus("CAN TX ERROR", TFT_RED);
    return false;
  }
  Serial.printf("TX %03lX [", static_cast<unsigned long>(frame.id));
  for (uint8_t i = 0; i < frame.length; ++i) {
    Serial.printf("%s%02X", i ? " " : "", frame.data[i]);
  }
  Serial.println("]");
  return true;
}

bool canTiming(uint32_t bitrate, twai_timing_config_t& timing) {
  switch (bitrate) {
    case 25000: timing = TWAI_TIMING_CONFIG_25KBITS(); return true;
    case 50000: timing = TWAI_TIMING_CONFIG_50KBITS(); return true;
    case 100000: timing = TWAI_TIMING_CONFIG_100KBITS(); return true;
    case 125000: timing = TWAI_TIMING_CONFIG_125KBITS(); return true;
    case 250000: timing = TWAI_TIMING_CONFIG_250KBITS(); return true;
    case 500000: timing = TWAI_TIMING_CONFIG_500KBITS(); return true;
    case 800000: timing = TWAI_TIMING_CONFIG_800KBITS(); return true;
    case 1000000: timing = TWAI_TIMING_CONFIG_1MBITS(); return true;
    default: return false;
  }
}

bool initialiseCan(const Preset& preset) {
  if (!preset.canConfigured) return false;
  if (!validFrame(preset.button1) || !validFrame(preset.button2) ||
      !validFrame(preset.neutral)) {
    Serial.println("Invalid CAN frame in active preset");
    return false;
  }
  if (canReady && canDriverBitrate == preset.bitrate) return true;
  if (canDriverInstalled) {
    twai_stop();
    twai_driver_uninstall();
    canDriverInstalled = false;
    canReady = false;
  }
  twai_general_config_t general =
      TWAI_GENERAL_CONFIG_DEFAULT(static_cast<gpio_num_t>(kCanTxPin),
                                  static_cast<gpio_num_t>(kCanRxPin), TWAI_MODE_NORMAL);
  general.tx_queue_len = 4;
  general.rx_queue_len = 0;
  twai_timing_config_t timing = {};
  if (!canTiming(preset.bitrate, timing)) {
    Serial.printf("Unsupported CAN bitrate: %lu\n",
                  static_cast<unsigned long>(preset.bitrate));
    return false;
  }
  // Reception is unused (rx_queue_len = 0); this SDK exposes an accept-all
  // filter macro, not an accept-none equivalent.
  const twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  const esp_err_t installResult = twai_driver_install(&general, &timing, &filter);
  if (installResult != ESP_OK) {
    Serial.printf("TWAI install failed: %s\n", esp_err_to_name(installResult));
    return false;
  }
  canDriverInstalled = true;
  if (twai_start() != ESP_OK) {
    twai_driver_uninstall();
    canDriverInstalled = false;
    return false;
  }
  canReady = true;
  canDriverBitrate = preset.bitrate;
  uint32_t alerts = 0;
  twai_reconfigure_alerts(TWAI_ALERT_TX_SUCCESS | TWAI_ALERT_TX_FAILED | TWAI_ALERT_BUS_OFF,
                          &alerts);
  Serial.printf("CAN ready: %lu bit/s\n", static_cast<unsigned long>(preset.bitrate));
  return true;
}

void switchToNextPreset() {
  const uint8_t nextIndex = (activePresetIndex + 1) % ButtonmapCan::kPresetCount;
  if (!savePresetIndex(nextIndex)) {
    Serial.println("Preset was not saved; selection unchanged");
    displayStatus("SAVE ERROR", TFT_RED);
    return;
  }
  activePresetIndex = nextIndex;
  if (activePreset().canConfigured) initialiseCan(activePreset());
  Serial.printf("Active preset: %s\n", activePreset().name);
  displayPresetStatus();
}

void queueClick(Action action) {
  const Preset& preset = activePreset();
  if (!preset.canConfigured) {
    Serial.printf("%s: CAN codes are not configured\n", preset.name);
    displayPresetStatus();
    return;
  }
  if (!canReady || neutralPending) return;
  const Frame& frame = action == Action::Button1 ? preset.button1 : preset.button2;
  if (transmitFrame(frame)) {
    neutralPending = true;
    neutralDueMs = millis() + preset.releaseDelayMs;
    displayStatus(action == Action::Button1 ? preset.button1Label
                                            : preset.button2Label,
                  TFT_GREEN);
  }
}

void updateButton(Button& button, uint32_t now) {
  const bool pressed = digitalRead(button.pin) == LOW;
  if (pressed != button.sampledPressed) {
    button.sampledPressed = pressed;
    button.lastSampleChangeMs = now;
  }
  if (button.stablePressed != button.sampledPressed &&
      now - button.lastSampleChangeMs >= kDebounceMs) {
    button.stablePressed = button.sampledPressed;
    if (button.stablePressed) queueClick(button.action);
  }
}

void updateCanAlerts() {
  uint32_t alerts = 0;
  if (twai_read_alerts(&alerts, 0) != ESP_OK) return;
  if (alerts & TWAI_ALERT_BUS_OFF) {
    canReady = false;
    Serial.println("K-CAN2 bus-off; transmission stopped");
    if (activePreset().canConfigured) displayStatus("CAN BUS-OFF", TFT_RED);
  }
  if (alerts & TWAI_ALERT_TX_FAILED) Serial.println("CAN frame was not acknowledged");
}

}  // namespace

void setup() {
  auto config = M5.config();
  M5.begin(config);
  Serial.begin(115200);
  pinMode(kButton1Pin, INPUT_PULLUP);
  pinMode(kButton2Pin, INPUT_PULLUP);
  firstButton.sampledPressed = firstButton.stablePressed =
      digitalRead(kButton1Pin) == LOW;
  secondButton.sampledPressed = secondButton.stablePressed = digitalRead(kButton2Pin) == LOW;
  activePresetIndex = loadPresetIndex();
  if (activePreset().canConfigured) initialiseCan(activePreset());
  Serial.printf("Active preset: %s\n", activePreset().name);
  displayPresetStatus();
}

void loop() {
  M5.update();
  const uint32_t now = millis();
  updateButton(firstButton, now);
  updateButton(secondButton, now);
  if (neutralPending && static_cast<int32_t>(now - neutralDueMs) >= 0) {
    const bool sent = transmitFrame(activePreset().neutral);
    neutralPending = false;
    if (sent) displayPresetStatus();
  }
  if (canReady) updateCanAlerts();
  if (M5.BtnA.wasPressed()) {
    if (neutralPending) {
      presetSwitchPending = true;
    } else {
      switchToNextPreset();
    }
  }
  if (presetSwitchPending && !neutralPending) {
    presetSwitchPending = false;
    switchToNextPreset();
  }
  delay(2);
}

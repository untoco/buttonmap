#include <M5Unified.h>
#include <buttonmap_config.h>
#include "driver/twai.h"

namespace {

using namespace ButtonmapConfig;

struct Button {
  uint8_t pin;
  uint8_t command;
  bool stablePressed;
  bool sampledPressed;
  uint32_t lastSampleChangeMs;

  Button(uint8_t pinValue, uint8_t commandValue)
      : pin(pinValue), command(commandValue), stablePressed(false), sampledPressed(false),
        lastSampleChangeMs(0) {}
};

Button previousButton{kPreviousButtonPin, kPreviousCommand};
Button nextButton{kNextButtonPin, kNextCommand};
bool canReady = false;
bool neutralPending = false;
uint32_t neutralDueMs = 0;

void displayStatus(const char* top, uint16_t colour) {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(colour, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.drawString(top, M5.Display.width() / 2, 32);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(1);
  M5.Display.drawString("K-CAN2 BUTTONMAP", M5.Display.width() / 2, 76);
}

bool transmitMedia(uint8_t command) {
  twai_message_t message = {};
  message.identifier = kMediaCanId;
  message.data_length_code = kMediaDlc;
  message.data[0] = command;
  message.data[1] = kMediaSecondByte;
  const esp_err_t result = twai_transmit(&message, pdMS_TO_TICKS(kTransmitTimeoutMs));
  if (result != ESP_OK) {
    Serial.printf("TX %02X failed: %s\n", command, esp_err_to_name(result));
    displayStatus("CAN TX ERROR", TFT_RED);
    return false;
  }
  Serial.printf("TX %03lX [%02X %02X]\n", static_cast<unsigned long>(kMediaCanId),
                command, kMediaSecondByte);
  return true;
}

bool initialiseCan() {
  twai_general_config_t general =
      TWAI_GENERAL_CONFIG_DEFAULT(static_cast<gpio_num_t>(kCanTxPin),
                                  static_cast<gpio_num_t>(kCanRxPin), TWAI_MODE_NORMAL);
  general.tx_queue_len = 4;
  general.rx_queue_len = 0;
  const twai_timing_config_t timing = TWAI_TIMING_CONFIG_100KBITS();
  // Reception is unused (rx_queue_len = 0); this SDK exposes an accept-all
  // filter macro, not an accept-none equivalent.
  const twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  const esp_err_t installResult = twai_driver_install(&general, &timing, &filter);
  if (installResult != ESP_OK) {
    Serial.printf("TWAI install failed: %s\n", esp_err_to_name(installResult));
    return false;
  }
  if (twai_start() != ESP_OK) {
    twai_driver_uninstall();
    return false;
  }
  uint32_t alerts = 0;
  twai_reconfigure_alerts(TWAI_ALERT_TX_SUCCESS | TWAI_ALERT_TX_FAILED | TWAI_ALERT_BUS_OFF,
                          &alerts);
  Serial.println("K-CAN2 ready: 100 kbit/s, transmit enabled");
  return true;
}

void queueClick(uint8_t command) {
  if (!canReady || neutralPending) return;
  if (transmitMedia(command)) {
    neutralPending = true;
    neutralDueMs = millis() + kReleaseDelayMs;
    displayStatus(command == kPreviousCommand ? "PREVIOUS" : "NEXT", TFT_GREEN);
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
    if (button.stablePressed) queueClick(button.command);
  }
}

void updateCanAlerts() {
  uint32_t alerts = 0;
  if (twai_read_alerts(&alerts, 0) != ESP_OK) return;
  if (alerts & TWAI_ALERT_BUS_OFF) {
    canReady = false;
    Serial.println("K-CAN2 bus-off; transmission stopped");
    displayStatus("CAN BUS-OFF", TFT_RED);
  }
  if (alerts & TWAI_ALERT_TX_FAILED) Serial.println("CAN frame was not acknowledged");
}

}  // namespace

void setup() {
  auto config = M5.config();
  M5.begin(config);
  Serial.begin(115200);
  pinMode(kPreviousButtonPin, INPUT_PULLUP);
  pinMode(kNextButtonPin, INPUT_PULLUP);
  previousButton.sampledPressed = previousButton.stablePressed =
      digitalRead(kPreviousButtonPin) == LOW;
  nextButton.sampledPressed = nextButton.stablePressed = digitalRead(kNextButtonPin) == LOW;
  canReady = initialiseCan();
  displayStatus(canReady ? "READY" : "CAN ERROR", canReady ? TFT_GREEN : TFT_RED);
}

void loop() {
  const uint32_t now = millis();
  updateButton(previousButton, now);
  updateButton(nextButton, now);
  if (neutralPending && static_cast<int32_t>(now - neutralDueMs) >= 0) {
    transmitMedia(kNeutralCommand);
    neutralPending = false;
    displayStatus("READY", TFT_GREEN);
  }
  if (canReady) updateCanAlerts();
  delay(2);
}

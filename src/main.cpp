#include <Arduino.h>
#include <HTTPClient.h>
#include <IRremote.hpp>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#if __has_include("secrets.h")
#include "secrets.h"
#define OYNK_HAS_LOCAL_CONFIG 1
#else
#define OYNK_HAS_LOCAL_CONFIG 0
#endif

namespace {
constexpr uint32_t kSerialBaud = 115200;
constexpr uint8_t kIrReceiverPin = 32;
constexpr uint32_t kWifiTimeoutMs = 20000;
constexpr uint32_t kRequestIntervalMs = 30000;
constexpr uint32_t kMaximumAmountNgn = 999999999;

enum class TerminalState : uint8_t {
  EnteringAmount,
  DraftReady,
};

uint32_t lastRequestAt = 0;
uint32_t amountNgn = 0;
TerminalState terminalState = TerminalState::EnteringAmount;
LiquidCrystal_I2C lcd(0x27, 16, 2);

void initializeDisplay() {
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
}

void initializeRemote() {
  IrReceiver.begin(kIrReceiverPin, DISABLE_LED_FEEDBACK);
  Serial.printf("IR receiver ready on GPIO %u\n", kIrReceiverPin);
}

void writeDisplayLine(uint8_t row, const char* text) {
  lcd.setCursor(0, row);
  for (uint8_t column = 0; column < 16; ++column) {
    const char character = text[column];
    if (character == '\0') {
      for (; column < 16; ++column) {
        lcd.print(' ');
      }
      return;
    }
    lcd.print(character);
  }
}

void showStatus(const char* lineOne, const char* lineTwo) {
  Serial.println();
  Serial.println("----------------");
  Serial.println(lineOne);
  Serial.println(lineTwo);
  Serial.println("----------------");

  writeDisplayLine(0, lineOne);
  writeDisplayLine(1, lineTwo);
}

void showAmountEntry() {
  char amountText[17];
  snprintf(amountText, sizeof(amountText), "NGN %lu",
           static_cast<unsigned long>(amountNgn));
  showStatus("ENTER AMOUNT", amountText);
}

int8_t digitForCommand(uint8_t command) {
  switch (command) {
    case 0x52:
      return 0;
    case 0x16:
      return 1;
    case 0x19:
      return 2;
    case 0x0D:
      return 3;
    case 0x0C:
      return 4;
    case 0x18:
      return 5;
    case 0x5E:
      return 6;
    case 0x08:
      return 7;
    case 0x1C:
      return 8;
    case 0x5A:
      return 9;
    default:
      return -1;
  }
}

void handleRemoteCommand(uint8_t command) {
  constexpr uint8_t kOkCommand = 0x40;
  constexpr uint8_t kBackCommand = 0x42;

  if (command == kBackCommand) {
    if (terminalState == TerminalState::DraftReady) {
      terminalState = TerminalState::EnteringAmount;
      amountNgn = 0;
    } else {
      amountNgn /= 10;
    }
    showAmountEntry();
    return;
  }

  if (command == kOkCommand) {
    if (amountNgn == 0) {
      showStatus("INVALID AMOUNT", "ENTER MORE THAN 0");
      return;
    }

    terminalState = TerminalState::DraftReady;
    char amountText[17];
    snprintf(amountText, sizeof(amountText), "NGN %lu",
             static_cast<unsigned long>(amountNgn));
    showStatus("DRAFT - NOT SENT", amountText);
    Serial.printf("Payment draft ready: amount_ngn=%lu; not submitted\n",
                  static_cast<unsigned long>(amountNgn));
    return;
  }

  const int8_t digit = digitForCommand(command);
  if (digit < 0 || terminalState != TerminalState::EnteringAmount) {
    return;
  }

  const uint32_t numericDigit = static_cast<uint32_t>(digit);
  if (amountNgn > (kMaximumAmountNgn - numericDigit) / 10) {
    showStatus("AMOUNT TOO LARGE", "MAX 999999999");
    return;
  }

  amountNgn = amountNgn * 10 + numericDigit;
  showAmountEntry();
}

#if OYNK_HAS_LOCAL_CONFIG
bool connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < kWifiTimeoutMs) {
    delay(250);
  }

  return WiFi.status() == WL_CONNECTED;
}

void checkAuthorizationService() {
  if (WiFi.status() != WL_CONNECTED && !connectWifi()) {
    showStatus("OYNK TERMINAL", "NETWORK OFFLINE");
    return;
  }

  if (OYNK_API_URL[0] == '\0') {
    showStatus("OYNK TERMINAL", "API NOT CONFIGURED");
    return;
  }

  WiFiClientSecure client;
  client.setCACert(OYNK_API_ROOT_CA);

  HTTPClient request;
  if (!request.begin(client, OYNK_API_URL)) {
    showStatus("OYNK TERMINAL", "REQUEST ERROR");
    return;
  }

  request.setTimeout(10000);
  const int status = request.GET();
  request.end();

  if (status >= 200 && status < 300) {
    showStatus("OYNK TERMINAL", "SERVICE READY");
  } else {
    showStatus("OYNK TERMINAL", "SERVICE UNAVAILABLE");
  }
}
#endif
}  // namespace

void setup() {
  Serial.begin(kSerialBaud);
  delay(500);
  initializeDisplay();
  initializeRemote();
  showStatus("OYNK TERMINAL", "BOOTING");

#if OYNK_HAS_LOCAL_CONFIG
  if (connectWifi()) {
    showStatus("OYNK TERMINAL", "NETWORK READY");
    checkAuthorizationService();
    showAmountEntry();
  } else {
    showStatus("OYNK TERMINAL", "NETWORK OFFLINE");
  }
#else
  showStatus("CONFIG REQUIRED", "SEE README");
#endif
}

void loop() {
  if (IrReceiver.decode()) {
    IrReceiver.printIRResultShort(&Serial);
    if ((IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) == 0) {
      handleRemoteCommand(IrReceiver.decodedIRData.command);
    }
    IrReceiver.resume();
  }

#if OYNK_HAS_LOCAL_CONFIG
  const uint32_t now = millis();
  if (OYNK_API_URL[0] != '\0' && now - lastRequestAt >= kRequestIntervalMs) {
    lastRequestAt = now;
    checkAuthorizationService();
  }
#endif
  delay(50);
}

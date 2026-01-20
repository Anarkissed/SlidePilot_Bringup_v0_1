#include <Arduino.h>
#include "HW/Display.h"
#include "BoardPins.h"

// Some TFT_* color names vary by graphics library/config.
// Define a sane default for TFT_GRAY if it's missing.
#ifndef TFT_GRAY
  #define TFT_GRAY 0x8410  // 16-bit RGB565 mid-gray
#endif

static void initPins() {
  pinMode(PIN_TFT_POWER, OUTPUT);
  digitalWrite(PIN_TFT_POWER, HIGH);

  pinMode(PIN_BTN_A, INPUT_PULLUP);
  pinMode(PIN_BTN_B, INPUT_PULLUP);
}

static void drawHeader() {
  Display.fillScreen(TFT_BLACK);
  Display.setTextSize(1);
  Display.setTextColor(TFT_WHITE, TFT_BLACK);
  Display.drawString("SlidePilot Bring-up", 6, 6);
  Display.setTextColor(TFT_GRAY, TFT_BLACK);
  Display.drawString("Display + buttons test", 6, 22);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  initPins();

  Display.init();
  Display.setRotation(1); // landscape
  Display.setBrightness(200); // 0..255

  drawHeader();

  Serial.println("SlidePilot bring-up started.");
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last < 150) return;
  last = millis();

  const bool aPressed = (digitalRead(PIN_BTN_A) == LOW);
  const bool bPressed = (digitalRead(PIN_BTN_B) == LOW);

  char line[64];
  snprintf(line, sizeof(line), "BTN A: %s   BTN B: %s", aPressed ? "DOWN" : "up", bPressed ? "DOWN" : "up");

  // Clear a strip and redraw line
  Display.fillRect(0, 50, Display.width(), 16, TFT_BLACK);
  Display.setTextColor(TFT_WHITE, TFT_BLACK);
  Display.drawString(line, 6, 50);

  // Simple heartbeat
  Display.fillRect(0, 70, Display.width(), 10, TFT_BLACK);
  Display.setTextColor(TFT_DARKGREEN, TFT_BLACK);
  snprintf(line, sizeof(line), "ms: %lu   heap: %lu", (unsigned long)millis(), (unsigned long)ESP.getFreeHeap());
  Display.drawString(line, 6, 70);

  Serial.printf("A=%d B=%d\n", aPressed, bPressed);
}

// WALKASSIST - grip touch sensor test.
//
// Wiring (AT42QT1010 breakout, Adafruit or SparkFun): VIN -> 3.3V, GND -> GND, OUT -> D2.
// Don't touch the pad while the board powers up (the chip calibrates then).
//
// Prints every touch and release with its duration, and any gestures.
// The Nano's orange LED (pin 13) lights while the sensor says "touched".
//
// Tests to run (Serial Monitor at 115200):
//   1. Tap the pad: "DOWN" then "UP after 0.1 s".
//   2. Grip through the cane's handle material. No trigger? Add copper tape
//      or a conductive sheet on the grip, wired to the board's touch pad.
//   3. Double tap and triple tap: "GESTURE: double tap" / "triple tap".
//   4. Hold test: grip and keep holding for 2+ minutes. A "still held" line
//      prints every 10 s. If "UP" appears while your hand is still on it,
//      the chip recalibrated during the long touch; note after how long.

#include <cane_touch.h>

const uint8_t TOUCH_PIN = 2;
const uint32_t HELD_REPORT_MS = 10000;
const uint32_t STATUS_MS = 2000;  // status line while nothing is happening

TouchSensor touch;
bool wasTouched = false;
bool wasConnected = false;
uint32_t nextHeldReport = 0;
uint32_t nextStatus = 0;
uint32_t longestMs = 0;
uint32_t highCount = 0;  // how often D2 has read HIGH since the last status line

void printBanner() {
  Serial.println("Touch test ready. Touch the pad.");
  Serial.println("Status every 2 s: if D2 never reads HIGH while the board's red LED");
  Serial.println("is on, the OUT wire isn't reaching the Nano's D2 pin.");
}

void setup() {
  Serial.begin(115200);
  uint32_t start = millis();
  while (!Serial && millis() - start < 3000) {}

  pinMode(LED_BUILTIN, OUTPUT);
  touch.begin(TOUCH_PIN);
}

void loop() {
  uint32_t now = millis();

  // Re-print the banner whenever a Serial Monitor connects, so a monitor
  // opened late doesn't sit blank.
  bool connected = (bool)Serial;
  if (connected && !wasConnected) printBanner();
  wasConnected = connected;

  Gesture g = touch.update(now);
  bool t = touch.touched();
  digitalWrite(LED_BUILTIN, t ? HIGH : LOW);
  if (digitalRead(TOUCH_PIN)) highCount++;

  if (!t && (int32_t)(now - nextStatus) >= 0) {
    Serial.print(now / 1000.0, 1);
    Serial.print(" s  status: D2 reads ");
    Serial.print(digitalRead(TOUCH_PIN) ? "HIGH" : "LOW");
    Serial.print(", HIGH ");
    Serial.print(highCount);
    Serial.println(" times since last status");
    highCount = 0;
    nextStatus = now + STATUS_MS;
  }

  if (t && !wasTouched) {
    Serial.print(now / 1000.0, 2);
    Serial.println(" s  DOWN");
    nextHeldReport = now + HELD_REPORT_MS;
  }
  if (!t && wasTouched) {
    uint32_t ms = touch.lastTouchMs();
    longestMs = max(longestMs, ms);
    Serial.print(now / 1000.0, 2);
    Serial.print(" s  UP after ");
    Serial.print(ms / 1000.0, 2);
    Serial.print(" s   (longest so far ");
    Serial.print(longestMs / 1000.0, 1);
    Serial.println(" s)");
  }
  if (t && (int32_t)(now - nextHeldReport) >= 0) {
    Serial.print("  still held: ");
    Serial.print(touch.heldMs(now) / 1000);
    Serial.println(" s");
    nextHeldReport += HELD_REPORT_MS;
  }
  if (g != GESTURE_NONE) {
    Serial.print(now / 1000.0, 2);
    Serial.print(" s  GESTURE: ");
    Serial.println(gestureName(g));
  }

  wasTouched = t;
}

// WALKASSIST - grip sensor test (round FSR on A0).
//
// Wiring (see docs/circuit.svg): FSR leg 1 -> 3V3, FSR leg 2 -> row 16 -> jumper
// to A0, 10 kOhm from row 16 to GND.
//
// Prints every touch and release with its duration, any gestures, and the
// grip pressure (0..1023). The Nano's orange LED (pin 13) lights while the
// grip counts as "touched".
//
// Tests to run (Serial Monitor at 115200):
//   1. No hand on the grip: status lines should show pressure near 0.
//      Rest your hand lightly, then grip normally, then lean hard: note the
//      pressures. PRESS_THRESHOLD (150) should sit between "resting" and
//      "gripping"; change it in cane_touch.h if not.
//   2. Grip and release: "DOWN" then "UP after ... s".
//   3. Double tap and triple tap: "GESTURE: double tap" / "triple tap".
//   4. Hold test: grip and keep holding for 2+ minutes. A "still held" line
//      prints every 10 s with the pressure. An FSR has no timeout, so "UP"
//      should only appear when you let go.

#include <cane_touch.h>

const uint8_t GRIP_PIN = A0;
const uint32_t HELD_REPORT_MS = 10000;
const uint32_t STATUS_MS = 1000;  // status line while nothing is happening

TouchSensor grip;
bool wasTouched = false;
bool wasConnected = false;
uint32_t nextHeldReport = 0;
uint32_t nextStatus = 0;
uint32_t longestMs = 0;
uint16_t peak = 0;  // highest pressure since the last report

void printBanner() {
  Serial.println("Grip (FSR on A0) test ready. Squeeze the grip.");
  Serial.print("Touched above ");
  Serial.print(TouchSensor::PRESS_THRESHOLD);
  Serial.print(", released below ");
  Serial.println(TouchSensor::RELEASE_THRESHOLD);
  Serial.println("If the pressure stays at 0 when squeezed, check the FSR, the row 16");
  Serial.println("jumper to A0, and the 10 kOhm resistor to GND.");
}

void setup() {
  Serial.begin(115200);
  uint32_t start = millis();
  while (!Serial && millis() - start < 3000) {}

  pinMode(LED_BUILTIN, OUTPUT);
  grip.begin(GRIP_PIN);
}

void loop() {
  uint32_t now = millis();

  // Re-print the banner whenever a Serial Monitor connects, so a monitor
  // opened late doesn't sit blank.
  bool connected = (bool)Serial;
  if (connected && !wasConnected) printBanner();
  wasConnected = connected;

  Gesture g = grip.update(now);
  bool t = grip.touched();
  digitalWrite(LED_BUILTIN, t ? HIGH : LOW);
  peak = max(peak, grip.pressure());

  if (!t && (int32_t)(now - nextStatus) >= 0) {
    Serial.print(now / 1000.0, 1);
    Serial.print(" s  status: pressure ");
    Serial.print(grip.pressure());
    Serial.print(" (peak ");
    Serial.print(peak);
    Serial.println(")");
    peak = 0;
    nextStatus = now + STATUS_MS;
  }

  if (t && !wasTouched) {
    Serial.print(now / 1000.0, 2);
    Serial.print(" s  DOWN  pressure ");
    Serial.println(grip.pressure());
    nextHeldReport = now + HELD_REPORT_MS;
    peak = 0;
  }
  if (!t && wasTouched) {
    uint32_t ms = grip.lastTouchMs();
    longestMs = max(longestMs, ms);
    Serial.print(now / 1000.0, 2);
    Serial.print(" s  UP after ");
    Serial.print(ms / 1000.0, 2);
    Serial.print(" s   (peak pressure ");
    Serial.print(peak);
    Serial.print(", longest touch so far ");
    Serial.print(longestMs / 1000.0, 1);
    Serial.println(" s)");
    peak = 0;
  }
  if (t && (int32_t)(now - nextHeldReport) >= 0) {
    Serial.print("  still held: ");
    Serial.print(grip.heldMs(now) / 1000);
    Serial.print(" s, pressure ");
    Serial.println(grip.pressure());
    nextHeldReport += HELD_REPORT_MS;
  }
  if (g != GESTURE_NONE) {
    Serial.print(now / 1000.0, 2);
    Serial.print(" s  GESTURE: ");
    Serial.println(gestureName(g));
  }

  wasTouched = t;
}

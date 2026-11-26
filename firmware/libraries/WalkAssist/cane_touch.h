// WALKASSIST - grip sensor: round FSR (force-sensitive resistor) on A0, with tap gestures.
//
// Wiring (see docs/circuit.svg):
//   FSR leg 1 -> 3V3
//   FSR leg 2 -> A0 (through the breadboard's free row 16 + a jumper)
//   10 kOhm   -> from A0's node (row 16) to GND
// A0 reads ~0 with no hand on the grip and rises as the grip is squeezed
// (0..1023, 10-bit). Unlike the old capacitive board, an FSR has no timeout:
// it reports a grip for as long as the hand presses.
//
// "Touched" = pressure above PRESS_THRESHOLD; it stays touched until the
// pressure drops below RELEASE_THRESHOLD (the gap stops flicker near the edge).
//
// Gestures (the hand must leave the grip first, then tap):
//   double tap  = two quick taps    -> cancel / "I'm OK" / accept baseline
//   triple tap  = three quick taps  -> re-calibrate
// A long press can't be a gesture on a cane: holding the cane while walking
// already looks like one long touch.
//
// Call update() as often as possible (every loop). It returns a gesture once,
// at the moment the gesture is complete, and GESTURE_NONE otherwise.

#pragma once
#include <Arduino.h>

enum Gesture : uint8_t {
  GESTURE_NONE = 0,
  GESTURE_DOUBLE_TAP = 2,
  GESTURE_TRIPLE_TAP = 3,
};

inline const char* gestureName(Gesture g) {
  switch (g) {
    case GESTURE_DOUBLE_TAP: return "double tap";
    case GESTURE_TRIPLE_TAP: return "triple tap";
    default: return "none";
  }
}

class TouchSensor {
 public:
  // Pressure thresholds (0..1023). Tune on the real grip with touch_test.
  static const uint16_t PRESS_THRESHOLD = 150;    // above this: touched
  static const uint16_t RELEASE_THRESHOLD = 100;  // below this: released

  // Timing, in ms. Tune these on the real grip.
  static const uint32_t SAMPLE_MS = 2;      // read A0 at most this often
  static const uint32_t DEBOUNCE_MS = 20;   // ignore flicker shorter than this
  static const uint32_t TAP_MAX_MS = 350;   // a touch longer than this is a grip, not a tap
  static const uint32_t GAP_MAX_MS = 450;   // max time between taps in one gesture

  void begin(uint8_t pin = A0) {
    pin_ = pin;
    pinMode(pin_, INPUT);
    analogReadResolution(10);               // 0..1023
    pressure_ = analogRead(pin_);
    raw_ = stable_ = pressure_ > PRESS_THRESHOLD;
    rawChangedAt_ = downAt_ = upAt_ = lastSampleAt_ = millis();
  }

  Gesture update(uint32_t now = millis()) {
    // Read and lightly smooth the pressure (1/4 new reading, 3/4 previous).
    if (now - lastSampleAt_ >= SAMPLE_MS) {
      lastSampleAt_ = now;
      pressure_ = (3 * pressure_ + analogRead(pin_)) / 4;
    }

    // Pressed / released with a gap between the two thresholds.
    bool r = raw_ ? (pressure_ >= RELEASE_THRESHOLD) : (pressure_ > PRESS_THRESHOLD);
    if (r != raw_) {
      raw_ = r;
      rawChangedAt_ = now;
    }

    // Accept a change only once it has been steady for DEBOUNCE_MS.
    if (raw_ != stable_ && now - rawChangedAt_ >= DEBOUNCE_MS) {
      stable_ = raw_;
      if (stable_) {
        downAt_ = now;                        // touch started
      } else {
        upAt_ = now;                          // touch ended
        lastTouchMs_ = now - downAt_;
        if (lastTouchMs_ <= TAP_MAX_MS) {
          taps_++;
        } else {
          taps_ = 0;                          // that was a grip, not a tap
        }
      }
    }

    // A tap sequence ends when the next touch turns into a grip (the user
    // grabs the cane again) or when no new tap comes within GAP_MAX_MS.
    if (taps_ > 0) {
      bool gripping = stable_ && now - downAt_ > TAP_MAX_MS;
      bool gapOver = !stable_ && now - upAt_ > GAP_MAX_MS;
      if (gripping || gapOver) {
        Gesture g = GESTURE_NONE;
        if (taps_ == 2) g = GESTURE_DOUBLE_TAP;
        if (taps_ == 3) g = GESTURE_TRIPLE_TAP;
        taps_ = 0;                            // 1 tap or 4+ taps: ignored
        return g;
      }
    }
    return GESTURE_NONE;
  }

  bool touched() const { return stable_; }

  // Smoothed grip pressure, 0 (no hand) .. 1023 (squeezed hard). Also a gait
  // signal: how hard the user leans on the cane.
  uint16_t pressure() const { return pressure_; }

  // How long the current touch has lasted (0 if not touched).
  uint32_t heldMs(uint32_t now = millis()) const { return stable_ ? now - downAt_ : 0; }

  // How long the most recent finished touch lasted.
  uint32_t lastTouchMs() const { return lastTouchMs_; }

 private:
  uint8_t pin_ = A0;
  uint16_t pressure_ = 0;
  bool raw_ = false;
  bool stable_ = false;
  uint32_t lastSampleAt_ = 0;
  uint32_t rawChangedAt_ = 0;
  uint32_t downAt_ = 0;
  uint32_t upAt_ = 0;
  uint32_t lastTouchMs_ = 0;
  uint8_t taps_ = 0;
};

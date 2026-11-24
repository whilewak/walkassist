// WALKASSIST - grip touch sensor (AT42QT1010 breakout: Adafruit 1374 or SparkFun SEN-12041) with tap gestures.
//
// Wiring: VIN -> 3.3V, GND -> GND, OUT -> D2. OUT is HIGH while touched.
// Don't touch the pad while the board powers up: the chip calibrates then.
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
  // Timing, in ms. Tune these on the real grip.
  static const uint32_t DEBOUNCE_MS = 20;   // ignore flicker shorter than this
  static const uint32_t TAP_MAX_MS = 350;   // a touch longer than this is a grip, not a tap
  static const uint32_t GAP_MAX_MS = 450;   // max time between taps in one gesture

  void begin(uint8_t pin) {
    pin_ = pin;
    pinMode(pin_, INPUT_PULLDOWN);  // reads "not touched" if the sensor is unplugged
    raw_ = stable_ = digitalRead(pin_);
    rawChangedAt_ = downAt_ = upAt_ = millis();
  }

  Gesture update(uint32_t now = millis()) {
    bool r = digitalRead(pin_);
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

  // How long the current touch has lasted (0 if not touched).
  uint32_t heldMs(uint32_t now = millis()) const { return stable_ ? now - downAt_ : 0; }

  // How long the most recent finished touch lasted.
  uint32_t lastTouchMs() const { return lastTouchMs_; }

 private:
  uint8_t pin_ = 2;
  bool raw_ = false;
  bool stable_ = false;
  uint32_t rawChangedAt_ = 0;
  uint32_t downAt_ = 0;
  uint32_t upAt_ = 0;
  uint32_t lastTouchMs_ = 0;
  uint8_t taps_ = 0;
};

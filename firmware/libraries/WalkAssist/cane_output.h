// WALKASSIST - non-blocking output patterns for the status LED and the buzzer.
//
// Wiring (see docs/circuit.svg):
//   Buzzer: D4 -> 1 kOhm -> 2N4401 base; buzzer between 3V3 and collector;
//           emitter -> GND. Active buzzer: on/off only, so sounds differ by timing.
//   RGB LED: D5 (red), D6 (green), D9 (blue), each through 220 Ohm;
//           common cathode -> GND (pass commonAnode = true if yours is common anode).
//
// Each output has two layers:
//   background - loops (or holds) until you change it: setBeep(), setRhythm(), setLight()
//   one-shot   - plays once on top, then the background resumes: playBeep(), flashLight()
// Call update() every loop; it never blocks, so sensing keeps running.
//
// Sounds (README, Phase 7):
//   ACK        one short chirp            gesture acknowledged
//   CONFIRM    double beep                calibration done / accepted
//   SLIP       one short beep             tip slip
//   NEAR_FALL  three quick beeps          stumble
//   REST       slow double beep every 4 s fatigue: "take a rest"
//   STOP_REST  three firm beeps, pause    high risk: "stop and rest"
//   LOCATOR    two chirps every 2 s       dropped cane: "find me"
//   COUNTDOWN  beeps getting faster       fall detected, double tap to cancel
//   ALARM      continuous                 fall not cancelled
//   RHYTHM     short tick every N ms      unsteady gait: tick at the user's own cadence
// Lights:
//   GREEN stable, AMBER rising risk, RED high risk, AMBER_FLASH / RED_FLASH
//   warnings, LOCATE blinking white ("find me"), WHITE (used for short flashes).

#pragma once
#include <Arduino.h>

enum class Beep : uint8_t {
  OFF, ACK, CONFIRM, SLIP, NEAR_FALL, REST, STOP_REST, LOCATOR, COUNTDOWN, ALARM, RHYTHM
};

enum class Light : uint8_t {
  OFF, GREEN, AMBER, RED, AMBER_FLASH, RED_FLASH, LOCATE, WHITE
};

inline const char* beepName(Beep b) {
  switch (b) {
    case Beep::OFF: return "off";
    case Beep::ACK: return "ack chirp";
    case Beep::CONFIRM: return "confirm double beep";
    case Beep::SLIP: return "slip beep";
    case Beep::NEAR_FALL: return "near-fall triple beep";
    case Beep::REST: return "rest (slow double beep)";
    case Beep::STOP_REST: return "stop and rest";
    case Beep::LOCATOR: return "locator chirps";
    case Beep::COUNTDOWN: return "fall countdown";
    case Beep::ALARM: return "alarm";
    case Beep::RHYTHM: return "rhythm ticks";
  }
  return "?";
}

inline const char* lightName(Light l) {
  switch (l) {
    case Light::OFF: return "off";
    case Light::GREEN: return "green";
    case Light::AMBER: return "amber";
    case Light::RED: return "red";
    case Light::AMBER_FLASH: return "amber flashing";
    case Light::RED_FLASH: return "red flashing";
    case Light::LOCATE: return "white blinking";
    case Light::WHITE: return "white";
  }
  return "?";
}

class CaneOutput {
 public:
  // Timing (ms). Tune by ear.
  static const uint16_t TICK_MS = 25;              // rhythm tick length
  static const uint16_t COUNTDOWN_BEEP_MS = 100;
  static const uint16_t COUNTDOWN_GAP_START = 900; // gap at the start of the countdown
  static const uint16_t COUNTDOWN_GAP_END = 150;   // gap when the countdown runs out
  static const uint32_t COUNTDOWN_MS = 15000;      // cancel window

  void begin(uint8_t buzzerPin = 4, uint8_t rPin = 5, uint8_t gPin = 6, uint8_t bPin = 9,
             bool commonAnode = false) {
    buzPin_ = buzzerPin; rPin_ = rPin; gPin_ = gPin; bPin_ = bPin;
    commonAnode_ = commonAnode;
    pinMode(buzPin_, OUTPUT);
    pinMode(rPin_, OUTPUT); pinMode(gPin_, OUTPUT); pinMode(bPin_, OUTPUT);
    writeBuzzer(false);
    writeColor(0, 0, 0);
  }

  // 0..255, scales every colour (default 255).
  void setBrightness(uint8_t b) { brightness_ = b; }

  // ---- buzzer ----
  void setBeep(Beep b, uint32_t now = millis()) {
    if (b == bg_.beep && b != Beep::COUNTDOWN) return;   // already playing: don't restart
    start(bg_, b, now);
  }
  void setRhythm(uint16_t intervalMs, uint32_t now = millis()) {
    rhythmMs_ = intervalMs < TICK_MS * 4 ? TICK_MS * 4 : intervalMs;
    if (bg_.beep != Beep::RHYTHM) start(bg_, Beep::RHYTHM, now);
  }
  void playBeep(Beep b, uint32_t now = millis()) { start(fg_, b, now); }
  Beep beep() const { return bg_.beep; }
  bool countdownDone(uint32_t now = millis()) const {
    return bg_.beep == Beep::COUNTDOWN && now - bg_.startedAt >= COUNTDOWN_MS;
  }

  // ---- LED ----
  void setLight(Light l, uint32_t now = millis()) {
    if (l == light_) return;
    light_ = l;
    lightStart_ = now;
  }
  void flashLight(Light l, uint16_t ms, uint32_t now = millis()) {
    flash_ = l;
    flashStart_ = now;
    flashMs_ = ms;
  }
  Light light() const { return light_; }

  // Stop everything (e.g. before sleeping).
  void silence(uint32_t now = millis()) {
    start(bg_, Beep::OFF, now);
    start(fg_, Beep::OFF, now);
    setLight(Light::OFF, now);
    flashMs_ = 0;
  }

  void update(uint32_t now = millis()) {
    // Buzzer: a running one-shot wins over the background.
    bool fgOn = false;
    bool fgActive = advance(fg_, now, fgOn);
    bool bgOn = false;
    advance(bg_, now, bgOn);
    writeBuzzer(fgActive ? fgOn : bgOn);

    // LED: a running flash wins over the background.
    Light show = light_;
    uint32_t since = now - lightStart_;
    if (flashMs_ > 0) {
      if (now - flashStart_ < flashMs_) {
        show = flash_;
        since = now - flashStart_;
      } else {
        flashMs_ = 0;
      }
    }
    showLight(show, since);
  }

  // For tests: what the outputs are doing right now.
  bool buzzerOn() const { return buzzerOn_; }
  uint8_t red() const { return r_; }
  uint8_t green() const { return g_; }
  uint8_t blue() const { return b_; }

 private:
  struct Track {
    Beep beep = Beep::OFF;
    uint32_t startedAt = 0;   // when the pattern started
    uint32_t stepAt = 0;      // when the current step started
    uint8_t step = 0;         // even = sound on, odd = silent
  };

  // Fixed patterns: alternating on / off durations in ms, starting with "on".
  // A 0 means "skip this step".
  struct Pattern { const uint16_t* steps; uint8_t n; bool loop; };

  static Pattern pattern(Beep b) {
    static const uint16_t ack[] = {40};
    static const uint16_t confirm[] = {80, 100, 80};
    static const uint16_t slip[] = {60};
    static const uint16_t nearFall[] = {60, 60, 60, 60, 60};
    static const uint16_t rest[] = {80, 120, 80, 3720};
    static const uint16_t stopRest[] = {200, 200, 200, 200, 200, 1800};
    static const uint16_t locator[] = {60, 80, 60, 1800};
    static const uint16_t alarm[] = {1000, 0};
    switch (b) {
      case Beep::ACK: return {ack, 1, false};
      case Beep::CONFIRM: return {confirm, 3, false};
      case Beep::SLIP: return {slip, 1, false};
      case Beep::NEAR_FALL: return {nearFall, 5, false};
      case Beep::REST: return {rest, 4, true};
      case Beep::STOP_REST: return {stopRest, 6, true};
      case Beep::LOCATOR: return {locator, 4, true};
      case Beep::ALARM: return {alarm, 2, true};
      default: return {nullptr, 0, false};   // OFF, and the computed ones below
    }
  }

  void start(Track& t, Beep b, uint32_t now) {
    t.beep = b;
    t.startedAt = t.stepAt = now;
    t.step = 0;
  }

  // Length of step `i` of track `t` (0 = skip). Handles the computed patterns.
  uint16_t stepLen(const Track& t, uint8_t i, uint32_t now) const {
    if (t.beep == Beep::RHYTHM) return (i % 2 == 0) ? TICK_MS : rhythmMs_ - TICK_MS;
    if (t.beep == Beep::COUNTDOWN) {
      if (i % 2 == 0) return COUNTDOWN_BEEP_MS;
      uint32_t el = now - t.startedAt;
      if (el > COUNTDOWN_MS) el = COUNTDOWN_MS;
      return COUNTDOWN_GAP_START - (uint32_t)(COUNTDOWN_GAP_START - COUNTDOWN_GAP_END) * el / COUNTDOWN_MS;
    }
    return pattern(t.beep).steps[i];
  }

  uint8_t stepCount(const Track& t) const {
    if (t.beep == Beep::RHYTHM || t.beep == Beep::COUNTDOWN) return 2;
    return pattern(t.beep).n;
  }

  bool loops(const Track& t) const {
    if (t.beep == Beep::RHYTHM || t.beep == Beep::COUNTDOWN) return true;
    return pattern(t.beep).loop;
  }

  // Moves the track forward to `now`. Returns false once a one-shot has finished
  // (or for OFF); `on` says whether the buzzer should sound.
  bool advance(Track& t, uint32_t now, bool& on) {
    on = false;
    if (t.beep == Beep::OFF) return false;
    uint8_t n = stepCount(t);
    for (uint8_t guard = 0; guard < 16; guard++) {
      uint16_t len = stepLen(t, t.step, now);
      if (now - t.stepAt < len) {
        on = (t.step % 2 == 0);
        return true;
      }
      t.stepAt += len;
      t.step++;
      if (t.step >= n) {
        if (!loops(t)) { t.beep = Beep::OFF; return false; }
        t.step = 0;
      }
    }
    t.stepAt = now;   // fell far behind (e.g. update() not called for a while): resync
    return true;
  }

  void showLight(Light l, uint32_t since) {
    switch (l) {
      case Light::OFF: writeColor(0, 0, 0); break;
      case Light::GREEN: writeColor(0, 255, 0); break;
      case Light::AMBER: writeColor(255, 80, 0); break;
      case Light::RED: writeColor(255, 0, 0); break;
      case Light::WHITE: writeColor(255, 255, 255); break;
      case Light::AMBER_FLASH: blink(since, 150, 150, 255, 80, 0); break;
      case Light::RED_FLASH: blink(since, 250, 250, 255, 0, 0); break;
      case Light::LOCATE: blink(since, 150, 850, 255, 255, 255); break;
    }
  }

  void blink(uint32_t since, uint16_t onMs, uint16_t offMs, uint8_t r, uint8_t g, uint8_t b) {
    if (since % (onMs + offMs) < onMs) writeColor(r, g, b);
    else writeColor(0, 0, 0);
  }

  void writeColor(uint8_t r, uint8_t g, uint8_t b) {
    r = (uint16_t)r * brightness_ / 255;
    g = (uint16_t)g * brightness_ / 255;
    b = (uint16_t)b * brightness_ / 255;
    if (r == r_ && g == g_ && b == b_ && colorWritten_) return;
    r_ = r; g_ = g; b_ = b; colorWritten_ = true;
    analogWrite(rPin_, commonAnode_ ? 255 - r : r);
    analogWrite(gPin_, commonAnode_ ? 255 - g : g);
    analogWrite(bPin_, commonAnode_ ? 255 - b : b);
  }

  void writeBuzzer(bool on) {
    if (on == buzzerOn_ && buzzerWritten_) return;
    buzzerOn_ = on; buzzerWritten_ = true;
    digitalWrite(buzPin_, on ? HIGH : LOW);
  }

  uint8_t buzPin_ = 4, rPin_ = 5, gPin_ = 6, bPin_ = 9;
  bool commonAnode_ = false;
  uint8_t brightness_ = 255;

  Track bg_, fg_;
  uint16_t rhythmMs_ = 1000;

  Light light_ = Light::OFF;
  uint32_t lightStart_ = 0;
  Light flash_ = Light::OFF;
  uint32_t flashStart_ = 0;
  uint16_t flashMs_ = 0;

  bool buzzerOn_ = false, buzzerWritten_ = false;
  uint8_t r_ = 0, g_ = 0, b_ = 0;
  bool colorWritten_ = false;
};

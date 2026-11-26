// WALKASSIST - output test: every LED colour and buzzer pattern.
//
// Wiring (see docs/circuit.svg):
//   Buzzer driver: D4 -> 1 kOhm -> 2N4401 base, buzzer + -> 3V3, buzzer - -> collector,
//                  emitter -> GND.
//   RGB LED: D5 red, D6 green, D9 blue (220 Ohm each), common leg -> GND.
//   Common-anode LED? Set COMMON_ANODE to true.
//
// Open the Serial Monitor at 115200 and type a key + Enter:
//
//   d  demo: steps through every cane situation from the plan, 6 s each
//   s  stop everything
//
//   Sounds (background, loop until changed):        One-shots (play once on top):
//     5 rest   6 stop-and-rest   7 locator             1 ack chirp   2 confirm
//     8 fall countdown   9 alarm   t rhythm ticks      3 slip beep   4 near-fall
//     +/- rhythm faster / slower (100 ms steps)
//
//   Lights:  g green   a amber   r red   A amber flashing   R red flashing
//            l locate (white blinking)   w white flash (one-shot)   o off
//
// The pattern player never blocks: the orange LED (pin 13) blinks steadily the
// whole time to show loop() keeps running.

#include <cane_output.h>

const uint8_t BUZZER_PIN = 4;
const uint8_t LED_R = 5, LED_G = 6, LED_B = 9;
const bool COMMON_ANODE = false;
const uint32_t DEMO_STEP_MS = 6000;

CaneOutput out;
uint16_t rhythmMs = 1000;

bool demo = false;
uint8_t demoStep = 0;
uint32_t demoNext = 0;
bool wasConnected = false;

void printMenu() {
  Serial.println("Output test. d = demo, s = stop.");
  Serial.println("Sounds: 1 ack, 2 confirm, 3 slip, 4 near-fall (one-shots);");
  Serial.println("        5 rest, 6 stop-and-rest, 7 locator, 8 countdown, 9 alarm, t rhythm, +/- speed");
  Serial.println("Lights: g green, a amber, r red, A amber flash, R red flash, l locate, w white flash, o off");
}

// One step of the demo = one row of the plan's "conclusions -> responses" table.
void runDemoStep(uint8_t i, uint32_t now) {
  out.silence(now);
  switch (i) {
    case 0: Serial.println("demo: stable -> green, silent");
            out.setLight(Light::GREEN, now); break;
    case 1: Serial.println("demo: gesture -> ack chirp + white flash, back to green");
            out.setLight(Light::GREEN, now); out.playBeep(Beep::ACK, now);
            out.flashLight(Light::WHITE, 150, now); break;
    case 2: Serial.println("demo: calibration done -> double beep + green flash");
            out.setLight(Light::GREEN, now); out.playBeep(Beep::CONFIRM, now); break;
    case 3: Serial.println("demo: unsteady gait -> amber + rhythm ticks (900 ms)");
            out.setLight(Light::AMBER, now); out.setRhythm(900, now); break;
    case 4: Serial.println("demo: fatigue -> amber + rest pattern");
            out.setLight(Light::AMBER, now); out.setBeep(Beep::REST, now); break;
    case 5: Serial.println("demo: tip slip -> one beep + amber flash");
            out.setLight(Light::GREEN, now); out.playBeep(Beep::SLIP, now);
            out.flashLight(Light::AMBER, 600, now); break;
    case 6: Serial.println("demo: near-fall -> triple beep + amber flashing");
            out.setLight(Light::AMBER_FLASH, now); out.playBeep(Beep::NEAR_FALL, now); break;
    case 7: Serial.println("demo: high risk -> red + stop-and-rest beeps");
            out.setLight(Light::RED, now); out.setBeep(Beep::STOP_REST, now); break;
    case 8: Serial.println("demo: dropped cane -> locator chirps + white blinking");
            out.setLight(Light::LOCATE, now); out.setBeep(Beep::LOCATOR, now); break;
    case 9: Serial.println("demo: fall -> red flashing + countdown (beeps speed up)");
            out.setLight(Light::RED_FLASH, now); out.setBeep(Beep::COUNTDOWN, now); break;
    case 10: Serial.println("demo: fall not cancelled -> alarm");
             out.setLight(Light::RED_FLASH, now); out.setBeep(Beep::ALARM, now); break;
  }
}
const uint8_t DEMO_STEPS = 11;

void handleKey(char c, uint32_t now) {
  if (c == '\n' || c == '\r' || c == ' ') return;
  if (c != 'd') demo = false;
  switch (c) {
    case 'd': demo = true; demoStep = 0; demoNext = now; Serial.println("demo on (any key stops it)"); return;
    case 's': out.silence(now); Serial.println("stopped"); return;
    case '1': out.playBeep(Beep::ACK, now); break;
    case '2': out.playBeep(Beep::CONFIRM, now); break;
    case '3': out.playBeep(Beep::SLIP, now); break;
    case '4': out.playBeep(Beep::NEAR_FALL, now); break;
    case '5': out.setBeep(Beep::REST, now); break;
    case '6': out.setBeep(Beep::STOP_REST, now); break;
    case '7': out.setBeep(Beep::LOCATOR, now); break;
    case '8': out.setBeep(Beep::COUNTDOWN, now); break;
    case '9': out.setBeep(Beep::ALARM, now); break;
    case 't': out.setRhythm(rhythmMs, now); break;
    case '+': rhythmMs = rhythmMs > 400 ? rhythmMs - 100 : 300; out.setRhythm(rhythmMs, now); break;
    case '-': rhythmMs = rhythmMs < 2000 ? rhythmMs + 100 : 2000; out.setRhythm(rhythmMs, now); break;
    case 'g': out.setLight(Light::GREEN, now); break;
    case 'a': out.setLight(Light::AMBER, now); break;
    case 'r': out.setLight(Light::RED, now); break;
    case 'A': out.setLight(Light::AMBER_FLASH, now); break;
    case 'R': out.setLight(Light::RED_FLASH, now); break;
    case 'l': out.setLight(Light::LOCATE, now); break;
    case 'w': out.flashLight(Light::WHITE, 200, now); break;
    case 'o': out.setLight(Light::OFF, now); break;
    default: Serial.print("unknown key: "); Serial.println(c); printMenu(); return;
  }
  Serial.print("buzzer: ");
  Serial.print(beepName(out.beep()));
  if (out.beep() == Beep::RHYTHM) { Serial.print(" every "); Serial.print(rhythmMs); Serial.print(" ms"); }
  Serial.print("   light: ");
  Serial.println(lightName(out.light()));
}

void setup() {
  Serial.begin(115200);
  uint32_t start = millis();
  while (!Serial && millis() - start < 3000) {}
  pinMode(LED_BUILTIN, OUTPUT);
  out.begin(BUZZER_PIN, LED_R, LED_G, LED_B, COMMON_ANODE);
  out.setLight(Light::GREEN);
}

void loop() {
  uint32_t now = millis();

  bool connected = (bool)Serial;
  if (connected && !wasConnected) printMenu();
  wasConnected = connected;

  while (Serial.available()) handleKey((char)Serial.read(), now);

  if (demo && (int32_t)(now - demoNext) >= 0) {
    runDemoStep(demoStep, now);
    demoStep = (demoStep + 1) % DEMO_STEPS;
    demoNext = now + DEMO_STEP_MS;
  }

  out.update(now);
  digitalWrite(LED_BUILTIN, (now / 250) % 2);   // heartbeat: loop() never blocks
}

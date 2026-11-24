# WALKASSIST — Project Plan

A smart cane that warns users before a fall by comparing their walking to their own baseline.

## Design decisions (locked)

| Topic | Decision |
|---|---|
| Inputs | Capacitive touch on the grip + 2 IMUs (onboard LSM6DS3 at the handle, second IMU near the tip over I2C) |
| Outputs | Vibration motor (in-hand cues), RGB LED (status), **active buzzer** (fall alarm, locator) |
| Phone link | **WiFi access point** hosted by the Nano: the phone joins the cane's network and opens a status page. No app, no internet, works on any phone. (The Nano can't run WiFi and BLE at the same time, so we use one.) |
| Power | **9 V battery** → Nano VIN (accepts ~4.5–21 V). Motor, LED, buzzer run from the Nano's 3.3 V pin through transistors. |
| Baseline | 5-min calibration walk (30–60 s in **demo mode**), user confirms it, then slow updates during stable walking; saved to flash at the end of each walk |
| Gestures | **Double tap** = cancel / "I'm OK" / accept baseline · **Long press (2 s)** = re-calibrate. No SOS. |
| Processing | No ML. Per-step features + z-scores on the fly; only the last ~30 **steps** kept in RAM. All sensing, scoring and alerts run on the cane. |

---

## Phase 0 — Parts (order the missing ones 11/20, fastest shipping)

### Have — used in the project

| Part | Role |
|---|---|
| MPU-6050 (6-DOF) | IMU #2 near the tip (I2C `0x68`) |
| Adafruit AT42QT1010 capacitive touch breakout | Grip sensor (main) |
| SparkFun SEN-12041 (AT42QT1011) capacitive touch breakout | Spare grip sensor |
| Active buzzer | Audio output |
| 9 V battery with wires | Power → Nano VIN |
| Adafruit 9-DOF (L3GD20H + LSM303) | Backup tip IMU if the MPU-6050 fails |

### Have — not needed (keep out of scope)

Pulse sensor, TMP36 temperature sensor, APDS-9960, sound impact sensor, ADXL335, 3.7 V LiPo (needs a charger + 5 V boost to power the Nano; the 9 V battery replaces it).

### Need to get

| Part | Purpose | Est. cost |
|---|---|---|
| Coin vibration motor, 3 V (buy 2) | Haptic output | $2–4 |
| 2 × 2N2222 NPN transistor, 1N4148 diode, 2 × 1 kΩ resistor | Motor + buzzer drivers | $1 |
| RGB LED, common cathode (or red + green LEDs) + 3 × 220 Ω resistors | Status light | $1 |
| Spare 9 V batteries (2), small on/off switch | Power | $6 |
| 4-conductor wire (~1.5 m) | Tip IMU cable down the shaft | $2 |
| Heat shrink, zip ties, Velcro, small project box | Mounting | $5 |
| Copper / conductive tape | Touch electrode on the grip | $2 |

- [ ] Check the buzzer is **active**: it beeps when connected straight to 3.3 V (a passive one only clicks).
- [ ] Keep receipts for the final budget slide.
- [ ] Check you have: breadboard, jumper wires, soldering iron, micro-USB cable, multimeter.

---

## Phase 1 — Bench prototype, one part at a time (11/20 – 11/22)

### 1.1 Environment
- [ ] Arduino IDE, board package **Arduino SAMD Boards**.
- [ ] Libraries: `Arduino_LSM6DS3`, `Adafruit MPU6050` + `Adafruit Unified Sensor`, `WiFiNINA`, `FlashStorage`.
- [ ] Git repo for firmware + logging scripts.

### 1.2 Handle IMU (onboard)
- [ ] Read accel + gyro at 104 Hz; print to Serial.
- [ ] Note which axis points down the shaft when the cane is upright.

### 1.3 Tip IMU (MPU-6050 over I2C)
- [ ] Wire: VCC → 3.3 V, GND → GND, SDA → A4, SCL → A5. Address `0x68` (no clash with the onboard IMU at `0x6A`).
- [ ] Run an I2C scanner: expect `0x60` (crypto chip), `0x68`, `0x6A`.
- [ ] Read both IMUs in one loop at a fixed ~104 Hz (`micros()` timing, no `delay()`).
- [ ] MPU-6050: accel range ±8 g, low-pass filter ~44 Hz.
- [ ] **Test early with the full ~1.2 m wire.** If reads fail: keep I2C at 100 kHz (or lower), twist SDA and SCL each with a GND wire, keep away from the motor wires.

### 1.4 Touch sensor + gestures
- [ ] AT42QT1010 (Adafruit): VIN → 3.3 V, GND, OUT → D2 (HIGH when touched).
- [ ] Test through the grip material; if it doesn't trigger, solder a wire from the board's pad to copper tape on the grip.
- [ ] **Hold it for 2+ minutes.** These chips can recalibrate during a long touch and drop the output while still held. If that happens, treat a release with no motion change as "still held", or try the SparkFun AT42QT1011 board.
- [ ] Gestures: **double tap** (two touches < 400 ms apart), **long press** (> 2 s). Make sure normal gripping while walking doesn't count as a tap.

### 1.5 Outputs
- [ ] **Vibration motor:** D3 → 1 kΩ → 2N2222 base; emitter → GND; motor between 3.3 V and collector; 1N4148 across the motor (stripe toward 3.3 V). Test PWM strength and patterns.
- [ ] **Active buzzer:** D4 → 1 kΩ → 2N2222 base; buzzer between 3.3 V and collector. On/off only; vary the **beep timing** to tell sounds apart (short chirps = locator, fast continuous = alarm).
- [ ] **RGB LED:** D5 / D6 / D9 through 220 Ω each. Green, amber (red + some green), red, blinking.

### 1.6 Power
- [ ] 9 V clip (through the switch) → VIN and GND. Confirm the Nano boots with USB unplugged.
- [ ] Never connect 9 V to the 3.3 V pin or to the motor/buzzer directly.
- [ ] Measure current while running (multimeter in series) to estimate battery life; bring spare batteries to the demo either way.

---

## Phase 2 — Mount on the cane (11/22 – 11/23)

- [ ] Handle box: Nano, buzzer, LED (visible from above), 9 V battery + switch (Velcro).
- [ ] Touch pad on top of the grip where the palm rests; vibration motor under the palm.
- [ ] Tip IMU ~10–15 cm above the rubber tip, mounted **rigidly** (hot glue + zip ties + heat shrink).
- [ ] 4-wire cable down the shaft, zip-tied every ~15 cm, strain relief at both ends.
- [ ] Shake test and drop test onto a soft surface: nothing comes loose, I2C keeps reading.
- [ ] Photos for the final slides and video.

---

## Phase 3 — Data collection (11/23 – 11/24)

- [ ] Firmware **logger mode**: stream `t, ax1…gz1, ax2…gz2, touch` as CSV over USB Serial at ~104 Hz.
- [ ] Python script on the laptop: save each session as a labeled CSV and plot it.
- [ ] Record labeled sessions (each teammate, several times):
  - [ ] Normal walk (5 min once, for testing the real calibration; 2 min otherwise)
  - [ ] Slow / tired walk, shuffling
  - [ ] Unsteady walk (knee brace or ankle weight, irregular rhythm)
  - [ ] Turns (90° and 180°)
  - [ ] Standing still, leaning on the cane
  - [ ] Simulated stumble and tip slip (safely, on a mat)
  - [ ] Cane dropped (released before impact)
  - [ ] Cane laid down while still gripped (simulated fall)
  - [ ] Grip lost at impact, then re-gripped within 5 s
  - [ ] Cane carried, in a car / bag, leaned against a wall
  - [ ] Double taps and long presses, while standing and while walking
- [ ] Mark events (key press or video timestamps) so the data can be labeled.

---

## Phase 4 — Gait features (11/24 – 11/26)

Prototype in Python on the recordings first, then port to Arduino C++.

- [ ] **Step (tip-strike) detection:** peak in tip-IMU acceleration magnitude above a threshold, minimum ~300 ms between strikes.
- [ ] **Features over the last ~30 steps** (ring buffer, a few floats per step):
  - [ ] Cadence (strikes / min)
  - [ ] Stride-time variability (std ÷ mean of time between strikes)
  - [ ] Swing angle (gyro-integrated shaft angle between strikes) → relative speed = cadence × swing
  - [ ] Lateral sway (variance of handle side-to-side tilt)
  - [ ] Shuffling (low swing + tip vibration between strikes)
  - [ ] Turn time and steps per turn (gyro yaw)
- [ ] **Instant events:** tip slip (sideways jolt at the tip during contact), stumble (jolt then fast re-plant), impact + final orientation (flat and still).
- [ ] **IMU 1-only fallback:** step detection and features from the handle IMU alone, used if the tip IMU stops responding. Check on the recordings how much worse it is.
- [ ] Check each feature separates normal vs. unsteady walks in the recordings.

---

## Phase 5 — Grip + motion state machine (11/26 – 11/27)

States: `WALKING`, `STANDING`, `CARRIED`, `PARKED`, `DROPPED`, `REGRIP_WAIT`, `FALL_CANCEL`, `FALL_ALARM`, `CALIBRATING`.

| Grip | Motion | Means | Cane does |
|---|---|---|---|
| Held | Rhythmic tip strikes | Walking | Analyze gait |
| Held | Still, upright | Standing | Watch for sway |
| Released | Moving, no strikes | Carried, in a car | Ignore (never scored as walking) |
| Released | Upright, still | Parked | Sleep (low power) |
| Released before impact | Lies flat, no re-grip | Dropped cane | Locator chirp, blinking LED |
| Held through impact | Ends flat and still | Person fell | Fall alarm with 15 s cancel |
| Lost at impact | Ends flat and still | Unclear | Wait **5 s** for re-grip. Re-gripped → resume, log a stumble. Not re-gripped → fall alarm with 15 s cancel |

- [ ] Fall flow: `FALL_CANCEL` (15 s: red LED, strong repeated haptic, buzzer countdown beeps) → double tap cancels → otherwise `FALL_ALARM` (continuous buzzer, red flashing, alert on the phone page).
- [ ] **Parked → sleep:** lower the IMU rate, turn off the LED, wake on grip.
- [ ] Test every row against the recordings; tune thresholds.

---

## Phase 6 — Baseline and risk score (11/27 – 11/28)

- [ ] **Calibration:** walk 5 min (demo mode: 30–60 s) → store mean and std of each feature.
- [ ] **User confirms:** when calibration finishes, the cane gives a double haptic pulse; **double tap** to accept, **long press** to redo.
- [ ] **Re-calibrate any time:** long press (while standing still, so it isn't triggered while walking).
- [ ] **Adapt:** update mean/std slowly (exponential moving average) only while the score is green.
- [ ] **Save** the baseline to flash after calibration and at the end of each walk (not continuously; flash wears out).
- [ ] **Risk score 0–100:** weighted sum of each feature's z-score (weight stride-time variability most), clipped to 0–100.
  - Green < 30 · Amber 30–60 · Red = fall, or score > 60 sustained (start with 10 s; tune).
- [ ] Hysteresis so the level doesn't flicker on and off.

---

## Phase 7 — Conclusions → responses (11/28 – 11/29)

| Conclusion | Evidence (vs. own baseline) | Cane responds |
|---|---|---|
| Stable | All features in normal range | Green LED, no haptic |
| Unsteady gait | Stride-time variability or lateral sway above baseline | Amber LED, haptic rhythm cue at the user's own cadence |
| Fatigue | Cadence + swing angle trending down over the session, more shuffling | Amber LED, "rest" haptic (slow double pulse) |
| Slippery / uneven surface | Tip-slip jolt | Short warning pulse, amber LED |
| Near-fall | Stumble (jolt, then fast re-plant) | Strong haptic burst, amber LED, logged for the caregiver |
| High risk, no fall | Score > 60 sustained | **Red LED, strong repeated "stop and rest" haptic**, logged + warning on the phone page. No buzzer. |
| Dropped cane | See Phase 5 | Locator chirps, blinking LED |
| Fall | Held through impact, flat and still (or no re-grip within 5 s) | Red LED, buzzer, 15 s cancel, then phone alert |

- [ ] Non-blocking pattern player for haptic / LED / buzzer (no `delay()`, sensing never stops).
- [ ] Haptic acknowledges every gesture (short tap back).
- [ ] Rhythm cue timed to the baseline stride interval.

---

## Phase 8 — Phone status page over WiFi (11/29 – 11/30)

- [ ] Nano starts a WiFi access point (`WiFi.beginAP("WALKASSIST", "<password>")`); phone joins it and opens `http://192.168.4.1`.
- [ ] Tiny web server on the Nano (`WiFiServer`) returns one HTML page that auto-refreshes every 2 s:
  - Current state and risk score (color-coded, with words, not color only)
  - This walk: duration, cadence, stride variability, number of near-falls / slips
  - Big **FALL ALERT** banner when `FALL_ALARM` is active; **WARNING** banner for sustained red
- [ ] Server handling must be non-blocking so it never delays sensing.
- [ ] If WiFi fails, the cane still works the same (nothing safety-related depends on it).
- [ ] Fallback if WiFi is a problem: the same summary over USB Serial on a laptop.

---

## Phase 9 — Integration, testing, tuning (11/30 – 12/1)

- [ ] One firmware with logger mode and live mode (serial command or compile flag).
- [ ] Run the full demo script 5+ times in a row without re-flashing.
- [ ] 10 min of normal walking → no warnings (false-alarm check).
- [ ] Carry the cane, drop it, lean it on a wall → never scored as walking, never a fall alarm.
- [ ] Fresh 9 V battery for the demo + spares; spare touch module and motor; USB cable.

---

## Phase 10 — Final demo and video (12/2)

### Demo script
1. Turn on in demo mode; walk 30–60 s → baseline learned; double tap to accept; LED green.
2. Walk unsteadily (knee brace, irregular rhythm) → amber LED, haptic rhythm cue.
3. Keep walking badly → score > 60 sustained → red LED, strong haptic, warning on the phone page.
4. Drop the cane → locator chirps, blinking LED.
5. Lay the cane down while still gripped → fall countdown; double tap cancels. Repeat without cancelling → alarm + FALL ALERT on the phone page.

- [ ] Record the 1-minute video before class (close-ups of LED, haptic, phone page).
- [ ] One handout / poster slide: inputs, outputs, intelligence, novelty.
- [ ] Check the demo hits each rubric item: multimodal input, multimodal output, intelligent, beyond screen, useful, robust, novel.

---

## Risks and fallbacks

| Risk | Fallback |
|---|---|
| Long I2C cable gives bad reads | Lower the clock, twist wires with GND, test early; move the Nano closer to the tip IMU; fall back to IMU 1 only |
| False alarms from carrying or dropping | Tune grip + motion rules on the recordings |
| Baseline learned during an unusual walk | User confirms at the end of calibration; long press re-calibrates |
| Taps triggered by normal gripping | Require a release between taps; only accept long press while standing |
| 9 V battery runs low during the demo | Fresh battery + spares; parked → sleep |
| Touch sensor doesn't work through the grip | Copper tape on the grip, or an FSR force sensor |
| Absolute speed too noisy | Report speed only as a trend vs. baseline |
| Motor vibration shows up in the handle IMU | Ignore IMU samples during motor pulses |
| WiFi page fails at the demo | Cane works offline; show the summary over USB Serial |

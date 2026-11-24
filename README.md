# WALKASSIST — Project Plan

A smart cane that warns users before a fall by comparing their walking to their own baseline.

## Status

| Done | Next |
|---|---|
| Dev setup: arduino-cli + VS Code tasks, git repo | Get parts: vibration motor (order), FSR + resistors |
| Both IMUs read together at 100 Hz (`imu_logger`) | Mount everything on the cane (Phase 2) |
| Grip touch sensor + double/triple tap gestures (temporary, see 1.4) | Record labeled walks (Phase 3) |
| Laptop recorder + plotter (`tools/`) | Bench-test LED, buzzer, motor (1.5) |
| | Replace touch board with an FSR (1.4) |

Critical path: **mount → record → gait analysis**. Outputs and the FSR can happen in parallel.

## Design decisions (locked)

| Topic | Decision |
|---|---|
| Inputs | Grip sensor (capacitive touch for now → **FSR pressure sensor**, see 1.4) + 2 IMUs (onboard LSM6DS3 at the handle, MPU-6050 near the tip over I2C) |
| Outputs | Vibration motor (in-hand cues), RGB LED (status), **active buzzer** (fall alarm, locator) |
| Phone link | **WiFi access point** hosted by the Nano: the phone joins the cane's network and opens a status page. No app, no internet, works on any phone. (The Nano can't run WiFi and BLE at the same time, so we use one.) |
| Power | **9 V battery** → Nano VIN (accepts ~4.5–21 V). Motor, LED, buzzer run from the Nano's 3.3 V pin through transistors. |
| Baseline | 5-min calibration walk (30–60 s in **demo mode**), user confirms it, then slow updates during stable walking; saved to flash at the end of each walk |
| Gestures | **Double tap** = cancel / "I'm OK" / accept baseline · **Triple tap** = re-calibrate. No SOS. (A long press can't work: gripping the cane while walking already looks like one.) |
| Processing | No ML. Per-step features + z-scores on the fly; only the last ~30 **steps** kept in RAM. All sensing, scoring and alerts run on the cane. |

## Repo layout

| Path | What it is |
|---|---|
| `firmware/libraries/WalkAssist/` | Shared drivers: `cane_imu.h` (LSM6DS3, MPU-6050), `cane_touch.h` (grip + gestures) |
| `firmware/i2c_scan/` | Lists I2C devices (wiring check) |
| `firmware/imu_logger/` | Streams both IMUs + grip as CSV at 100 Hz |
| `firmware/touch_test/` | Grip sensor test, gestures, hold test |
| `tools/arduino.sh` | Compile / upload / monitor (used by the VS Code tasks) |
| `tools/log_serial.py`, `tools/plot_session.py` | Record labeled sessions to `data/`, plot them |

---

## Phase 0 — Parts

### Have — used in the project

| Part | Role |
|---|---|
| MPU-6050 (6-DOF) | IMU #2 near the tip (I2C `0x68`). |
| Adafruit AT42QT1010 capacitive touch breakout | Grip sensor for now (drops output after ~20 s of touch; to be replaced by an FSR, see 1.4) |
| SparkFun SEN-12041 capacitive touch breakout (same AT42QT1010 chip) | Spare grip sensor |
| Active buzzer | Audio output |
| 9 V battery with wires | Power → Nano VIN |
| Adafruit 9-DOF (L3GD20H + LSM303) | Backup tip IMU if MPU-6050s misbehave (needs a new driver) |

### Have — not needed (keep out of scope)

Pulse sensor, TMP36 temperature sensor, APDS-9960, sound impact sensor, ADXL335, 3.7 V LiPo (would need a charger + 5 V boost; the 9 V battery replaces it).

### Get from lab

| Part | Purpose |
|---|---|
| **Square FSR** (Alpha MF02A-N-221-A01) + a round FSR (MF01A-N-221-A01) as spare | Grip pressure sensor (replaces the touch board) |
| 2 × **2N4401** NPN transistor (same role as a 2N2222) | Motor + buzzer drivers |
| Diode (1N4148 or 1N400x) | Across the vibration motor |
| RGB LED (check common cathode vs. anode) | Status light |
| Slide or rocker switch, spare 9 V batteries | Power |
| Heat shrink, conductive sheet, perf board, headers | Mounting, grip electrode, final wiring |

### Still need to buy / ask for

| Part | Purpose | Est. cost |
|---|---|---|
| **Coin vibration motor, 3 V (buy 2)** | Haptic output | $2–4 |
| Resistors: 2 × 1 kΩ, 3 × 220 Ω, 1 × 10 kΩ | Transistor bases, LED, FSR divider | $1 |
| 4-conductor wire (~1.5 m) | Tip IMU cable down the shaft, one continuous run | $2 |
| Zip ties, Velcro, small project box | Mounting | $5 |

- [ ] Order the vibration motor.
- [ ] Check the buzzer is **active**: it beeps when connected straight to 3.3 V (a passive one only clicks).
- [ ] Keep receipts for the final budget slide.

---

## Phase 1 — Bench prototype, one part at a time

### 1.1 Environment
- [x] Board package **Arduino SAMD Boards**; `arduino-cli` installed (Homebrew).
- [x] VS Code tasks: Compile / Upload / Monitor (`tools/arduino.sh`, `.vscode/`).
- [x] Git repo for firmware + tools.
- [x] IMU drivers written directly against the registers, so no IMU libraries are needed.
- [ ] Later: `WiFiNINA` (phone page), `FlashStorage` (baseline).

### 1.2 Handle IMU (onboard LSM6DS3)
- [x] Read accel + gyro at 100 Hz (±8 g, ±500 °/s). Reads ~1.01 g at rest.
- [ ] Note which axis points down the shaft when the cane is upright.

### 1.3 Tip IMU (MPU-6050 over I2C)
- [x] Wire: VCC → 3.3 V, GND → GND, SDA → A4, SCL → A5.
- [x] I2C scan shows `0x60` (crypto chip), `0x68` (MPU-6050), `0x6A` (onboard IMU). A `0x7E` entry also appears; it's reserved / harmless.
- [x] Both IMUs read in one loop at a fixed 100 Hz (`micros()` timing, no `delay()`), ±8 g, ±500 °/s, ~44 Hz low-pass.
- [x] Gyro bias measured at startup (cane still for 2 s); tip IMU dropouts detected and auto-reconnected (`t_ok` column).
- [x] Tip reads ~1.05 g at rest: ~5 % high, normal for an MPU-6050; fine for strike / swing detection.
- [ ] Note which axis points down the shaft when the cane is upright.
- [ ] **Test with the full ~1.2 m cable** (chained jumpers OK for testing). Wiggle the wires while recording; tip-missing count should stay ~0. If not: twist SDA/SCL with GND, lower `I2C_CLOCK_HZ` to 50000. For the mounted cane, use one continuous soldered cable.

### 1.4 Grip sensor + gestures
- [x] AT42QT1010 (Adafruit): VIN → 3.3 V, GND, OUT → D2 (HIGH when touched). Working with `touch_test`.
- [x] Gestures in code (`cane_touch.h`): **double tap** and **triple tap** (each tap < 350 ms, gaps < 450 ms). Simulated sequences pass, including regripping and brief releases while walking.
- [x] `touch` and `gesture` columns added to `imu_logger`, recorder and plots.
- [x] **Hold test: the AT42QT1010 drops its output after ~20 s of continuous touch** (measured 20.3 s and 22.7 s with the hand still on the pad).

> **Limitation — AT42QT1010 can't sense a continuous grip.** The chip has a fixed "max on-duration": after a long touch it assumes the pad is blocked, recalibrates, and reports "not touched" until the hand lets go and touches again. The datasheet gives ~60 s in low-power mode; the Adafruit board ships in fast mode, which shortens it to the ~20 s we measured. No pin or jumper disables it.
>
> **Impact:** a user grips the cane for minutes while walking, so after ~20 s the cane thinks it was released. That breaks the grip + motion rules (Phase 5): a real fall would look like "released before impact" → dropped-cane chirp instead of the fall alarm, and walking can look like "carried". Taps and short touches still work.
>
> **Plan:** keep the current touch code for now; replace the grip sensor with an **FSR** (square Alpha MF02A-N-221-A01 preferred, + 10 kΩ resistor to an analog pin). An FSR has no timeout and also measures how hard the user leans on the cane. Only `touched()` changes (pressure above a threshold); the tap-gesture logic stays.

- [ ] Get the FSR and switch the grip sensor; re-run the hold test.
- [ ] Try the gestures on the real grip; tune the timing constants if needed.
- [ ] Update the proposal: "capacitive touch sensor" → "grip pressure sensor (FSR)".

### 1.5 Outputs
- [ ] **Vibration motor:** D3 → 1 kΩ → 2N4401 base; emitter → GND; motor between 3.3 V and collector; diode across the motor (stripe toward 3.3 V). Test PWM strength and patterns.
- [ ] **Active buzzer:** D4 → 1 kΩ → 2N4401 base; buzzer between 3.3 V and collector. On/off only; vary the **beep timing** to tell sounds apart (short chirps = locator, fast continuous = alarm).
- [ ] **RGB LED:** D5 / D6 / D9 through 220 Ω each. Green, amber (red + some green), red, blinking.
- [ ] `output_test` sketch that cycles through all patterns.

### 1.6 Power
- [ ] 9 V clip (through the switch) → VIN and GND. Confirm the Nano boots with USB unplugged.
- [ ] Never connect 9 V to the 3.3 V pin or to the motor/buzzer directly.
- [ ] Measure current while running (multimeter in series) to estimate battery life; bring spare batteries to the demo either way.

---

## Phase 2 — Mount on the cane

- [ ] Handle box: Nano, buzzer, LED (visible from above), 9 V battery + switch (Velcro).
- [ ] Grip sensor on top of the grip where the palm rests; vibration motor under the palm.
- [ ] Tip IMU ~10–15 cm above the rubber tip, mounted **rigidly** (hot glue + zip ties + heat shrink).
- [ ] 4-wire cable down the shaft, zip-tied every ~15 cm, strain relief at both ends.
- [ ] Record the shaft axis of each IMU (the one reading ±1 g with the cane upright).
- [ ] Shake test and drop test onto a soft surface: nothing comes loose, I2C keeps reading.
- [ ] Photos for the final slides and video.

---

## Phase 3 — Data collection

- [x] Firmware **logger mode** (`imu_logger`): `t_ms, h_ax…h_gz, t_ax…t_gz, h_ok, t_ok, touch, gesture` as CSV at 100 Hz.
- [x] Recorder (`tools/log_serial.py --label <name>`): saves to `data/<label>_<time>.csv`; press Enter to add a marker.
- [x] Plotter (`tools/plot_session.py`): both IMUs, grip, markers, gestures.
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
  - [ ] Double and triple taps, while standing and while walking
- [ ] Until the FSR is in, don't trust the `touch` column past ~20 s of continuous grip. Re-record the grip-dependent sessions (drop, fall, carried) after the switch.
- [ ] Decide whether `data/` goes in git (recordings can get large) or in `.gitignore`.

---

## Phase 4 — Gait features

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

## Phase 5 — Grip + motion state machine

States: `WALKING`, `STANDING`, `CARRIED`, `PARKED`, `DROPPED`, `REGRIP_WAIT`, `FALL_CANCEL`, `FALL_ALARM`, `CALIBRATING`.

Needs the FSR grip sensor (1.4): the touch board can't report a continuous grip.

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

## Phase 6 — Baseline and risk score

- [ ] **Calibration:** walk 5 min (demo mode: 30–60 s) → store mean and std of each feature.
- [ ] **User confirms:** when calibration finishes, the cane gives a double haptic pulse; **double tap** to accept, **triple tap** to redo.
- [ ] **Re-calibrate any time:** triple tap (accept it only while standing still, so it isn't triggered while walking).
- [ ] **Adapt:** update mean/std slowly (exponential moving average) only while the score is green.
- [ ] **Save** the baseline to flash after calibration and at the end of each walk (not continuously; flash wears out).
- [ ] **Risk score 0–100:** weighted sum of each feature's z-score (weight stride-time variability most), clipped to 0–100.
  - Green < 30 · Amber 30–60 · Red = fall, or score > 60 sustained (start with 10 s; tune).
- [ ] Hysteresis so the level doesn't flicker on and off.

---

## Phase 7 — Conclusions → responses

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

## Phase 8 — Phone status page over WiFi

- [ ] Nano starts a WiFi access point (`WiFi.beginAP("WALKASSIST", "<password>")`); phone joins it and opens `http://192.168.4.1`.
- [ ] Tiny web server on the Nano (`WiFiServer`) returns one HTML page that auto-refreshes every 2 s:
  - Current state and risk score (color-coded, with words, not color only)
  - This walk: duration, cadence, stride variability, number of near-falls / slips
  - Big **FALL ALERT** banner when `FALL_ALARM` is active; **WARNING** banner for sustained red
- [ ] Server handling must be non-blocking so it never delays sensing.
- [ ] If WiFi fails, the cane still works the same (nothing safety-related depends on it).
- [ ] Fallback if WiFi is a problem: the same summary over USB Serial on a laptop.

---

## Phase 9 — Integration, testing, tuning

- [ ] One firmware with logger mode and live mode (serial command or compile flag).
- [ ] Run the full demo script 5+ times in a row without re-flashing.
- [ ] 10 min of normal walking → no warnings (false-alarm check).
- [ ] Carry the cane, drop it, lean it on a wall → never scored as walking, never a fall alarm.
- [ ] Fresh 9 V battery for the demo + spares; spare grip sensor and motor; USB cable.

---

## Phase 10 — Final demo and video

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
| Long I2C cable gives bad reads | Lower the clock, twist wires with GND, one continuous soldered cable; move the Nano closer to the tip IMU; fall back to IMU 1 only |
| Chained jumper joints work loose on the cane | Use one continuous cable once mounted; heat shrink + hot glue at the pins |
| False alarms from carrying or dropping | Tune grip + motion rules on the recordings |
| Baseline learned during an unusual walk | User confirms at the end of calibration; triple tap re-calibrates |
| Taps triggered by normal gripping | Taps must be short touches after a release; accept triple tap only while standing |
| 9 V battery runs low during the demo | Fresh battery + spares; parked → sleep |
| AT42QT1010 reports "released" after ~20 s of continuous grip (confirmed) | Replace with an FSR before Phase 5; until then, don't trust `touch` in recordings longer than ~20 s |
| FSR misses a light grip | Place it where the palm presses hardest; tune the threshold per user; square FSR covers more area |
| Absolute speed too noisy | Report speed only as a trend vs. baseline |
| Motor vibration shows up in the handle IMU | Ignore IMU samples during motor pulses |
| WiFi page fails at the demo | Cane works offline; show the summary over USB Serial |

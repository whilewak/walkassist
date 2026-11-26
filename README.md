# WALKASSIST — Project Plan

A smart cane that warns users before a fall by comparing their walking to their own baseline.

## Status

| Done | Next |
|---|---|
| Dev setup: arduino-cli + VS Code tasks, git repo | Wire the round FSR and tune its threshold with `touch_test` (1.4) |
| Both IMUs read together at 100 Hz (`imu_logger`) | Zip-tie the box on, mount the tip IMU and its cable (Phase 2) |
| Grip code for the **round FSR on A0** + double/triple tap gestures, grip pressure logged | Record labeled walks (Phase 3) |
| Laptop recorder + plotter (`tools/`) | Solder the LED and buzzer-driver assemblies, test them with `output_test` (1.5) |
| LED + buzzer pattern library (`cane_output.h`) and `output_test` | |
| Clamp box designed (`cad/`), **printed**, test-fitted on the cane with the Nano, breadboard and battery | |
| Breadboard wiring diagram (`docs/circuit.svg`) | |

Critical path: **mount → record → gait analysis**. Outputs can happen in parallel.

## Design decisions (locked)

| Topic | Decision |
|---|---|
| Inputs | **Round FSR** grip pressure sensor on A0 (replaced the capacitive touch board, see 1.4) + 2 IMUs (onboard LSM6DS3 at the handle, MPU-6050 near the tip over I2C) |
| Outputs | **RGB LED** (visual: status and warnings) + **active buzzer** (audio: cues, fall alarm, locator). Two output modalities. The vibration motor was dropped: cues that were going to be in-hand vibrations are now short buzzer patterns + LED, which people nearby can also hear. |
| Phone link | **WiFi access point** hosted by the Nano: the phone joins the cane's network and opens a status page. No app, no internet, works on any phone. (The Nano can't run WiFi and BLE at the same time, so we use one.) |
| Power | **9 V battery** → Nano VIN (accepts ~4.5–21 V). LED and buzzer run from the Nano's 3.3 V pin (buzzer through a transistor). No power switch in the box: unplug the battery clip to turn off. |
| Enclosure | 3D-printed box (`cad/clamp_box.scad`) zip-tied to the shaft just below the handle; sliding lid; the 9 V lies flat in its own bay; Nano on a **mini breadboard** (wiring: `docs/circuit.svg`). |
| Baseline | 5-min calibration walk (30–60 s in **demo mode**), user confirms it, then slow updates during stable walking; saved to flash at the end of each walk |
| Gestures | **Double tap** = cancel / "I'm OK" / accept baseline · **Triple tap** = re-calibrate. No SOS. (A long press can't work: gripping the cane while walking already looks like one.) |
| Processing | No ML. Per-step features + z-scores on the fly; only the last ~30 **steps** kept in RAM. All sensing, scoring and alerts run on the cane. |

## Repo layout

| Path | What it is |
|---|---|
| `firmware/libraries/WalkAssist/` | Shared drivers: `cane_imu.h` (LSM6DS3, MPU-6050), `cane_touch.h` (FSR grip + gestures), `cane_output.h` (LED + buzzer patterns) |
| `firmware/i2c_scan/` | Lists I2C devices (wiring check) |
| `firmware/imu_logger/` | Streams both IMUs + grip (pressed, gesture, pressure) as CSV at 100 Hz |
| `firmware/touch_test/` | Grip (FSR) test: pressure readings, gestures, hold test |
| `firmware/output_test/` | Every LED colour and buzzer pattern, plus a demo of each cane situation |
| `firmware/README.md` | Pin map, wiring, how to run the sketches |
| `tools/arduino.sh` | Compile / upload / monitor (used by the VS Code tasks) |
| `tools/log_serial.py`, `tools/plot_session.py` | Record labeled sessions to `data/`, plot them |
| `cad/clamp_box.scad` | Parametric OpenSCAD model of the box (box, lid, fit and slide test pieces) |
| `docs/circuit.svg` | Breadboard wiring diagram |

---

## Phase 0 — Parts

### Have — used in the project

| Part | Role |
|---|---|
| MPU-6050 (6-DOF) | IMU #2 near the tip (I2C `0x68`). |
| **Round FSR** (Alpha MF01A-N-221-A01) | Grip pressure sensor on A0 |
| Mini breadboard (~17 rows, no power rails) | Holds the Nano; every wire plugs in next to its pin (see `docs/circuit.svg`) |
| Active buzzer | Audio output |
| 9 V battery with snap clip | Power → Nano VIN |
| Printed clamp box + lid | Enclosure on the cane |
| Adafruit 9-DOF (L3GD20H + LSM303) | Backup tip IMU if MPU-6050s misbehave (needs a new driver) |
| Adafruit / SparkFun AT42QT1010 touch boards | No longer used (time out after ~20 s, see 1.4) |

### Have — not needed (keep out of scope)

Pulse sensor, TMP36 temperature sensor, APDS-9960, sound impact sensor, ADXL335, 3.7 V LiPo (would need a charger + 5 V boost; the 9 V battery replaces it).

### Still to collect (lab inventory / ask)

| Part | Purpose |
|---|---|
| Resistors: **1 × 10 kΩ**, **1 × 1 kΩ**, **3 × 220 Ω** | FSR divider (on the breadboard), buzzer transistor base, LED colours |
| 1 × **2N4401** NPN transistor | Buzzer driver |
| RGB LED (check common cathode vs. anode) | Status light |
| Female-female jumper wires | FSR legs (don't solder them), off-board assemblies |
| 4-conductor wire (~1.5 m) | Tip IMU cable down the shaft, one continuous run |
| Zip ties (4.8 × 1.4 mm fit the box slots), rubber strip, heat shrink | Mounting |
| Spare 9 V batteries | Demo |

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
- [x] Gestures in code (`cane_touch.h`): **double tap** and **triple tap** (each tap < 350 ms, gaps < 450 ms).
- [x] **Switched to a round FSR on A0** (`cane_touch.h`): pressed above 150, released below 100 (0–1023), A0 sampled every 2 ms and lightly smoothed. `pressure()` also gives how hard the user leans on the cane.
- [x] `touch`, `gesture` and `grip` (pressure) columns in `imu_logger`; grip pressure plotted by `plot_session.py`.
- [x] Simulated tests pass: double / triple taps, regripping, brief releases while walking, a 2-minute hold staying pressed, a light resting hand not counting, noise near the threshold.
- [ ] Wire it (FSR leg 1 → 3V3, leg 2 → free row 16 → jumper → A0, 10 kΩ from row 16 to GND) and run `touch_test`: note the pressure with no hand, a resting hand, a normal grip and leaning hard; adjust the thresholds so 150 sits between resting and gripping.
- [ ] Try the gestures on the real grip; tune the timing constants if needed.
- [ ] Update the proposal: "capacitive touch sensor" → "grip pressure sensor (FSR)".

> **Why not the capacitive touch board:** the AT42QT1010 drops its output after ~20 s of continuous touch (measured 20.3 s and 22.7 s; it recalibrates and there's no way to turn that off). A user grips the cane for minutes while walking, so it would report "released" mid-walk and break the fall-vs-dropped rules. The FSR has no timeout.

### 1.5 Outputs
The breadboard has no spare rows for these, so each is a small soldered assembly with wires that plug in next to its pin (dashed boxes in `docs/circuit.svg`).
- [ ] **Active buzzer driver:** 2N4401 (flat side facing you: E-B-C). 1 kΩ on the base → wire to **D4**; buzzer − on the collector, buzzer + → **3V3**; emitter → **GND** (right). On/off only; vary the **beep timing** to tell sounds apart (soft ticks = rhythm cue, short chirps = locator, fast continuous = alarm).
- [ ] **RGB LED:** 220 Ω soldered on each colour leg → **D5** (red), **D6** (green), **D9** (blue); common (longest) leg → **GND** (right). Pushed into the top-left hole of the box.
- [x] Pattern library `cane_output.h`: every sound and light from Phase 7, non-blocking, background + one-shot layers. Timing checked in simulation (13 checks).
- [x] `output_test` sketch: each pattern by key, or `d` for a demo of every situation.
- [ ] Run `output_test` on the real LED and buzzer; tune timings by ear, check the amber mix looks amber.

### 1.6 Power
- [ ] 9 V clip: red → **VIN**, black → **GND** (left). Tin the stranded ends so they push firmly into the breadboard. Confirm the Nano boots with USB unplugged.
- [ ] Never connect 9 V to the 3.3 V pin or to the buzzer directly.
- [ ] No switch in the box: unplug the clip to turn off (or add an inline switch on the red wire, out through a wire hole).
- [ ] Measure current while running (multimeter in series) to estimate battery life; bring spare batteries to the demo either way.

---

## Phase 2 — Mount on the cane

Box: `cad/clamp_box.scad`, about **60 × 86 × 37.5 mm** (inside 52 × 81 × 20 mm: 27 mm battery bay, divider, 52 mm electronics bay). Openings: top end = LED (left), USB (middle), grip wires (right); bottom end = tip IMU cable; lid = buzzer grille.

- [x] Fit test, slide test and full box printed; Nano on the mini breadboard and the 9 V fit, USB lines up with its opening.
- [ ] Mount the box: rubber strip on the shaft, then each of the 3 zip ties up through one slot, across the floor groove, down the other slot and around the shaft (before the breadboard and battery go in).
- [ ] Stick the breadboard to the floor (its own adhesive back), battery flat in its bay, lid slid down until it clicks.
- [ ] Round FSR on top of the grip where the palm presses hardest, under one thin layer of tape; wires in through the top-right hole.
- [ ] Tip IMU ~10–15 cm above the rubber tip, mounted **rigidly** (hot glue + zip ties + heat shrink).
- [ ] 4-wire cable down the shaft, zip-tied every ~15 cm, strain relief at both ends, in through the bottom hole.
- [ ] Record the shaft axis of each IMU (the one reading ±1 g with the cane upright).
- [ ] Shake test and drop test onto a soft surface: nothing comes loose (breadboard wires included), I2C keeps reading.
- [ ] Photos for the final slides and video.

---

## Phase 3 — Data collection

- [x] Firmware **logger mode** (`imu_logger`): `t_ms, h_ax…h_gz, t_ax…t_gz, h_ok, t_ok, touch, gesture, grip` as CSV at 100 Hz.
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
- [ ] Record with the FSR wired, so the grip columns are valid for the grip-dependent sessions (drop, fall, carried).
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

Uses the FSR grip sensor (1.4), which reports a continuous grip. Grip pressure can also tell "leaning on the cane" from "lightly holding it".

| Grip | Motion | Means | Cane does |
|---|---|---|---|
| Held | Rhythmic tip strikes | Walking | Analyze gait |
| Held | Still, upright | Standing | Watch for sway |
| Released | Moving, no strikes | Carried, in a car | Ignore (never scored as walking) |
| Released | Upright, still | Parked | Sleep (low power) |
| Released before impact | Lies flat, no re-grip | Dropped cane | Locator chirp, blinking LED |
| Held through impact | Ends flat and still | Person fell | Fall alarm with 15 s cancel |
| Lost at impact | Ends flat and still | Unclear | Wait **5 s** for re-grip. Re-gripped → resume, log a stumble. Not re-gripped → fall alarm with 15 s cancel |

- [ ] Fall flow: `FALL_CANCEL` (15 s: red LED, buzzer countdown beeps getting faster) → double tap cancels → otherwise `FALL_ALARM` (continuous buzzer, red flashing, alert on the phone page).
- [ ] **Parked → sleep:** lower the IMU rate, turn off the LED, wake on grip.
- [ ] Test every row against the recordings; tune thresholds.

---

## Phase 6 — Baseline and risk score

- [ ] **Calibration:** walk 5 min (demo mode: 30–60 s) → store mean and std of each feature.
- [ ] **User confirms:** when calibration finishes, the cane gives a double beep and blinks green; **double tap** to accept, **triple tap** to redo.
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
| Stable | All features in normal range | Green LED, silent |
| Unsteady gait | Stride-time variability or lateral sway above baseline | Amber LED, short buzzer ticks at the user's own cadence (rhythm cue) |
| Fatigue | Cadence + swing angle trending down over the session, more shuffling | Amber LED, "rest" pattern (slow double beep every few seconds) |
| Slippery / uneven surface | Tip-slip jolt | One short beep, amber flash |
| Near-fall | Stumble (jolt, then fast re-plant) | Three quick beeps, amber flashing, logged for the caregiver |
| High risk, no fall | Score > 60 sustained | **Red LED, repeated "stop and rest" beep pattern**, logged + warning on the phone page. Never the continuous fall alarm. |
| Dropped cane | See Phase 5 | Locator chirps, blinking LED |
| Fall | Held through impact, flat and still (or no re-grip within 5 s) | Red LED, buzzer, 15 s cancel, then phone alert |

- [x] Non-blocking pattern player for LED / buzzer (`cane_output.h`; no `delay()`, sensing never stops).
- [ ] Every gesture is acknowledged (one short chirp + LED blink).
- [ ] Rhythm cue ticks timed to the baseline stride interval; keep them short so they guide without becoming noise.
- [ ] Every pattern must be distinct by ear: rhythm ticks, rest double-beep, near-fall triple beep, locator chirps, countdown, alarm.

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
- [ ] Fresh 9 V battery for the demo + spares; spare grip sensor and buzzer; USB cable.

---

## Phase 10 — Final demo and video

### Demo script
1. Turn on in demo mode; walk 30–60 s → baseline learned; double tap to accept; LED green.
2. Walk unsteadily (knee brace, irregular rhythm) → amber LED, buzzer rhythm ticks.
3. Keep walking badly → score > 60 sustained → red LED, "stop and rest" beeps, warning on the phone page.
4. Drop the cane → locator chirps, blinking LED.
5. Lay the cane down while still gripped → fall countdown; double tap cancels. Repeat without cancelling → alarm + FALL ALERT on the phone page.

- [ ] Record the 1-minute video before class (close-ups of the LED and phone page, with sound on for the buzzer).
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
| Round FSR misses a light grip (small ~13 mm sensing area) | Place it where the palm presses hardest; tune the thresholds per user; the square FSR covers more area if needed |
| Breadboard wires work loose as the cane taps the ground | Cut wires to length, strain-relieve them, dab of hot glue at the ends once working; soldered assemblies for the LED and buzzer |
| Lid too tight / loose, or doesn't click | `slide_clear` and `detent_h` in `cad/clamp_box.scad`; reprint only the lid |
| Absolute speed too noisy | Report speed only as a trend vs. baseline |
| Beep cues annoy the user or people nearby | Ticks only while the gait is unsteady; short beeps; the loud continuous sound is reserved for a fall |
| Buzzer cues missed in a noisy place | LED shows the same state; the phone page shows warnings |
| WiFi page fails at the demo | Cane works offline; show the summary over USB Serial |

# WALKASSIST firmware

Board: **Arduino Nano 33 IoT** (Arduino IDE → Boards Manager → "Arduino SAMD Boards").
No libraries need installing. Shared drivers live in `libraries/WalkAssist/`
(`cane_imu.h`, `cane_touch.h`); `tools/arduino.sh` and the VS Code tasks pass
that folder to the compiler. In the Arduino IDE, copy or symlink
`libraries/WalkAssist` into `~/Documents/Arduino/libraries/` instead.

| Sketch | What it does |
|---|---|
| `i2c_scan/` | Lists every I2C device. Run this first to check the wiring. |
| `touch_test/` | Grip sensor (FSR) test: pressure readings, gestures, and the 2-minute hold test. |
| `imu_logger/` | Reads both IMUs and the grip sensor at 100 Hz and streams CSV over USB. |

## Pin map

Full schematic: [`docs/circuit.svg`](../docs/circuit.svg). Everything runs at 3.3 V.

| Nano pin | Connects to | Notes |
|---|---|---|
| 3V3, GND | All modules' power | Never 9 V here |
| A4 (SDA), A5 (SCL) | MPU-6050 tip IMU | Onboard LSM6DS3 shares this bus (0x6A) |
| A0 | Round FSR grip sensor | 3V3 → FSR → row 16 → jumper → A0; 10 kΩ from row 16 to GND |
| D4 | 1 kΩ → 2N4401 base → active buzzer | Buzzer + leg to 3V3, − leg to collector; emitter to GND |
| D5 / D6 / D9 (PWM) | 220 Ω → RGB LED red / green / blue | Common cathode to GND |
| VIN | 9 V battery + | Battery − to GND |

Free: D2, D3, D7, D8, D10–D12, A1–A3, A6, A7. Avoid D0/D1 (Serial1) and D13 (onboard LED).
(No vibration motor: outputs are the RGB LED and the buzzer.)

## Wiring: grip sensor (round FSR on A0)

The FSR's resistance drops when it's pressed. With a 10 kΩ resistor to GND,
A0 reads ~0 with no hand and rises toward 1023 as the grip is squeezed.

| Connection | Breadboard |
|---|---|
| FSR leg 1 | **3V3** row |
| FSR leg 2 | free **row 16** (left half) |
| 10 kΩ resistor | from the left **GND** row to row 16 |
| Jumper | from row 16 to the **A0** row |

- Don't solder the FSR's legs (heat melts its plastic): push female jumper wires on.
- Place it on top of the handle where the palm presses hardest, under one thin
  layer of tape. The FSR's legs can go either way round.
- The Nano's orange LED lights while the grip counts as pressed (both sketches).
- "Pressed" = pressure above 150, "released" = below 100 (`PRESS_THRESHOLD`,
  `RELEASE_THRESHOLD` in `cane_touch.h`). The gap stops flicker.
- Unlike the old AT42QT1010 touch board, the FSR has no timeout, so a long
  grip stays "pressed". The touch board is no longer used; D2 is free.

### Gestures

The hand leaves the grip, then taps (each tap under 0.35 s, gaps under 0.45 s):

| Gesture | Meaning |
|---|---|
| Double tap | Cancel / "I'm OK" / accept baseline |
| Triple tap | Re-calibrate |

A long press isn't used: holding the cane while walking already looks like one.
Timing constants are at the top of `cane_touch.h`.

### Grip test

Upload `touch_test` and open the Serial Monitor:

1. With no hand on the grip, the status line shows the pressure (should be near 0).
2. Rest your hand lightly, grip normally, then lean hard; note each pressure.
   150 should sit between "resting" and "gripping"; if not, change the
   thresholds in `cane_touch.h`.
3. Try double and triple taps.
4. Hold for 2+ minutes: `still held` prints every 10 s with the pressure, and
   `UP` should appear only when you let go.

## Wiring: tip IMU (MPU-6050 / GY-521 board)

| GY-521 pin | Nano 33 IoT pin |
|---|---|
| VCC | **3.3V** (not 5V: the Nano's 5V pin is off unless a jumper is soldered) |
| GND | GND |
| SCL | **A5** |
| SDA | **A4** |
| AD0, INT, XDA, XCL | leave unconnected (AD0 floating = address `0x68`) |

The handle IMU (LSM6DS3) is built into the Nano and already on the same A4/A5 bus at `0x6A`.

Long cable down the shaft: start on the breadboard with short jumpers. Once
that works, swap in the full ~1.2 m wire and run `i2c_scan` again. The sketches
already use the slow, more reliable 100 kHz I2C clock.

## Steps

1. Upload `i2c_scan`, open Serial Monitor at 115200. Expect `0x60`, `0x68`, `0x6A`.
2. Upload `imu_logger`. Keep the cane still for the first 2 s (gyro bias).
3. Type `p` + Enter and open **Tools → Serial Plotter** to watch acceleration live.
   With the cane upright and still, the axis pointing along the shaft reads about ±1 g.
   Write down which axis that is for each IMU.
4. Close the Serial Monitor/Plotter (only one program can use the port), then record:

```bash
pip3 install -r tools/requirements.txt
python3 tools/log_serial.py --label normal_walk
python3 tools/plot_session.py data/normal_walk_<timestamp>.csv
```

## Serial commands (imu_logger)

| Key | Action |
|---|---|
| `c` | Re-measure gyro bias (hold still 2 s) |
| `p` | Plotter mode (labeled accelerations) |
| `v` | CSV mode (default) |
| `h` | Print the CSV header |
| `s` | Status: which IMUs answered, grip state, read errors |

CSV columns: `t_ms`, handle `h_ax…h_gz`, tip `t_ax…t_gz`, `h_ok`, `t_ok`,
`touch` (1 = gripped), `gesture` (2 = double tap, 3 = triple tap, on the
sample where it finished), `grip` (FSR pressure 0–1023). `log_serial.py` adds
`mark` for Enter presses.

## If the tip IMU isn't found

- Swap SDA/SCL: A4 is SDA, A5 is SCL.
- Check the GY-521 power LED is on.
- `WHO_AM_I` other than `0x68` (e.g. `0x70`, `0x98`) means a clone chip. The
  sketch still uses it, but if the readings look wrong, try another MPU-6050.
- Works on short wires but not the long cable: shorten it, or twist the
  SDA and SCL wires each together with a ground wire.
- When the tip IMU drops out mid-stream, the logger prints `# tip IMU lost`
  and keeps sending handle data with `t_ok = 0`, retrying every second.

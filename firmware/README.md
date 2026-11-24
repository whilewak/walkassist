# WALKASSIST firmware

Board: **Arduino Nano 33 IoT** (Arduino IDE → Boards Manager → "Arduino SAMD Boards").
No libraries need installing. Shared drivers live in `libraries/WalkAssist/`
(`cane_imu.h`, `cane_touch.h`); `tools/arduino.sh` and the VS Code tasks pass
that folder to the compiler. In the Arduino IDE, copy or symlink
`libraries/WalkAssist` into `~/Documents/Arduino/libraries/` instead.

| Sketch | What it does |
|---|---|
| `i2c_scan/` | Lists every I2C device. Run this first to check the wiring. |
| `touch_test/` | Grip sensor test: touches, gestures, and the 2-minute hold test. |
| `imu_logger/` | Reads both IMUs and the grip sensor at 100 Hz and streams CSV over USB. |

## Pin map

Full schematic: [`docs/circuit.svg`](../docs/circuit.svg). Everything runs at 3.3 V.

| Nano pin | Connects to | Notes |
|---|---|---|
| 3V3, GND | All modules' power | Never 9 V here |
| A4 (SDA), A5 (SCL) | MPU-6050 tip IMU | Onboard LSM6DS3 shares this bus (0x6A) |
| A0 | FSR grip sensor (planned) | 3V3 → FSR → A0 → 10 kΩ → GND |
| D2 | AT42QT1010 OUT (temporary) | Freed once the FSR is in |
| D3 (PWM) | 1 kΩ → 2N4401 base → vibration motor | Motor from 3V3 to collector, diode across it (stripe toward 3V3) |
| D4 | 1 kΩ → 2N4401 base → active buzzer | Buzzer + leg to 3V3, − leg to collector |
| D5 / D6 / D9 (PWM) | 220 Ω → RGB LED red / green / blue | Common cathode to GND |
| VIN | 9 V battery + (through the switch) | Battery − to GND |

Free: D7, D8, D10–D12, A1–A3, A6, A7. Avoid D0/D1 (Serial1) and D13 (onboard LED).

## Wiring: grip touch sensor (Adafruit AT42QT1010 breakout)

| Touch board pin | Nano 33 IoT pin |
|---|---|
| VIN | 3.3V |
| GND | GND |
| OUT | **D2** |

- Don't touch the pad while powering up; the chip calibrates at power-on.
- The Nano's orange LED lights while the sensor reads "touched" (both sketches).
- To sense through the cane grip, wire the board's pad to copper tape or a
  conductive sheet wrapped around the grip.
- The SparkFun SEN-12041 uses the same AT42QT1010 chip and wires the same way: VDD → 3.3V, GND, OUT → D2.
- **The AT42QT1010 drops its output after ~60 s of continuous touch** (it recalibrates; datasheet §3.3). Run the hold test below.

### Gestures

The hand leaves the grip, then taps (each tap under 0.35 s, gaps under 0.45 s):

| Gesture | Meaning |
|---|---|
| Double tap | Cancel / "I'm OK" / accept baseline |
| Triple tap | Re-calibrate |

A long press isn't used: holding the cane while walking already looks like one.
Timing constants are at the top of `cane_touch.h`.

### Hold test

Upload `touch_test`, grip the sensor and keep holding for 2+ minutes. A
`still held` line prints every 10 s. If `UP` appears while your hand is still
on it, the chip recalibrated during the long touch; note after how long.

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
sample where it finished). `log_serial.py` adds `mark` for Enter presses.

## If the tip IMU isn't found

- Swap SDA/SCL: A4 is SDA, A5 is SCL.
- Check the GY-521 power LED is on.
- `WHO_AM_I` other than `0x68` (e.g. `0x70`, `0x98`) means a clone chip. The
  sketch still uses it, but if the readings look wrong, try another MPU-6050.
- Works on short wires but not the long cable: shorten it, or twist the
  SDA and SCL wires each together with a ground wire.
- When the tip IMU drops out mid-stream, the logger prints `# tip IMU lost`
  and keeps sending handle data with `t_ok = 0`, retrying every second.

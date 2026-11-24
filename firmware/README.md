# WALKASSIST firmware

Board: **Arduino Nano 33 IoT** (Arduino IDE → Boards Manager → "Arduino SAMD Boards").
No extra libraries are needed for the IMU sketches.

| Sketch | What it does |
|---|---|
| `i2c_scan/` | Lists every I2C device. Run this first to check the wiring. |
| `imu_logger/` | Reads both IMUs at 100 Hz and streams CSV over USB. |

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
| `s` | Status: which IMUs answered, read errors |

## If the tip IMU isn't found

- Swap SDA/SCL: A4 is SDA, A5 is SCL.
- Check the GY-521 power LED is on.
- `WHO_AM_I` other than `0x68` (e.g. `0x70`, `0x98`) means a clone chip. The
  sketch still uses it, but if the readings look wrong, try another MPU-6050.
- Works on short wires but not the long cable: shorten it, or twist the
  SDA and SCL wires each together with a ground wire.
- When the tip IMU drops out mid-stream, the logger prints `# tip IMU lost`
  and keeps sending handle data with `t_ok = 0`, retrying every second.

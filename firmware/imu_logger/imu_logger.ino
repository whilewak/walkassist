// WALKASSIST - read both IMUs and the grip sensor at 100 Hz and stream them over USB.
//
// Handle IMU = LSM6DS3 on the Nano 33 IoT. Tip IMU = MPU-6050 over I2C.
// Grip = AT42QT1010 touch breakout, OUT on D2.
// Open Serial Monitor / Plotter at 115200 baud, or record with tools/log_serial.py.
//
// Output (lines starting with '#' are status messages, not data):
//   CSV mode (default):
//     t_ms,h_ax,h_ay,h_az,h_gx,h_gy,h_gz,t_ax,t_ay,t_az,t_gx,t_gy,t_gz,h_ok,t_ok,touch,gesture
//     h_ = handle IMU, t_ = tip IMU, acceleration in g, rotation in deg/s,
//     *_ok = 1 if that IMU was read this sample, 0 if not,
//     touch = 1 while the grip is touched, gesture = 2 (double tap) or
//     3 (triple tap) on the sample where a gesture finished, else 0.
//   Plotter mode: accelerations only, labeled for the Arduino IDE Serial Plotter.
//
// Serial commands (type a letter, press Enter):
//   c  re-measure gyro bias (hold the cane still for 2 s)
//   h  print the CSV header again
//   p  plotter mode      v  CSV mode      s  status

#include <Wire.h>
#include <cane_imu.h>
#include <cane_touch.h>

const uint8_t TOUCH_PIN = 2;
const uint32_t SAMPLE_PERIOD_US = 10000;  // 100 Hz
const uint32_t I2C_CLOCK_HZ = 100000;     // 100 kHz: reliable over a long cable
const uint8_t TIP_FAILS_BEFORE_LOST = 10; // consecutive failed reads
const uint32_t TIP_RETRY_MS = 1000;       // how often to try to reconnect

enum OutputMode { MODE_CSV, MODE_PLOTTER };
OutputMode mode = MODE_CSV;

Lsm6ds3 handleImu;
Mpu6050 tipImu;
TouchSensor touch;
bool handleOk = false;
bool tipOk = false;
Gesture pendingGesture = GESTURE_NONE;  // reported on the next sample

float handleBias[3] = {0, 0, 0};  // gyro bias, deg/s
float tipBias[3] = {0, 0, 0};

uint32_t nextSampleUs = 0;
uint32_t lastTipRetryMs = 0;
uint8_t tipFailsInRow = 0;
uint32_t tipReadErrors = 0;
uint32_t lateSamples = 0;

// ------------------------------------------------------------------ setup --
void setup() {
  Serial.begin(115200);
  uint32_t start = millis();
  while (!Serial && millis() - start < 3000) {}  // don't hang when on battery

  Wire.begin();
  Wire.setClock(I2C_CLOCK_HZ);
  touch.begin(TOUCH_PIN);
  pinMode(LED_BUILTIN, OUTPUT);  // mirrors the grip sensor

  handleOk = handleImu.begin();
  tipOk = tipImu.begin();
  printStatus();

  calibrateGyroBias();
  printHeader();
  nextSampleUs = micros();
}

// ------------------------------------------------------------------- loop --
void loop() {
  handleSerialCommands();

  // The touch sensor is checked every pass (not only every 10 ms) so tap
  // timing is accurate. A finished gesture waits for the next sample.
  Gesture g = touch.update();
  if (g != GESTURE_NONE) {
    pendingGesture = g;
    Serial.print("# gesture: ");
    Serial.println(gestureName(g));
  }
  digitalWrite(LED_BUILTIN, touch.touched() ? HIGH : LOW);

  // Wait for the next 10 ms tick without blocking.
  if ((int32_t)(micros() - nextSampleUs) < 0) return;
  nextSampleUs += SAMPLE_PERIOD_US;
  if ((int32_t)(micros() - nextSampleUs) > (int32_t)SAMPLE_PERIOD_US) {
    nextSampleUs = micros() + SAMPLE_PERIOD_US;  // fell far behind: resync
    lateSamples++;
  }
  uint32_t tMs = millis();

  ImuSample h = {0, 0, 0, 0, 0, 0};
  ImuSample t = {0, 0, 0, 0, 0, 0};
  bool hRead = handleOk && handleImu.read(h);
  bool tRead = tipOk && tipImu.read(t);

  trackTipHealth(tRead);

  if (hRead) removeBias(h, handleBias);
  if (tRead) removeBias(t, tipBias);

  if (mode == MODE_CSV) {
    printCsv(tMs, h, t, hRead, tRead, touch.touched(), pendingGesture);
    pendingGesture = GESTURE_NONE;
  } else {
    printPlotter(h, t);
  }
}

// --------------------------------------------------------- tip IMU health --
// The tip IMU sits at the end of a long cable, so expect occasional failed
// reads. After several in a row, mark it lost and keep trying to reconnect;
// the handle IMU keeps streaming either way.
void trackTipHealth(bool tRead) {
  if (tipOk) {
    if (tRead) {
      tipFailsInRow = 0;
    } else {
      tipReadErrors++;
      if (++tipFailsInRow >= TIP_FAILS_BEFORE_LOST) {
        tipOk = false;
        Serial.println("# tip IMU lost, retrying every second");
      }
    }
  } else if (millis() - lastTipRetryMs > TIP_RETRY_MS) {
    lastTipRetryMs = millis();
    if (tipImu.begin()) {
      tipOk = true;
      tipFailsInRow = 0;
      Serial.println("# tip IMU reconnected");
    }
  }
}

// -------------------------------------------------------------- gyro bias --
// A still gyro still reads a small offset. Average 2 s of still readings and
// subtract it, so integrated angles (swing) don't drift.
void calibrateGyroBias() {
  const int N = 200;
  float hSum[3] = {0, 0, 0}, tSum[3] = {0, 0, 0};
  float hMagMin = 99, hMagMax = 0;
  int hCount = 0, tCount = 0;

  Serial.println("# measuring gyro bias: keep the cane still for 2 s...");
  for (int i = 0; i < N; i++) {
    ImuSample s;
    if (handleOk && handleImu.read(s)) {
      hSum[0] += s.gx; hSum[1] += s.gy; hSum[2] += s.gz;
      float mag = sqrtf(s.ax * s.ax + s.ay * s.ay + s.az * s.az);
      hMagMin = min(hMagMin, mag);
      hMagMax = max(hMagMax, mag);
      hCount++;
    }
    if (tipOk && tipImu.read(s)) {
      tSum[0] += s.gx; tSum[1] += s.gy; tSum[2] += s.gz;
      tCount++;
    }
    delay(10);
  }

  if (hCount > 0 && hMagMax - hMagMin > 0.1f) {
    Serial.println("# cane moved during calibration, bias not changed (send 'c' to retry)");
    return;
  }
  for (int k = 0; k < 3; k++) {
    if (hCount > 0) handleBias[k] = hSum[k] / hCount;
    if (tCount > 0) tipBias[k] = tSum[k] / tCount;
  }
  Serial.print("# gyro bias handle (deg/s): ");
  printTriple(handleBias);
  Serial.print("# gyro bias tip    (deg/s): ");
  printTriple(tipBias);
}

void removeBias(ImuSample& s, const float bias[3]) {
  s.gx -= bias[0];
  s.gy -= bias[1];
  s.gz -= bias[2];
}

// ----------------------------------------------------------------- output --
void printHeader() {
  Serial.println("t_ms,h_ax,h_ay,h_az,h_gx,h_gy,h_gz,t_ax,t_ay,t_az,t_gx,t_gy,t_gz,"
                 "h_ok,t_ok,touch,gesture");
}

void printSample(const ImuSample& s) {
  Serial.print(s.ax, 3); Serial.print(',');
  Serial.print(s.ay, 3); Serial.print(',');
  Serial.print(s.az, 3); Serial.print(',');
  Serial.print(s.gx, 2); Serial.print(',');
  Serial.print(s.gy, 2); Serial.print(',');
  Serial.print(s.gz, 2); Serial.print(',');
}

void printCsv(uint32_t tMs, const ImuSample& h, const ImuSample& t, bool hRead, bool tRead,
              bool touched, Gesture g) {
  Serial.print(tMs); Serial.print(',');
  printSample(h);
  printSample(t);
  Serial.print(hRead ? 1 : 0); Serial.print(',');
  Serial.print(tRead ? 1 : 0); Serial.print(',');
  Serial.print(touched ? 1 : 0); Serial.print(',');
  Serial.println((int)g);
}

void printPlotter(const ImuSample& h, const ImuSample& t) {
  Serial.print("h_ax:"); Serial.print(h.ax, 3);
  Serial.print(",h_ay:"); Serial.print(h.ay, 3);
  Serial.print(",h_az:"); Serial.print(h.az, 3);
  Serial.print(",t_ax:"); Serial.print(t.ax, 3);
  Serial.print(",t_ay:"); Serial.print(t.ay, 3);
  Serial.print(",t_az:"); Serial.print(t.az, 3);
  Serial.print(",touch:"); Serial.println(touch.touched() ? 1 : 0);
}

void printTriple(const float v[3]) {
  Serial.print(v[0], 2); Serial.print(", ");
  Serial.print(v[1], 2); Serial.print(", ");
  Serial.println(v[2], 2);
}

void printStatus() {
  Serial.print("# handle IMU (LSM6DS3 0x6A): ");
  Serial.print(handleOk ? "OK" : "NOT FOUND");
  Serial.print(", WHO_AM_I=0x");
  Serial.println(handleImu.whoAmI, HEX);

  Serial.print("# tip IMU (MPU-6050 0x68): ");
  Serial.print(tipOk ? "OK" : "NOT FOUND (check wiring)");
  Serial.print(", WHO_AM_I=0x");
  Serial.println(tipImu.whoAmI, HEX);

  Serial.print("# grip sensor (D2): ");
  Serial.println(touch.touched() ? "touched" : "not touched");

  Serial.print("# tip read errors: ");
  Serial.print(tipReadErrors);
  Serial.print(", late samples: ");
  Serial.println(lateSamples);
}

// --------------------------------------------------------------- commands --
void handleSerialCommands() {
  while (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case 'c': calibrateGyroBias(); nextSampleUs = micros(); break;
      case 'h': printHeader(); break;
      case 'p': mode = MODE_PLOTTER; break;
      case 'v': mode = MODE_CSV; printHeader(); break;
      case 's': printStatus(); break;
      default: break;  // ignore newlines and anything else
    }
  }
}

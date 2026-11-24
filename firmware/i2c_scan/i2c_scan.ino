// WALKASSIST - I2C scanner
//
// Lists every device on the I2C bus (pins A4 = SDA, A5 = SCL) every 3 s.
// On a Nano 33 IoT with the MPU-6050 wired up, expect:
//   0x60  ATECC608A crypto chip (onboard)
//   0x68  MPU-6050 (tip IMU)        <- missing? check the 4 wires
//   0x6A  LSM6DS3  (onboard IMU)
//
// Open Serial Monitor at 115200 baud.

#include <Wire.h>

const uint32_t I2C_CLOCK_HZ = 100000;  // 100 kHz: safest for a long cable

void setup() {
  Serial.begin(115200);
  uint32_t start = millis();
  while (!Serial && millis() - start < 3000) {}  // wait for USB, but not forever

  Wire.begin();
  Wire.setClock(I2C_CLOCK_HZ);
  Serial.println("I2C scanner ready");
}

const char* knownName(uint8_t addr) {
  switch (addr) {
    case 0x19: return "LSM303 accelerometer (Adafruit 9-DOF board)";
    case 0x1E: return "LSM303 magnetometer (Adafruit 9-DOF board)";
    case 0x60: return "ATECC608A crypto chip (onboard)";
    case 0x68: return "MPU-6050 (tip IMU, AD0 low)";
    case 0x69: return "MPU-6050 (AD0 high)";
    case 0x6A: return "LSM6DS3 (onboard IMU)";
    case 0x6B: return "L3GD20H gyro (Adafruit 9-DOF) or LSM6DS33 (MinIMU-9)";
    default:   return (addr >= 0x78) ? "reserved address, probably a glitch" : "";
  }
}

void loop() {
  int found = 0;
  Serial.println("Scanning...");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("  0x");
      if (addr < 16) Serial.print('0');
      Serial.print(addr, HEX);
      Serial.print("  ");
      Serial.println(knownName(addr));
      found++;
    }
  }
  Serial.print(found);
  Serial.println(" device(s) found\n");
  delay(3000);
}

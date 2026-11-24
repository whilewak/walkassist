// WALKASSIST - minimal drivers for the two IMUs, talking to their registers
// directly over I2C so we control range and rate and need no extra libraries.
//
//   Handle IMU: LSM6DS3 on the Nano 33 IoT, address 0x6A
//   Tip IMU:    MPU-6050 (GY-521 board), address 0x68 (AD0 pin low / unconnected)
//
// Both are set to +-8 g and +-500 deg/s, about 100 samples per second.
// Units out: acceleration in g, rotation rate in deg/s.

#pragma once
#include <Arduino.h>
#include <Wire.h>

struct ImuSample {
  float ax, ay, az;  // g
  float gx, gy, gz;  // deg/s
};

namespace i2c {

inline bool writeReg(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

inline bool readRegs(uint8_t addr, uint8_t reg, uint8_t* buf, uint8_t n) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;  // device did not answer
  if (Wire.requestFrom(addr, (size_t)n) != n) return false;
  for (uint8_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}

inline int16_t le16(const uint8_t* b) { return (int16_t)((b[1] << 8) | b[0]); }
inline int16_t be16(const uint8_t* b) { return (int16_t)((b[0] << 8) | b[1]); }

}  // namespace i2c

// ---------------------------------------------------------------- LSM6DS3 --
class Lsm6ds3 {
 public:
  static const uint8_t ADDR = 0x6A;

  bool begin() {
    uint8_t who = 0;
    if (!i2c::readRegs(ADDR, 0x0F, &who, 1)) return false;   // WHO_AM_I
    whoAmI = who;
    if (who != 0x69 && who != 0x6A && who != 0x6C) return false;  // DS3 / DS3TR-C / DSL
    return i2c::writeReg(ADDR, 0x12, 0x44)     // CTRL3_C: block data update, auto-increment
        && i2c::writeReg(ADDR, 0x10, 0x4C)     // CTRL1_XL: 104 Hz, +-8 g
        && i2c::writeReg(ADDR, 0x11, 0x44);    // CTRL2_G:  104 Hz, +-500 dps
  }

  bool read(ImuSample& s) {
    uint8_t b[12];
    // OUTX_L_G (0x22): gyro X,Y,Z then accel X,Y,Z, little-endian
    if (!i2c::readRegs(ADDR, 0x22, b, 12)) return false;
    s.gx = i2c::le16(b + 0) * GYRO_DPS_PER_LSB;
    s.gy = i2c::le16(b + 2) * GYRO_DPS_PER_LSB;
    s.gz = i2c::le16(b + 4) * GYRO_DPS_PER_LSB;
    s.ax = i2c::le16(b + 6) * ACCEL_G_PER_LSB;
    s.ay = i2c::le16(b + 8) * ACCEL_G_PER_LSB;
    s.az = i2c::le16(b + 10) * ACCEL_G_PER_LSB;
    return true;
  }

  uint8_t whoAmI = 0;

 private:
  static constexpr float ACCEL_G_PER_LSB = 0.000244f;  // +-8 g
  static constexpr float GYRO_DPS_PER_LSB = 0.0175f;   // +-500 dps
};

// --------------------------------------------------------------- MPU-6050 --
class Mpu6050 {
 public:
  static const uint8_t ADDR = 0x68;

  bool begin() {
    uint8_t who = 0;
    if (!i2c::readRegs(ADDR, 0x75, &who, 1)) return false;   // WHO_AM_I
    whoAmI = who;  // 0x68 on a genuine chip; clones may report 0x70, 0x72, 0x98...
    if (!i2c::writeReg(ADDR, 0x6B, 0x01)) return false;     // PWR_MGMT_1: wake, gyro clock
    delay(10);
    return i2c::writeReg(ADDR, 0x1A, 0x03)     // CONFIG: low-pass filter ~44 Hz
        && i2c::writeReg(ADDR, 0x19, 0x09)     // SMPLRT_DIV: 1 kHz / (1 + 9) = 100 Hz
        && i2c::writeReg(ADDR, 0x1B, 0x08)     // GYRO_CONFIG: +-500 dps
        && i2c::writeReg(ADDR, 0x1C, 0x10);    // ACCEL_CONFIG: +-8 g
  }

  bool read(ImuSample& s) {
    uint8_t b[14];
    // ACCEL_XOUT_H (0x3B): accel X,Y,Z, temperature, gyro X,Y,Z, big-endian
    if (!i2c::readRegs(ADDR, 0x3B, b, 14)) return false;
    s.ax = i2c::be16(b + 0) / ACCEL_LSB_PER_G;
    s.ay = i2c::be16(b + 2) / ACCEL_LSB_PER_G;
    s.az = i2c::be16(b + 4) / ACCEL_LSB_PER_G;
    s.gx = i2c::be16(b + 8) / GYRO_LSB_PER_DPS;
    s.gy = i2c::be16(b + 10) / GYRO_LSB_PER_DPS;
    s.gz = i2c::be16(b + 12) / GYRO_LSB_PER_DPS;
    return true;
  }

  uint8_t whoAmI = 0;

 private:
  static constexpr float ACCEL_LSB_PER_G = 4096.0f;  // +-8 g
  static constexpr float GYRO_LSB_PER_DPS = 65.5f;   // +-500 dps
};

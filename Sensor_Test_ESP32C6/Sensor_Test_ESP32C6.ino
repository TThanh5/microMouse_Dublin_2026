/*
 * ==============================================================================
 * MICROMOUSE ESP32-C6: MPU-6050 IMU DIAGNOSTIC TEST (STANDALONE)
 * Authoritative Spec: MICROMOUSE_MASTER_PROMPT.md
 * ==============================================================================
 *
 * HARDWARE CONNECTIONS:
 * - ESP32-C6 3.3V  --> MPU-6050 VCC
 * - ESP32-C6 GND   --> MPU-6050 GND
 * - ESP32-C6 GPIO6 --> MPU-6050 SDA
 * - ESP32-C6 GPIO7 --> MPU-6050 SCL
 * - (AD0 pin on MPU-6050 should be GND or floating for address 0x68)
 *
 * ARDUINO IDE SETTINGS:
 * 1. Board: "ESP32C6 Dev Module"
 * 2. Tools -> "USB CDC On Boot" -> "Enabled"  <-- CRITICAL for Serial output!
 * 3. Serial Monitor: 115200 baud
 * ==============================================================================
 */

#include <Arduino.h>
#include <Wire.h>

// --- Authoritative I2C Pinout ---
constexpr uint8_t PIN_I2C_SDA  = 6;
constexpr uint8_t PIN_I2C_SCL  = 7;
constexpr uint8_t ADDR_MPU6050 = 0x68;

// Register Map for MPU-6050
constexpr uint8_t REG_PWR_MGMT_1   = 0x6B;
constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;
constexpr uint8_t REG_WHO_AM_I     = 0x75;

bool mpuDetected = false;

// Scan I2C bus to verify physical connection
void scanI2CBus() {
  Serial.println(F("\n--- [I2C SCAN] Scanning bus on SDA=GPIO6, SCL=GPIO7 ---"));
  uint8_t count = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf(" > Found device at I2C address: 0x%02X", addr);
      if (addr == ADDR_MPU6050) {
        Serial.print(F(" [MPU-6050 IMU - MATCH!]"));
        mpuDetected = true;
      }
      Serial.println();
      count++;
    }
  }

  if (count == 0) {
    Serial.println(F("[!] WARNING: No I2C devices found!"));
    Serial.println(F("    Checklist:"));
    Serial.println(F("    1. Is VCC connected to 3.3V?"));
    Serial.println(F("    2. Is GND connected to Common GND?"));
    Serial.println(F("    3. Is SDA connected to GPIO 6?"));
    Serial.println(F("    4. Is SCL connected to GPIO 7?"));
  } else if (!mpuDetected) {
    Serial.println(F("[!] Device found on I2C, but NOT at address 0x68. Check AD0 pin wiring."));
  } else {
    Serial.println(F("[OK] MPU-6050 physically detected on I2C bus."));
  }
  Serial.println(F("----------------------------------------------------\n"));
}

// Wake up MPU-6050 and check WHO_AM_I register
bool initMPU6050() {
  Serial.println(F("[IMU] Initializing MPU-6050..."));

  // Check WHO_AM_I register (should return 0x68)
  Wire.beginTransmission(ADDR_MPU6050);
  Wire.write(REG_WHO_AM_I);
  if (Wire.endTransmission(false) == 0) {
    if (Wire.requestFrom((uint8_t)ADDR_MPU6050, (size_t)1, true) == 1) {
      uint8_t whoami = Wire.read();
      Serial.printf(" > WHO_AM_I register: 0x%02X (Expected: 0x68)\n", whoami);
    }
  }

  // Wake up MPU-6050: Write 0 to PWR_MGMT_1 register
  Wire.beginTransmission(ADDR_MPU6050);
  Wire.write(REG_PWR_MGMT_1);
  Wire.write(0x00);
  uint8_t err = Wire.endTransmission();

  if (err == 0) {
    Serial.println(F("[OK] MPU-6050 woke up and is ready to stream data!\n"));
    return true;
  } else {
    Serial.printf("[ERROR] Failed to wake up MPU-6050! Wire error code: %d\n", err);
    return false;
  }
}

void setup() {
  Serial.begin(115200);

  // Handshake for ESP32-C6 native USB-CDC
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0 < 3000)) {
    delay(10);
  }
  delay(500);

  Serial.println(F("\n\n========================================================"));
  Serial.println(F("     MICROMOUSE ESP32-C6: MPU-6050 IMU TEST BENCH       "));
  Serial.println(F("========================================================"));

  // Initialize I2C with timeout protection
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 100000);
  Wire.setTimeOut(100);
  delay(100);

  // Scan bus
  scanI2CBus();

  // Initialize IMU
  if (mpuDetected) {
    initMPU6050();
    Serial.println(F("Starting continuous telemetry (10 Hz update rate)..."));
    Serial.println(F("Tip: Tilt the robot to see Pitch/Roll and rotate to see GyroZ change."));
    Serial.println(F("--------------------------------------------------------------------------------"));
  } else {
    Serial.println(F("[!] Setup aborted: MPU-6050 not responding."));
  }
}

void loop() {
  if (!mpuDetected) {
    delay(1000);
    return;
  }

  // Read 14 bytes from MPU-6050 (Accel X/Y/Z, Temp, Gyro X/Y/Z)
  Wire.beginTransmission(ADDR_MPU6050);
  Wire.write(REG_ACCEL_XOUT_H);
  if (Wire.endTransmission(false) == 0) {
    if (Wire.requestFrom((uint8_t)ADDR_MPU6050, (size_t)14, true) == 14) {
      int16_t rawAx = (Wire.read() << 8) | Wire.read();
      int16_t rawAy = (Wire.read() << 8) | Wire.read();
      int16_t rawAz = (Wire.read() << 8) | Wire.read();
      int16_t rawTemp = (Wire.read() << 8) | Wire.read();
      int16_t rawGx = (Wire.read() << 8) | Wire.read();
      int16_t rawGy = (Wire.read() << 8) | Wire.read();
      int16_t rawGz = (Wire.read() << 8) | Wire.read();

      // Convert to physical units
      // Accelerometer full scale +/- 2g: sensitivity = 16384 LSB/g
      float ax = (float)rawAx / 16384.0f;
      float ay = (float)rawAy / 16384.0f;
      float az = (float)rawAz / 16384.0f;

      // Gyroscope full scale +/- 250 deg/s: sensitivity = 131 LSB/(deg/s)
      float gx = (float)rawGx / 131.0f;
      float gy = (float)rawGy / 131.0f;
      float gz = (float)rawGz / 131.0f;

      // Calculate approximate Pitch and Roll angles in degrees
      float pitch = atan2(ay, sqrt(ax * ax + az * az)) * 180.0f / PI;
      float roll  = atan2(-ax, az) * 180.0f / PI;

      // Print telemetry formatted
      Serial.printf("ACCEL [g]: X=%+5.2f Y=%+5.2f Z=%+5.2f | GYRO [dps]: Z(Yaw)=%+6.1f | ANGLES: Pitch=%+5.1f* Roll=%+5.1f*\n",
                    ax, ay, az, gz, pitch, roll);
    }
  }

  delay(100); // 10 Hz refresh rate
}

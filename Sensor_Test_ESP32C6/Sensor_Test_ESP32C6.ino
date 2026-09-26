/*
 * ==============================================================================
 * MICROMOUSE ESP32-C6: FULL SENSOR SUITE DIAGNOSTIC TEST (ROBUST)
 * Authoritative Spec: MICROMOUSE_MASTER_PROMPT.md
 * Target: ESP32-C6 DevKitC-1
 * ==============================================================================
 *
 * HARDWARE MAPPING:
 * - I2C SDA       : GPIO 6
 * - I2C SCL       : GPIO 7
 * - ToF Left      : XSHUT = GPIO 10  (Address: 0x30)
 * - ToF Front     : XSHUT = GPIO 11  (Address: 0x31)
 * - ToF Right     : XSHUT = GPIO 5   (Address: 0x32)
 * - IMU MPU-6050  : Address: 0x68
 *
 * ARDUINO IDE:
 * - Board: "ESP32C6 Dev Module"
 * - Tools -> "USB CDC On Boot" -> "Enabled"
 * - Serial Monitor: 115200 baud
 * ==============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

// --- Authoritative GPIO Pins ---
constexpr uint8_t PIN_I2C_SDA     = 6;
constexpr uint8_t PIN_I2C_SCL     = 7;

constexpr uint8_t PIN_XSHUT_LEFT  = 10;
constexpr uint8_t PIN_XSHUT_FRONT = 5;   // Front ToF on GPIO 5
constexpr uint8_t PIN_XSHUT_RIGHT = 11;  // Right ToF on GPIO 11

// --- I2C Target Addresses ---
constexpr uint8_t ADDR_TOF_LEFT   = 0x30;
constexpr uint8_t ADDR_TOF_FRONT  = 0x31;
constexpr uint8_t ADDR_TOF_RIGHT  = 0x32;
constexpr uint8_t ADDR_MPU6050    = 0x68;

Adafruit_VL53L0X loxLeft;
Adafruit_VL53L0X loxFront;
Adafruit_VL53L0X loxRight;

bool leftOk  = false;
bool frontOk = false;
bool rightOk = false;
bool imuOk   = false;

void scanI2CBus(const char *label) {
  Serial.printf("\n--- [I2C BUS SCAN: %s] ---\n", label);
  uint8_t count = 0;
  for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf(" > Found device at 0x%02X: ", addr);
      if (addr == ADDR_TOF_LEFT)        Serial.println(F("[ToF LEFT  - OK (0x30)]"));
      else if (addr == ADDR_TOF_FRONT)  Serial.println(F("[ToF FRONT - OK (0x31)]"));
      else if (addr == ADDR_TOF_RIGHT)  Serial.println(F("[ToF RIGHT - OK (0x32)]"));
      else if (addr == ADDR_MPU6050)    Serial.println(F("[MPU-6050 IMU - OK (0x68)]"));
      else if (addr == 0x29)            Serial.println(F("[ToF DEFAULT 0x29 (Not yet remapped!)]"));
      else                              Serial.println(F("[Other I2C device]"));
      count++;
    }
  }
  Serial.printf("Devices found: %d\n", count);
  Serial.println(F("------------------------------------------\n"));
}

bool initSingleToF(Adafruit_VL53L0X &sensor, uint8_t xshutPin, uint8_t targetAddr, const char *name) {
  Serial.printf("[ToF] Activating %s (XSHUT=GPIO %d)...\n", name, xshutPin);
  digitalWrite(xshutPin, HIGH);
  delay(60); // Allow sensor internal MCU to boot

  // Strategy 1: Attempt to begin directly with the target address
  if (sensor.begin(targetAddr, false, &Wire)) {
    Serial.printf(" > [OK] %s initialized at address 0x%02X\n", name, targetAddr);
    return true;
  }

  // Strategy 2: If begin(target) failed, initialize at default 0x29 and setAddress
  Serial.printf(" > [Fallback] Trying default 0x29 for %s...\n", name);
  if (sensor.begin(0x29, false, &Wire)) {
    if (sensor.setAddress(targetAddr)) {
      Serial.printf(" > [OK] %s remapped from 0x29 -> 0x%02X\n", name, targetAddr);
      return true;
    }
  }

  Serial.printf(" > [ERROR] %s failed to respond!\n", name);
  return false;
}

void initToFSensors() {
  Serial.println(F("\n[ToF] === STARTING SEQUENTIAL VL53L0X INITIALIZATION ==="));

  // Step 1: Put all 3 sensors into HARDWARE SHUTDOWN
  pinMode(PIN_XSHUT_LEFT, OUTPUT);
  pinMode(PIN_XSHUT_FRONT, OUTPUT);
  pinMode(PIN_XSHUT_RIGHT, OUTPUT);

  digitalWrite(PIN_XSHUT_LEFT, LOW);
  digitalWrite(PIN_XSHUT_FRONT, LOW);
  digitalWrite(PIN_XSHUT_RIGHT, LOW);
  delay(150); // Crucial: allow internal capacitors to fully discharge

  // Step 2: Initialize LEFT sensor (GPIO 10 -> 0x30)
  leftOk = initSingleToF(loxLeft, PIN_XSHUT_LEFT, ADDR_TOF_LEFT, "LEFT");

  // Step 3: Initialize FRONT sensor (GPIO 11 -> 0x31)
  frontOk = initSingleToF(loxFront, PIN_XSHUT_FRONT, ADDR_TOF_FRONT, "FRONT");

  // Step 4: Initialize RIGHT sensor (GPIO 5 -> 0x32)
  rightOk = initSingleToF(loxRight, PIN_XSHUT_RIGHT, ADDR_TOF_RIGHT, "RIGHT");
}

void initMPU6050() {
  Serial.println(F("\n[IMU] Initializing MPU-6050 at 0x68..."));
  Wire.beginTransmission(ADDR_MPU6050);
  Wire.write(0x6B); // PWR_MGMT_1
  Wire.write(0x00); // Wake up
  uint8_t err = Wire.endTransmission();
  if (err == 0) {
    imuOk = true;
    Serial.println(F(" > [OK] MPU-6050 woke up and responding!"));
  } else {
    imuOk = false;
    Serial.printf(" > [ERROR] MPU-6050 failed at 0x68 (Error code: %d)\n", err);
  }
}

void setup() {
  Serial.begin(115200);

  uint32_t t0 = millis();
  while (!Serial && (millis() - t0 < 3000)) {
    delay(10);
  }
  delay(500);

  Serial.println(F("\n\n========================================================"));
  Serial.println(F("   MICROMOUSE ESP32-C6: FULL SENSOR DIAGNOSTIC SUITE    "));
  Serial.println(F("   Left=0x30 (GPIO10) | Front=0x31 (GPIO5) | Right=0x32 (GPIO11) "));
  Serial.println(F("   IMU=0x68 (SDA=GPIO6, SCL=GPIO7)                      "));
  Serial.println(F("========================================================"));

  // Initialize I2C Bus
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 100000);
  Wire.setTimeOut(100);
  delay(100);

  // Scan before setup
  scanI2CBus("PRE-INIT STATE");

  // Initialize sensors sequentially
  initToFSensors();
  initMPU6050();

  // Scan after setup
  scanI2CBus("FINAL CONFIGURATION");

  Serial.println(F("Streaming live distance & motion data (10 Hz):"));
  Serial.println(F("--------------------------------------------------------------------------------"));
}

void loop() {
  uint16_t distLeft = 9999, distFront = 9999, distRight = 9999;
  bool validL = false, validF = false, validR = false;

  // 1. Read Left
  if (leftOk) {
    VL53L0X_RangingMeasurementData_t m;
    loxLeft.rangingTest(&m, false);
    if (m.RangeStatus != 4) {
      distLeft = m.RangeMilliMeter;
      validL = true;
    }
  }

  // 2. Read Front
  if (frontOk) {
    VL53L0X_RangingMeasurementData_t m;
    loxFront.rangingTest(&m, false);
    if (m.RangeStatus != 4) {
      distFront = m.RangeMilliMeter;
      validF = true;
    }
  }

  // 3. Read Right
  if (rightOk) {
    VL53L0X_RangingMeasurementData_t m;
    loxRight.rangingTest(&m, false);
    if (m.RangeStatus != 4) {
      distRight = m.RangeMilliMeter;
      validR = true;
    }
  }

  // 4. Read IMU
  float pitch = 0.0f, roll = 0.0f, gz = 0.0f;
  if (imuOk) {
    Wire.beginTransmission(ADDR_MPU6050);
    Wire.write(0x3B);
    if (Wire.endTransmission(false) == 0) {
      if (Wire.requestFrom((uint8_t)ADDR_MPU6050, (size_t)14, true) == 14) {
        int16_t rawAx = (Wire.read() << 8) | Wire.read();
        int16_t rawAy = (Wire.read() << 8) | Wire.read();
        int16_t rawAz = (Wire.read() << 8) | Wire.read();
        Wire.read(); Wire.read(); // temp
        Wire.read(); Wire.read(); // gx
        Wire.read(); Wire.read(); // gy
        int16_t rawGz = (Wire.read() << 8) | Wire.read();

        float ax = (float)rawAx / 16384.0f;
        float ay = (float)rawAy / 16384.0f;
        float az = (float)rawAz / 16384.0f;
        gz = (float)rawGz / 131.0f;

        pitch = atan2(ay, sqrt(ax * ax + az * az)) * 180.0f / PI;
        roll  = atan2(-ax, az) * 180.0f / PI;
      }
    }
  }

  // Print results
  Serial.print(F("ToF [mm] -> Left: "));
  if (validL) Serial.printf("%4d", distLeft); else Serial.print(F("----"));

  Serial.print(F(" | Front: "));
  if (validF) Serial.printf("%4d", distFront); else Serial.print(F("----"));

  Serial.print(F(" | Right: "));
  if (validR) Serial.printf("%4d", distRight); else Serial.print(F("----"));

  if (imuOk) {
    Serial.printf("  ||  IMU: Pitch=%+5.1f* Roll=%+5.1f* YawRate=%+6.1f dps",
                  pitch, roll, gz);
  }
  Serial.println();

  delay(100);
}

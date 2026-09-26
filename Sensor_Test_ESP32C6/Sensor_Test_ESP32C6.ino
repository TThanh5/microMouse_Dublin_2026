/*
 * ==============================================================================
 * MICROMOUSE ESP32-C6: FULL SENSOR BENCH (3x ToF VL53L0X + MPU-6050 IMU)
 * Spec: MICROMOUSE_MASTER_PROMPT.md (with Right XSHUT updated to GPIO 5)
 * ==============================================================================
 *
 * HARDWARE WIRING:
 * - Common I2C Bus:
 *     ESP32-C6 GPIO 6  --> SDA (Shared by all 4 sensors)
 *     ESP32-C6 GPIO 7  --> SCL (Shared by all 4 sensors)
 *     ESP32-C6 3.3V    --> VCC (Shared by all 4 sensors)
 *     ESP32-C6 GND     --> GND (Shared by all 4 sensors)
 *
 * - VL53L0X XSHUT Pins (Sequential Address Assignment):
 *     ESP32-C6 GPIO 10 --> Left  ToF XSHUT (Address remapped to 0x30)
 *     ESP32-C6 GPIO 11 --> Front ToF XSHUT (Address remapped to 0x31)
 *     ESP32-C6 GPIO 5  --> Right ToF XSHUT (Address remapped to 0x32)
 *
 * - MPU-6050:
 *     I2C Address: 0x68 (AD0 left unconnected or to GND)
 *
 * ARDUINO IDE SETTINGS:
 * - Board: "ESP32C6 Dev Module"
 * - Tools -> "USB CDC On Boot" -> "Enabled"
 * - Serial Monitor: 115200 baud
 * - Required Library: "Adafruit_VL53L0X" (install via Library Manager)
 * ==============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

// --- GPIO Pin Definitions ---
constexpr uint8_t PIN_I2C_SDA     = 6;
constexpr uint8_t PIN_I2C_SCL     = 7;

constexpr uint8_t PIN_XSHUT_LEFT  = 10;
constexpr uint8_t PIN_XSHUT_FRONT = 11;
constexpr uint8_t PIN_XSHUT_RIGHT = 5;  // Right ToF XSHUT kept on GPIO 5

// --- Target I2C Addresses ---
constexpr uint8_t ADDR_TOF_LEFT   = 0x30;
constexpr uint8_t ADDR_TOF_FRONT  = 0x31;
constexpr uint8_t ADDR_TOF_RIGHT  = 0x32;
constexpr uint8_t ADDR_MPU6050    = 0x68;

// VL53L0X Sensor Instances
Adafruit_VL53L0X loxLeft;
Adafruit_VL53L0X loxFront;
Adafruit_VL53L0X loxRight;

bool leftOk  = false;
bool frontOk = false;
bool rightOk = false;
bool imuOk   = false;

// Scan I2C bus and report status of all expected devices
void scanI2CBus() {
  Serial.println(F("\n--- [I2C BUS SCAN] (SDA=GPIO6, SCL=GPIO7) ---"));
  uint8_t count = 0;

  for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf(" > Found device at 0x%02X: ", addr);
      if (addr == ADDR_TOF_LEFT)        Serial.println(F("[ToF LEFT  - Address OK (0x30)]"));
      else if (addr == ADDR_TOF_FRONT)  Serial.println(F("[ToF FRONT - Address OK (0x31)]"));
      else if (addr == ADDR_TOF_RIGHT)  Serial.println(F("[ToF RIGHT - Address OK (0x32)]"));
      else if (addr == ADDR_MPU6050)    Serial.println(F("[MPU-6050 IMU - OK (0x68)]"));
      else if (addr == 0x29)            Serial.println(F("[!] WARNING: Sensor still at DEFAULT 0x29 (XSHUT failure?)"));
      else                              Serial.println(F("[Unknown device]"));
      count++;
    }
  }

  Serial.printf("Scan complete. %d device(s) found on I2C bus.\n", count);
  if (count < 4) {
    Serial.println(F("[!] Note: Expected 4 devices (0x30, 0x31, 0x32, 0x68). Check missing device wiring."));
  }
  Serial.println(F("---------------------------------------------\n"));
}

// Sequential initialization of the 3 VL53L0X sensors
void initToFSensors() {
  Serial.println(F("[ToF] Starting sequential initialization of 3 VL53L0X sensors..."));

  // Step 1: Hold all XSHUT pins LOW to keep all sensors in shutdown
  pinMode(PIN_XSHUT_LEFT, OUTPUT);
  pinMode(PIN_XSHUT_FRONT, OUTPUT);
  pinMode(PIN_XSHUT_RIGHT, OUTPUT);

  digitalWrite(PIN_XSHUT_LEFT, LOW);
  digitalWrite(PIN_XSHUT_FRONT, LOW);
  digitalWrite(PIN_XSHUT_RIGHT, LOW);
  delay(30);

  // Step 2: Wake up and initialize LEFT sensor (reassign 0x29 -> 0x30)
  Serial.printf("[ToF] Enabling LEFT sensor (GPIO %d)...\n", PIN_XSHUT_LEFT);
  digitalWrite(PIN_XSHUT_LEFT, HIGH);
  delay(50);
  if (loxLeft.begin(ADDR_TOF_LEFT, false, &Wire)) {
    leftOk = true;
    Serial.println(F(" > [OK] LEFT sensor remapped to 0x30."));
  } else {
    Serial.println(F(" > [ERROR] Failed to initialize LEFT sensor at 0x30!"));
  }

  // Step 3: Wake up and initialize FRONT sensor (reassign 0x29 -> 0x31)
  Serial.printf("[ToF] Enabling FRONT sensor (GPIO %d)...\n", PIN_XSHUT_FRONT);
  digitalWrite(PIN_XSHUT_FRONT, HIGH);
  delay(50);
  if (loxFront.begin(ADDR_TOF_FRONT, false, &Wire)) {
    frontOk = true;
    Serial.println(F(" > [OK] FRONT sensor remapped to 0x31."));
  } else {
    Serial.println(F(" > [ERROR] Failed to initialize FRONT sensor at 0x31!"));
  }

  // Step 4: Wake up and initialize RIGHT sensor (reassign 0x29 -> 0x32)
  Serial.printf("[ToF] Enabling RIGHT sensor (GPIO %d)...\n", PIN_XSHUT_RIGHT);
  digitalWrite(PIN_XSHUT_RIGHT, HIGH);
  delay(50);
  if (loxRight.begin(ADDR_TOF_RIGHT, false, &Wire)) {
    rightOk = true;
    Serial.println(F(" > [OK] RIGHT sensor remapped to 0x32."));
  } else {
    Serial.println(F(" > [ERROR] Failed to initialize RIGHT sensor at 0x32!"));
  }
}

// Initialize MPU-6050 IMU
void initMPU6050() {
  Serial.println(F("[IMU] Initializing MPU-6050 at 0x68..."));

  Wire.beginTransmission(ADDR_MPU6050);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0x00); // Wake up MPU-6050
  uint8_t err = Wire.endTransmission();

  if (err == 0) {
    imuOk = true;
    Serial.println(F(" > [OK] MPU-6050 active at 0x68."));
  } else {
    imuOk = false;
    Serial.printf(" > [ERROR] MPU-6050 failed to wake up! Error code: %d\n", err);
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
  Serial.println(F("  MICROMOUSE ESP32-C6: FULL SENSOR SUITE TEST BENCH     "));
  Serial.println(F("  Sensors: 3x VL53L0X ToF (0x30,0x31,0x32) + MPU-6050  "));
  Serial.println(F("========================================================"));

  // Initialize I2C Bus with timeout protection
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 100000);
  Wire.setTimeOut(100);
  delay(100);

  // Initialize all sensors
  initToFSensors();
  initMPU6050();

  // Run I2C Bus Verification Scan
  scanI2CBus();

  Serial.println(F("Starting continuous live sensor stream (10 Hz):"));
  Serial.println(F("Wave hands in front of Left, Front, Right sensors to test mm values."));
  Serial.println(F("--------------------------------------------------------------------------------"));
}

void loop() {
  uint16_t distLeft = 9999, distFront = 9999, distRight = 9999;
  bool validL = false, validF = false, validR = false;

  // 1. Read Left ToF
  if (leftOk) {
    VL53L0X_RangingMeasurementData_t m;
    loxLeft.rangingTest(&m, false);
    if (m.RangeStatus != 4) {
      distLeft = m.RangeMilliMeter;
      validL = true;
    }
  }

  // 2. Read Front ToF
  if (frontOk) {
    VL53L0X_RangingMeasurementData_t m;
    loxFront.rangingTest(&m, false);
    if (m.RangeStatus != 4) {
      distFront = m.RangeMilliMeter;
      validF = true;
    }
  }

  // 3. Read Right ToF
  if (rightOk) {
    VL53L0X_RangingMeasurementData_t m;
    loxRight.rangingTest(&m, false);
    if (m.RangeStatus != 4) {
      distRight = m.RangeMilliMeter;
      validR = true;
    }
  }

  // 4. Read MPU-6050
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

  // 5. Formatted Output in English
  Serial.print(F("ToF [mm] -> L: "));
  if (validL) Serial.printf("%4d", distLeft); else Serial.print(F("----"));

  Serial.print(F(" | F: "));
  if (validF) Serial.printf("%4d", distFront); else Serial.print(F("----"));

  Serial.print(F(" | R: "));
  if (validR) Serial.printf("%4d", distRight); else Serial.print(F("----"));

  if (imuOk) {
    Serial.printf("  ||  IMU -> Pitch:%+5.1f* Roll:%+5.1f* YawRate:%+6.1f dps",
                  pitch, roll, gz);
  }
  Serial.println();

  delay(100); // 10 Hz refresh
}

#include "sensors.h"
#include "config_pins.h"
#include <Adafruit_VL53L0X.h>

static Adafruit_VL53L0X s_loxLeft;
static Adafruit_VL53L0X s_loxFront;
static Adafruit_VL53L0X s_loxRight;

static bool s_leftInitialized  = false;
static bool s_frontInitialized = false;
static bool s_rightInitialized = false;
static bool s_imuInitialized   = false;

void initI2C() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 100000);
  delay(50);
}

void runI2CScanner() {
  Serial.println(F("\n--- I2C BUS SCANNER (SDA=6, SCL=7) ---"));
  uint8_t count = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();

    if (err == 0) {
      Serial.print(F("Found I2C device at 0x"));
      if (addr < 16) Serial.print(F("0"));
      Serial.print(addr, HEX);

      if (addr == ADDR_VL53L0X_DEFAULT) Serial.print(F(" [VL53L0X DEFAULT]"));
      else if (addr == ADDR_VL53L0X_LEFT)  Serial.print(F(" [VL53L0X LEFT]"));
      else if (addr == ADDR_VL53L0X_FRONT) Serial.print(F(" [VL53L0X FRONT]"));
      else if (addr == ADDR_VL53L0X_RIGHT) Serial.print(F(" [VL53L0X RIGHT]"));
      else if (addr == ADDR_MPU6050)       Serial.print(F(" [MPU-6050 IMU]"));
      Serial.println();
      count++;
    }
  }

  if (count == 0) {
    Serial.println(F("No I2C devices found! Check 3.3V power, GND, SDA (GPIO 6), SCL (GPIO 7)."));
  } else {
    Serial.printf("Scan complete. %d device(s) found.\n", count);
  }
  Serial.println(F("--------------------------------------\n"));
}

bool initVL53L0XSensors() {
  Serial.println(F("[ToF] Starting sequential VL53L0X initialization..."));

  // Step 1: Disable all ToF sensors using XSHUT
  pinMode(PIN_XSHUT_LEFT, OUTPUT);
  pinMode(PIN_XSHUT_FRONT, OUTPUT);
  pinMode(PIN_XSHUT_RIGHT, OUTPUT);

  digitalWrite(PIN_XSHUT_LEFT, LOW);
  digitalWrite(PIN_XSHUT_FRONT, LOW);
  digitalWrite(PIN_XSHUT_RIGHT, LOW);
  delay(20);

  // Step 2: Initialize Left sensor (0x29 -> 0x30)
  Serial.println(F("[ToF] Enabling Left sensor (GPIO 10)..."));
  digitalWrite(PIN_XSHUT_LEFT, HIGH);
  delay(20);
  if (s_loxLeft.begin(ADDR_VL53L0X_LEFT, false, &Wire)) {
    s_leftInitialized = true;
    Serial.println(F("[ToF] Left sensor OK at address 0x30"));
  } else {
    Serial.println(F("[ToF] ERROR: Failed to initialize Left sensor!"));
  }

  // Step 3: Initialize Front sensor (0x29 -> 0x31)
  Serial.println(F("[ToF] Enabling Front sensor (GPIO 11)..."));
  digitalWrite(PIN_XSHUT_FRONT, HIGH);
  delay(20);
  if (s_loxFront.begin(ADDR_VL53L0X_FRONT, false, &Wire)) {
    s_frontInitialized = true;
    Serial.println(F("[ToF] Front sensor OK at address 0x31"));
  } else {
    Serial.println(F("[ToF] ERROR: Failed to initialize Front sensor!"));
  }

  // Step 4: Initialize Right sensor (0x29 -> 0x32)
  Serial.println(F("[ToF] Enabling Right sensor (GPIO 14)..."));
  digitalWrite(PIN_XSHUT_RIGHT, HIGH);
  delay(20);
  if (s_loxRight.begin(ADDR_VL53L0X_RIGHT, false, &Wire)) {
    s_rightInitialized = true;
    Serial.println(F("[ToF] Right sensor OK at address 0x32"));
  } else {
    Serial.println(F("[ToF] ERROR: Failed to initialize Right sensor!"));
  }

  return s_leftInitialized && s_frontInitialized && s_rightInitialized;
}

bool initMPU6050() {
  Serial.println(F("[IMU] Initializing MPU-6050 at 0x68..."));

  // Wake up MPU-6050 (PWR_MGMT_1 register = 0)
  Wire.beginTransmission(ADDR_MPU6050);
  Wire.write(0x6B);
  Wire.write(0x00);
  uint8_t err = Wire.endTransmission();

  if (err == 0) {
    s_imuInitialized = true;
    Serial.println(F("[IMU] MPU-6050 initialized successfully."));
    return true;
  } else {
    Serial.println(F("[IMU] ERROR: MPU-6050 not responding at 0x68!"));
    s_imuInitialized = false;
    return false;
  }
}

SensorReadings readAllSensors() {
  SensorReadings r;
  r.distLeftMm  = 9999;
  r.distFrontMm = 9999;
  r.distRightMm = 9999;
  r.leftValid   = false;
  r.frontValid  = false;
  r.rightValid  = false;
  r.imuValid    = false;
  r.ax = r.ay = r.az = 0;
  r.gx = r.gy = r.gz = 0;

  // Read Left ToF
  if (s_leftInitialized) {
    VL53L0X_RangingMeasurementData_t measure;
    s_loxLeft.rangingTest(&measure, false);
    if (measure.RangeStatus != 4) {
      r.distLeftMm = measure.RangeMilliMeter;
      r.leftValid  = true;
    }
  }

  // Read Front ToF
  if (s_frontInitialized) {
    VL53L0X_RangingMeasurementData_t measure;
    s_loxFront.rangingTest(&measure, false);
    if (measure.RangeStatus != 4) {
      r.distFrontMm = measure.RangeMilliMeter;
      r.frontValid  = true;
    }
  }

  // Read Right ToF
  if (s_rightInitialized) {
    VL53L0X_RangingMeasurementData_t measure;
    s_loxRight.rangingTest(&measure, false);
    if (measure.RangeStatus != 4) {
      r.distRightMm = measure.RangeMilliMeter;
      r.rightValid  = true;
    }
  }

  // Read MPU-6050
  if (s_imuInitialized) {
    Wire.beginTransmission(ADDR_MPU6050);
    Wire.write(0x3B);
    if (Wire.endTransmission(false) == 0) {
      if (Wire.requestFrom((uint8_t)ADDR_MPU6050, (size_t)14, true) == 14) {
        r.ax = (Wire.read() << 8) | Wire.read();
        r.ay = (Wire.read() << 8) | Wire.read();
        r.az = (Wire.read() << 8) | Wire.read();
        Wire.read(); Wire.read(); // Skip temperature
        r.gx = (Wire.read() << 8) | Wire.read();
        r.gy = (Wire.read() << 8) | Wire.read();
        r.gz = (Wire.read() << 8) | Wire.read();
        r.imuValid = true;
      }
    }
  }

  return r;
}

void printSensorTelemetry() {
  SensorReadings r = readAllSensors();
  Serial.print(F("ToF [mm] | Left: "));
  if (r.leftValid) Serial.print(r.distLeftMm); else Serial.print(F("---"));
  Serial.print(F(" | Front: "));
  if (r.frontValid) Serial.print(r.distFrontMm); else Serial.print(F("---"));
  Serial.print(F(" | Right: "));
  if (r.rightValid) Serial.print(r.distRightMm); else Serial.print(F("---"));

  if (r.imuValid) {
    Serial.print(F(" || IMU AccZ: "));
    Serial.print(r.az);
    Serial.print(F(" GyrZ: "));
    Serial.print(r.gz);
  }
  Serial.println();
}

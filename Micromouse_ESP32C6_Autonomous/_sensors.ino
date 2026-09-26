// ==============================================================================
// SENSORS MODULE (3x VL53L0X ToF Laser + MPU-6050 IMU)
// ==============================================================================

Adafruit_VL53L0X loxLeft;
Adafruit_VL53L0X loxFront;
Adafruit_VL53L0X loxRight;

bool s_tofLeftOk  = false;
bool s_tofFrontOk = false;
bool s_tofRightOk = false;
bool s_imuOk      = false;
uint8_t s_mpuAddr = ADDR_MPU6050;

uint16_t distLeftMm  = 9999;
uint16_t distFrontMm = 9999;
uint16_t distRightMm = 9999;

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
      else if (addr == 0x68)            Serial.println(F("[MPU-6050 IMU (0x68)]"));
      else if (addr == 0x69)            Serial.println(F("[MPU-6050 IMU (0x69 - AD0 High)]"));
      else if (addr == 0x29)            Serial.println(F("[ToF DEFAULT 0x29 (Not yet remapped!)]"));
      else                              Serial.println(F("[Other I2C device]"));
      count++;
    }
  }
  Serial.printf("Total devices found: %d\n", count);
  Serial.println(F("------------------------------------------\n"));
}

static bool initSingleToFSensor(Adafruit_VL53L0X &sensor, uint8_t xshutPin, uint8_t targetAddr, const char *name) {
  Serial.printf("[ToF] Booting %s (XSHUT=GPIO %d)...\n", name, xshutPin);

  // Drive XSHUT HIGH to turn sensor on
  pinMode(xshutPin, OUTPUT);
  digitalWrite(xshutPin, HIGH);
  delay(80); // Wait for internal sensor boot

  // Quick I2C probe
  Wire.beginTransmission(0x29);
  bool foundAtDefault = (Wire.endTransmission() == 0);

  Wire.beginTransmission(targetAddr);
  bool foundAtTarget = (Wire.endTransmission() == 0);

  Serial.printf(" > Probe: default(0x29)=%s | target(0x%02X)=%s\n",
                foundAtDefault ? "YES" : "NO", targetAddr, foundAtTarget ? "YES" : "NO");

  // Strategy 1: Already at target address (from previous soft reset without power cycle)
  if (foundAtTarget) {
    if (sensor.begin(targetAddr, false, &Wire)) {
      Serial.printf(" > [OK] %s recognized at address 0x%02X\n", name, targetAddr);
      return true;
    }
  }

  // Strategy 2: Initialize at default 0x29, then remap to target address
  if (sensor.begin(0x29, false, &Wire)) {
    if (sensor.setAddress(targetAddr)) {
      Serial.printf(" > [OK] %s remapped 0x29 -> 0x%02X\n", name, targetAddr);
      return true;
    }
  }

  // Strategy 3: Try releasing pin as INPUT_PULLUP (some boards require open-drain high)
  pinMode(xshutPin, INPUT_PULLUP);
  delay(50);
  if (sensor.begin(0x29, false, &Wire)) {
    if (sensor.setAddress(targetAddr)) {
      Serial.printf(" > [OK] %s remapped via pullup 0x29 -> 0x%02X\n", name, targetAddr);
      return true;
    }
  }

  Serial.printf(" > [ERROR] %s sensor failed at 0x%02X!\n", name, targetAddr);
  return false;
}

void initSensors() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 100000);
  Wire.setTimeOut(100);
  delay(100);

  // 1. Initial I2C bus scan before touching XSHUT pins
  scanI2CBus("PRE-INIT STATE");

  // 2. Disable all ToF sensors using XSHUT (Hardware discharge)
  pinMode(PIN_XSHUT_LEFT, OUTPUT);
  pinMode(PIN_XSHUT_FRONT, OUTPUT);
  pinMode(PIN_XSHUT_RIGHT, OUTPUT);

  digitalWrite(PIN_XSHUT_LEFT, LOW);
  digitalWrite(PIN_XSHUT_FRONT, LOW);
  digitalWrite(PIN_XSHUT_RIGHT, LOW);
  delay(200); // Complete discharge

  // 3. Sequential Init
  s_tofLeftOk  = initSingleToFSensor(loxLeft, PIN_XSHUT_LEFT, ADDR_TOF_LEFT, "LEFT");
  s_tofFrontOk = initSingleToFSensor(loxFront, PIN_XSHUT_FRONT, ADDR_TOF_FRONT, "FRONT");
  s_tofRightOk = initSingleToFSensor(loxRight, PIN_XSHUT_RIGHT, ADDR_TOF_RIGHT, "RIGHT");

  // 4. Wake up MPU-6050 (Try 0x68 then 0x69)
  s_imuOk = false;
  s_mpuAddr = 0x68;
  Wire.beginTransmission(0x68);
  Wire.write(0x6B); // PWR_MGMT_1
  Wire.write(0x00); // Wake up
  uint8_t err = Wire.endTransmission();

  if (err == 0) {
    s_imuOk = true;
    Serial.println(F("[IMU] MPU-6050 OK at 0x68"));
  } else {
    // Try alternate address 0x69 (AD0 pulled HIGH)
    Wire.beginTransmission(0x69);
    Wire.write(0x6B);
    Wire.write(0x00);
    if (Wire.endTransmission() == 0) {
      s_imuOk = true;
      s_mpuAddr = 0x69;
      Serial.println(F("[IMU] MPU-6050 OK at alternate address 0x69 (AD0=HIGH)"));
    } else {
      Serial.printf("[IMU] ERROR: MPU-6050 failed at 0x68 (err=%d) and 0x69!\n", err);
    }
  }

  // 5. Final I2C scan to verify everything
  scanI2CBus("FINAL CONFIGURATION");
}

void readDistances() {
  if (s_tofLeftOk) {
    VL53L0X_RangingMeasurementData_t m;
    loxLeft.rangingTest(&m, false);
    distLeftMm = (m.RangeStatus != 4) ? m.RangeMilliMeter : 9999;
  }
  if (s_tofFrontOk) {
    VL53L0X_RangingMeasurementData_t m;
    loxFront.rangingTest(&m, false);
    distFrontMm = (m.RangeStatus != 4) ? m.RangeMilliMeter : 9999;
  }
  if (s_tofRightOk) {
    VL53L0X_RangingMeasurementData_t m;
    loxRight.rangingTest(&m, false);
    distRightMm = (m.RangeStatus != 4) ? m.RangeMilliMeter : 9999;
  }
}

bool hasLeftWall() {
  return distLeftMm < WALL_SIDE_THRESHOLD_MM;
}

bool hasFrontWall() {
  return distFrontMm < WALL_FRONT_THRESHOLD_MM;
}

bool hasRightWall() {
  return distRightMm < WALL_SIDE_THRESHOLD_MM;
}

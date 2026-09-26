/*
 * ==============================================================================
 * MICROMOUSE ESP32-C6: MPU-6050 IMU DIAGNOSTIC TEST (STANDALONE)
 * Authoritative Spec: MICROMOUSE_MASTER_PROMPT.md
 * ==============================================================================
 *
 * HARDWARE CONNECTIONS:
 * - ESP32-C6 3.3V  --> MPU-6050 VCC (Check if LED on IMU turns ON!)
 * - ESP32-C6 GND   --> MPU-6050 GND
 * - ESP32-C6 GPIO6 --> MPU-6050 SDA
 * - ESP32-C6 GPIO7 --> MPU-6050 SCL
 * - MPU-6050 AD0   --> GND or disconnected (unconnected = address 0x68)
 *
 * ARDUINO IDE SETTINGS:
 * - Board: "ESP32C6 Dev Module"
 * - Tools -> "USB CDC On Boot" -> "Enabled"
 * - Serial Monitor: 115200 baud
 * ==============================================================================
 */

#include <Arduino.h>
#include <Wire.h>

// --- Authoritative I2C Pinout ---
constexpr uint8_t PIN_I2C_SDA     = 6;
constexpr uint8_t PIN_I2C_SCL     = 7;
constexpr uint8_t ADDR_MPU6050_A  = 0x68; // Default address (AD0 = LOW/Floating)
constexpr uint8_t ADDR_MPU6050_B  = 0x69; // Alternate address (AD0 = HIGH)

// Register Map for MPU-6050
constexpr uint8_t REG_PWR_MGMT_1   = 0x6B;
constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;
constexpr uint8_t REG_WHO_AM_I     = 0x75;

uint8_t activeMpuAddr = 0;

// Scan I2C bus (standard addresses 0x08 to 0x77)
void scanI2CBus() {
  Serial.println(F("\n--- [I2C BUS SCAN] (SDA=GPIO6, SCL=GPIO7) ---"));
  uint8_t count = 0;
  activeMpuAddr = 0;

  // I2C valid slave address range is 0x08 to 0x77 (0x00-0x07 are I2C reserved)
  for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      Serial.printf(" > Found valid I2C device at: 0x%02X", addr);
      if (addr == ADDR_MPU6050_A) {
        Serial.print(F(" <-- [MPU-6050 DETECTED (Address 0x68)]"));
        activeMpuAddr = addr;
      } else if (addr == ADDR_MPU6050_B) {
        Serial.print(F(" <-- [MPU-6050 DETECTED (Address 0x69, AD0 is HIGH)]"));
        activeMpuAddr = addr;
      }
      Serial.println();
      count++;
    }
  }

  if (count == 0) {
    Serial.println(F("\n[!] ERROR: No I2C devices responded on 0x08 - 0x77!"));
    Serial.println(F("    >>> TROUBLESHOOTING CHECKLIST:"));
    Serial.println(F("    1. SWAP SDA and SCL WIRES:"));
    Serial.println(F("       Connect SDA -> ESP32 GPIO 6"));
    Serial.println(F("       Connect SCL -> ESP32 GPIO 7"));
    Serial.println(F("       (If unsure, try swapping GPIO 6 and 7!)"));
    Serial.println(F("    2. CHECK POWER LED:"));
    Serial.println(F("       Does the small LED on the MPU-6050 board light up?"));
    Serial.println(F("       If NOT, verify 3.3V and GND wires."));
    Serial.println(F("    3. CHECK AD0 PIN:"));
    Serial.println(F("       Leave AD0 disconnected or connect it to GND."));
  } else if (activeMpuAddr != 0) {
    Serial.printf("\n[SUCCESS] MPU-6050 is ready at address 0x%02X!\n", activeMpuAddr);
  }
  Serial.println(F("---------------------------------------------\n"));
}

bool initMPU6050(uint8_t addr) {
  Serial.printf("[IMU] Initializing MPU-6050 at 0x%02X...\n", addr);

  // Check WHO_AM_I register (0x75)
  Wire.beginTransmission(addr);
  Wire.write(REG_WHO_AM_I);
  if (Wire.endTransmission(false) == 0) {
    if (Wire.requestFrom(addr, (size_t)1, true) == 1) {
      uint8_t whoami = Wire.read();
      Serial.printf(" > WHO_AM_I register value: 0x%02X (Expected: 0x68)\n", whoami);
    }
  }

  // Wake up MPU-6050 by writing 0 to PWR_MGMT_1 register (0x6B)
  Wire.beginTransmission(addr);
  Wire.write(REG_PWR_MGMT_1);
  Wire.write(0x00);
  uint8_t err = Wire.endTransmission();

  if (err == 0) {
    Serial.println(F("[OK] MPU-6050 sensor successfully configured and active!\n"));
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

  // Initialize I2C on GPIO 6 & 7 with bus settling delay
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 100000);
  Wire.setTimeOut(100);
  delay(200);

  // Scan bus
  scanI2CBus();

  // Initialize MPU-6050 if detected
  if (activeMpuAddr != 0) {
    initMPU6050(activeMpuAddr);
    Serial.println(F("Streaming live motion telemetry (10 Hz):"));
    Serial.println(F("--------------------------------------------------------------------------------"));
  } else {
    Serial.println(F("[STOPPED] Check hardware wiring above, then press RST button to retry."));
  }
}

void loop() {
  if (activeMpuAddr == 0) {
    delay(1000);
    return;
  }

  // Read 14 bytes: Accel X/Y/Z, Temp, Gyro X/Y/Z
  Wire.beginTransmission(activeMpuAddr);
  Wire.write(REG_ACCEL_XOUT_H);
  if (Wire.endTransmission(false) == 0) {
    if (Wire.requestFrom(activeMpuAddr, (size_t)14, true) == 14) {
      int16_t rawAx = (Wire.read() << 8) | Wire.read();
      int16_t rawAy = (Wire.read() << 8) | Wire.read();
      int16_t rawAz = (Wire.read() << 8) | Wire.read();
      Wire.read(); Wire.read(); // Skip temp
      int16_t rawGx = (Wire.read() << 8) | Wire.read();
      int16_t rawGy = (Wire.read() << 8) | Wire.read();
      int16_t rawGz = (Wire.read() << 8) | Wire.read();

      // Convert units
      float ax = (float)rawAx / 16384.0f; // in g
      float ay = (float)rawAy / 16384.0f;
      float az = (float)rawAz / 16384.0f;
      float gz = (float)rawGz / 131.0f;   // in deg/sec (Yaw rate)

      // Calculate approximate Pitch and Roll
      float pitch = atan2(ay, sqrt(ax * ax + az * az)) * 180.0f / PI;
      float roll  = atan2(-ax, az) * 180.0f / PI;

      Serial.printf("ACCEL [g]: X=%+5.2f Y=%+5.2f Z=%+5.2f | YAW RATE: %+6.1f deg/s | PITCH: %+5.1f* ROLL: %+5.1f*\n",
                    ax, ay, az, gz, pitch, roll);
    }
  }

  delay(100);
}

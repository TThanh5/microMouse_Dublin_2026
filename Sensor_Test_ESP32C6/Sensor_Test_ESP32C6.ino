/*
 * ==============================================================================
 * MICROMOUSE ESP32-C6: SENSOR TEST BENCH (ToF VL53L0X + IMU MPU-6050)
 * Source of truth: MICROMOUSE_MASTER_PROMPT.md
 * ==============================================================================
 *
 * MỤC ĐÍCH:
 * File này chỉ kiểm tra độc lập 3 cảm biến khoảng cách VL53L0X và cảm biến góc MPU-6050.
 * Không chạy motor, không kích hoạt encoder. Cực kỳ an toàn, chỉ cần cắm cáp USB vào ESP32.
 *
 * SƠ ĐỒ CHÂN (THEO MASTER PROMPT):
 * - I2C SDA       : GPIO 6
 * - I2C SCL       : GPIO 7
 * - ToF Trái XSHUT : GPIO 10  (Địa chỉ I2C mới: 0x30)
 * - ToF Trước XSHUT: GPIO 11  (Địa chỉ I2C mới: 0x31)
 * - ToF Phải XSHUT : GPIO 14  (Địa chỉ I2C mới: 0x32)
 * - MPU-6050      : Địa chỉ 0x68
 * - Cấp nguồn cảm biến: 3.3V và GND từ ESP32-C6.
 *
 * THƯ VIỆN CẦN CÓ TRÊN ARDUINO IDE:
 * - "Adafruit_VL53L0X" (vào Library Manager tìm và cài đặt).
 * ==============================================================================
 */

#include <Wire.h>
#include <Adafruit_VL53L0X.h>

// --- Định nghĩa chân phần cứng ---
constexpr uint8_t PIN_I2C_SDA     = 6;
constexpr uint8_t PIN_I2C_SCL     = 7;

constexpr uint8_t PIN_XSHUT_LEFT  = 10;
constexpr uint8_t PIN_XSHUT_FRONT = 11;
constexpr uint8_t PIN_XSHUT_RIGHT = 14;

// --- Địa chỉ I2C sau khi gán tuần tự ---
constexpr uint8_t ADDR_TOF_LEFT   = 0x30;
constexpr uint8_t ADDR_TOF_FRONT  = 0x31;
constexpr uint8_t ADDR_TOF_RIGHT  = 0x32;
constexpr uint8_t ADDR_MPU6050    = 0x68;

// Đối tượng cảm biến VL53L0X
Adafruit_VL53L0X loxLeft;
Adafruit_VL53L0X loxFront;
Adafruit_VL53L0X loxRight;

bool leftOk  = false;
bool frontOk = false;
bool rightOk = false;
bool imuOk   = false;

// Hàm quét toàn bộ bus I2C để kiểm tra thiết bị có mặt
void scanI2CBus() {
  Serial.println(F("\n--- [I2C SCANNER] ĐANG QUÉT CÁC THIẾT BỊ TRÊN BUS I2C ---"));
  uint8_t count = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf(" > Thấy thiết bị tại địa chỉ: 0x%02X", addr);
      if (addr == 0x29)            Serial.print(F(" (VL53L0X Mặc định - Chưa đổi)"));
      else if (addr == ADDR_TOF_LEFT)  Serial.print(F(" (ToF TRÁI - OK)"));
      else if (addr == ADDR_TOF_FRONT) Serial.print(F(" (ToF TRƯỚC - OK)"));
      else if (addr == ADDR_TOF_RIGHT) Serial.print(F(" (ToF PHẢI - OK)"));
      else if (addr == ADDR_MPU6050)   Serial.print(F(" (MPU-6050 IMU - OK)"));
      Serial.println();
      count++;
    }
  }
  if (count == 0) {
    Serial.println(F("[!] CẢNH BÁO: Không tìm thấy bất kỳ thiết bị I2C nào!"));
    Serial.println(F("    Vui lòng kiểm tra lại dây 3.3V, GND, SDA (GPIO 6), SCL (GPIO 7)."));
  } else {
    Serial.printf("-> Quét xong. Tìm thấy %d thiết bị.\n", count);
  }
  Serial.println(F("----------------------------------------------------------\n"));
}

// Hàm khởi tạo tuần tự 3 cảm biến VL53L0X theo mục số 5 của Master Prompt
void initToFSensors() {
  Serial.println(F("[ToF] Bắt đầu quy trình khởi tạo tuần tự 3 cảm biến VL53L0X..."));

  // Bước 1: Kéo tất cả chân XSHUT xuống LOW để tắt toàn bộ 3 cảm biến
  pinMode(PIN_XSHUT_LEFT, OUTPUT);
  pinMode(PIN_XSHUT_FRONT, OUTPUT);
  pinMode(PIN_XSHUT_RIGHT, OUTPUT);

  digitalWrite(PIN_XSHUT_LEFT, LOW);
  digitalWrite(PIN_XSHUT_FRONT, LOW);
  digitalWrite(PIN_XSHUT_RIGHT, LOW);
  delay(30);

  // Bước 2: Bật cảm biến TRÁI -> Đổi địa chỉ sang 0x30
  Serial.println(F("[ToF] Đang bật cảm biến TRÁI (GPIO 10)..."));
  digitalWrite(PIN_XSHUT_LEFT, HIGH);
  delay(30);
  if (loxLeft.begin(ADDR_TOF_LEFT, false, &Wire)) {
    leftOk = true;
    Serial.println(F(" -> [OK] Cảm biến Trái đã nhận địa chỉ 0x30"));
  } else {
    Serial.println(F(" -> [LỖI] Không thể gán địa chỉ cho cảm biến Trái!"));
  }

  // Bước 3: Bật cảm biến TRƯỚC -> Đổi địa chỉ sang 0x31
  Serial.println(F("[ToF] Đang bật cảm biến TRƯỚC (GPIO 11)..."));
  digitalWrite(PIN_XSHUT_FRONT, HIGH);
  delay(30);
  if (loxFront.begin(ADDR_TOF_FRONT, false, &Wire)) {
    frontOk = true;
    Serial.println(F(" -> [OK] Cảm biến Trước đã nhận địa chỉ 0x31"));
  } else {
    Serial.println(F(" -> [LỖI] Không thể gán địa chỉ cho cảm biến Trước!"));
  }

  // Bước 4: Bật cảm biến PHẢI -> Đổi địa chỉ sang 0x32
  Serial.println(F("[ToF] Đang bật cảm biến PHẢI (GPIO 14)..."));
  digitalWrite(PIN_XSHUT_RIGHT, HIGH);
  delay(30);
  if (loxRight.begin(ADDR_TOF_RIGHT, false, &Wire)) {
    rightOk = true;
    Serial.println(F(" -> [OK] Cảm biến Phải đã nhận địa chỉ 0x32"));
  } else {
    Serial.println(F(" -> [LỖI] Không thể gán địa chỉ cho cảm biến Phải!"));
  }
}

// Khởi tạo đánh thức MPU-6050
void initMPU() {
  Serial.println(F("[IMU] Đang khởi tạo MPU-6050 tại địa chỉ 0x68..."));
  Wire.beginTransmission(ADDR_MPU6050);
  Wire.write(0x6B); // Thanh ghi quản lý nguồn PWR_MGMT_1
  Wire.write(0x00); // Ghi 0 để đánh thức chip
  if (Wire.endTransmission() == 0) {
    imuOk = true;
    Serial.println(F(" -> [OK] MPU-6050 đã thức giấc và sẵn sàng!"));
  } else {
    imuOk = false;
    Serial.println(F(" -> [LỖI] Không phản hồi từ MPU-6050 tại 0x68!"));
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000); // Chờ USB Serial trên ESP32-C6 kết nối ổn định

  Serial.println(F("\n========================================================"));
  Serial.println(F("  CHƯƠNG TRÌNH TEST CẢM BIẾN: ToF VL53L0X & IMU MPU-6050 "));
  Serial.println(F("========================================================"));

  // Khởi động bus I2C trên GPIO 6 & GPIO 7
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 100000);
  delay(100);

  // Khởi tạo các cảm biến
  initToFSensors();
  initMPU();

  // Quét I2C một lần để người dùng xác nhận kết quả
  scanI2CBus();

  Serial.println(F("Bắt đầu đọc dữ liệu liên tục (Chu kỳ 100ms)..."));
  Serial.println(F("Gợi ý: Dùng tay/bìa che lần lượt 3 hướng để thấy mm thay đổi."));
  Serial.println(F("--------------------------------------------------------------------------------"));
}

void loop() {
  uint16_t distLeft = 9999, distFront = 9999, distRight = 9999;
  bool validL = false, validF = false, validR = false;

  // 1. Đọc cảm biến khoảng cách TRÁI
  if (leftOk) {
    VL53L0X_RangingMeasurementData_t m;
    loxLeft.rangingTest(&m, false);
    if (m.RangeStatus != 4) {
      distLeft = m.RangeMilliMeter;
      validL = true;
    }
  }

  // 2. Đọc cảm biến khoảng cách TRƯỚC
  if (frontOk) {
    VL53L0X_RangingMeasurementData_t m;
    loxFront.rangingTest(&m, false);
    if (m.RangeStatus != 4) {
      distFront = m.RangeMilliMeter;
      validF = true;
    }
  }

  // 3. Đọc cảm biến khoảng cách PHẢI
  if (rightOk) {
    VL53L0X_RangingMeasurementData_t m;
    loxRight.rangingTest(&m, false);
    if (m.RangeStatus != 4) {
      distRight = m.RangeMilliMeter;
      validR = true;
    }
  }

  // 4. Đọc dữ liệu MPU-6050 (Gia tốc trục Z và Tốc độ góc quay quanh trục Z)
  int16_t az = 0, gz = 0;
  if (imuOk) {
    Wire.beginTransmission(ADDR_MPU6050);
    Wire.write(0x3B); // Đọc từ thanh ghi ACCEL_XOUT_H
    if (Wire.endTransmission(false) == 0) {
      if (Wire.requestFrom((uint8_t)ADDR_MPU6050, (size_t)14, true) == 14) {
        Wire.read(); Wire.read(); // ax
        Wire.read(); Wire.read(); // ay
        az = (Wire.read() << 8) | Wire.read(); // az (Trục thẳng đứng)
        Wire.read(); Wire.read(); // temp
        Wire.read(); Wire.read(); // gx
        Wire.read(); Wire.read(); // gy
        gz = (Wire.read() << 8) | Wire.read(); // gz (Vận tốc góc xoay z)
      }
    }
  }

  // 5. In kết quả dạng bảng đẹp mắt trên Serial Monitor
  Serial.print(F("ToF [mm] -> Trái: "));
  if (validL) Serial.printf("%4d", distLeft); else Serial.print(F("----"));

  Serial.print(F("  |  Trước: "));
  if (validF) Serial.printf("%4d", distFront); else Serial.print(F("----"));

  Serial.print(F("  |  Phải: "));
  if (validR) Serial.printf("%4d", distRight); else Serial.print(F("----"));

  if (imuOk) {
    Serial.printf("  ||  IMU [Raw] -> AccZ: %6d  GyrZ (Xoay): %6d", az, gz);
  }
  Serial.println();

  delay(100); // Tần số cập nhật 10 Hz
}

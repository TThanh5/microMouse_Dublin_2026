/*
 * ==============================================================================
 * MICROMOUSE HARDWARE BRING-UP & PRE-RUN TEST FIRMWARE
 * Target: ESP32-C6 DevKitC-1
 * Authoritative Spec: MICROMOUSE_MASTER_PROMPT.md
 * ==============================================================================
 *
 * HOW TO USE IN ARDUINO IDE:
 * 1. Open this file (Micromouse_Bringup_Test.ino) in Arduino IDE.
 * 2. Select Board: "ESP32-C6 Dev Module" (or "ESP32-C6-DevKitC-1").
 * 3. Make sure the library "Adafruit_VL53L0X" is installed in Library Manager.
 * 4. Connect USB, upload, and open Serial Monitor at 115200 baud.
 *
 * TEST MODES AVAILABLE VIA SERIAL:
 *  '1' -> Run I2C Scanner (verifies 0x30, 0x31, 0x32, 0x68)
 *  '2' -> Live ToF & IMU Monitor (test sensor ranges by waving hands)
 *  '3' -> Live Encoder Monitor (spin wheels by hand to verify tick counts)
 *  '4' -> Motor Direction Test (briefly pulses motors to check forward/reverse wiring)
 *  '5' -> AUTONOMOUS PRE-RUN (chạy thử thực tế: bò chậm, giữ làn, dừng trước tường)
 *  's' -> Emergency Stop
 *  'm' -> Show Menu
 * ==============================================================================
 */

#include "config_pins.h"
#include "motors.h"
#include "encoders.h"
#include "sensors.h"
#include "prerun.h"

enum StreamMode {
  STREAM_NONE = 0,
  STREAM_SENSORS,
  STREAM_ENCODERS
};

static StreamMode s_currentStream = STREAM_NONE;
static uint32_t s_lastStreamTime = 0;

void printMenu() {
  Serial.println(F("\n========================================================"));
  Serial.println(F("      MICROMOUSE ESP32-C6 BRING-UP & TEST RUN MENU     "));
  Serial.println(F("========================================================"));
  Serial.println(F(" [1] Run I2C Bus Scanner"));
  Serial.println(F(" [2] Live ToF & IMU Monitor (Wave hand to test distance)"));
  Serial.println(F(" [3] Live Encoder Monitor (Spin wheels by hand)"));
  Serial.println(F(" [4] Motor Direction Test (Check forward/reverse wiring)"));
  Serial.println(F(" [5] AUTONOMOUS PRE-RUN (Chạy thử: giữ tâm làn & dừng)"));
  Serial.println(F(" [s] Emergency Stop (Stop all motors immediately)"));
  Serial.println(F(" [m] Show this menu again"));
  Serial.println(F("========================================================"));
  Serial.print(F("Enter your choice: "));
}

void setup() {
  Serial.begin(115200);
  delay(1500); // Allow USB-CDC on ESP32-C6 to stabilize

  Serial.println(F("\n\n>>> INITIALIZING MICROMOUSE ESP32-C6 FIRMWARE <<<"));

  // 1. Initialize Motors (Stop immediately at boot)
  initMotors();
  Serial.println(F("[OK] Motor driver pins initialized (Motors STOPPED)."));

  // 2. Initialize Encoders
  initEncoders();
  Serial.println(F("[OK] Encoder interrupts attached (Left: 0/1, Right: 2/3)."));

  // 3. Initialize I2C Bus (SDA=6, SCL=7)
  initI2C();
  Serial.println(F("[OK] I2C bus initialized at 100 kHz on GPIO 6/7."));

  // 4. Sequential VL53L0X Initialization
  initVL53L0XSensors();

  // 5. Initialize MPU-6050
  initMPU6050();

  // 6. Display Interactive Menu
  printMenu();
}

void loop() {
  // Check for incoming user commands via Serial
  if (Serial.available()) {
    char cmd = Serial.read();

    // If streaming was active, any key stops the stream
    if (s_currentStream != STREAM_NONE && cmd != '\n' && cmd != '\r') {
      s_currentStream = STREAM_NONE;
      Serial.println(F("\n[Stream stopped.]"));
      printMenu();
      return;
    }

    switch (cmd) {
      case '1':
        s_currentStream = STREAM_NONE;
        runI2CScanner();
        printMenu();
        break;

      case '2':
        Serial.println(F("\n[Starting Live ToF & IMU Monitor... Send any key to stop]"));
        s_currentStream = STREAM_SENSORS;
        break;

      case '3':
        Serial.println(F("\n[Starting Live Encoder Monitor... Spin wheels by hand. Send any key to stop]"));
        resetEncoders();
        s_currentStream = STREAM_ENCODERS;
        break;

      case '4':
        s_currentStream = STREAM_NONE;
        testMotorsSequentially(600);
        printMenu();
        break;

      case '5':
        s_currentStream = STREAM_NONE;
        executeAutonomousPreRun(12000);
        printMenu();
        break;

      case 's':
      case 'S':
        s_currentStream = STREAM_NONE;
        stopMotors();
        Serial.println(F("\n[!] EMERGENCY STOP: Motors stopped."));
        printMenu();
        break;

      case 'm':
      case 'M':
        s_currentStream = STREAM_NONE;
        printMenu();
        break;

      default:
        // Ignore whitespace/newlines
        break;
    }
  }

  // Handle active telemetry streams
  if (s_currentStream == STREAM_SENSORS) {
    if (millis() - s_lastStreamTime >= 100) {
      s_lastStreamTime = millis();
      printSensorTelemetry();
    }
  } else if (s_currentStream == STREAM_ENCODERS) {
    if (millis() - s_lastStreamTime >= 100) {
      s_lastStreamTime = millis();
      Serial.printf("Encoder Ticks -> Left: %6ld | Right: %6ld\n",
                    (long)getLeftTicks(), (long)getRightTicks());
    }
  }
}

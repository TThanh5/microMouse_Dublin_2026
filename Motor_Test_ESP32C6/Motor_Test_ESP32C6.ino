/*
 * ==============================================================================
 * MICROMOUSE ESP32-C6: STANDALONE MOTOR BENCH TEST (TB6612FNG / DRI0044)
 * Authoritative Spec: MICROMOUSE_MASTER_PROMPT.md
 * ==============================================================================
 *
 * HARDWARE CONNECTIONS:
 * 1. Logic & Control (ESP32-C6 -> DRI0044):
 *    - ESP32-C6 GPIO 18 --> Left Motor PWM  (E1 / PWMA)
 *    - ESP32-C6 GPIO 19 --> Left Motor DIR  (M1 / DIRA)
 *    - ESP32-C6 GPIO 20 --> Right Motor PWM (E2 / PWMB)
 *    - ESP32-C6 GPIO 21 --> Right Motor DIR (M2 / DIRB)
 *    - ESP32-C6 3.3V    --> DRI0044 VCC     (Logic power)
 *    - ESP32-C6 GND     --> DRI0044 GND     (Common GND)
 *
 * 2. High Power (2S Battery -> DRI0044):
 *    - 2S Battery (+)   --> DRI0044 VM      (7.4V - 8.4V motor power)
 *    - 2S Battery (-)   --> DRI0044 GND     (Common GND with ESP32)
 *
 * 3. Motors (DRI0044 -> GA12-N20 Motors):
 *    - Driver M1 (+/-)  --> Left N20 Motor terminals
 *    - Driver M2 (+/-)  --> Right N20 Motor terminals
 *
 * SAFETY INSTRUCTION:
 * - Prop the robot up so the wheels are NOT touching the desk/floor!
 *
 * ARDUINO IDE SETTINGS:
 * - Board: "ESP32C6 Dev Module"
 * - Tools -> "USB CDC On Boot" -> "Enabled"
 * - Serial Monitor: 115200 baud
 * ==============================================================================
 */

#include <Arduino.h>

// --- Authoritative Motor GPIO Pins (from Master Prompt Section 3.1) ---
constexpr uint8_t PIN_MOTOR_LEFT_PWM  = 18;
constexpr uint8_t PIN_MOTOR_LEFT_DIR  = 19;
constexpr uint8_t PIN_MOTOR_RIGHT_PWM = 20;
constexpr uint8_t PIN_MOTOR_RIGHT_DIR = 21;

// --- Configurable Direction Inversion Flags ---
// Change to true if a motor spins backwards when given a Forward command
constexpr bool MOTOR_LEFT_INVERT  = false;
constexpr bool MOTOR_RIGHT_INVERT = false;

// Default test speed (0 - 255)
constexpr int TEST_SPEED = 110;

void stopMotors() {
  analogWrite(PIN_MOTOR_LEFT_PWM, 0);
  analogWrite(PIN_MOTOR_RIGHT_PWM, 0);
  digitalWrite(PIN_MOTOR_LEFT_DIR, LOW);
  digitalWrite(PIN_MOTOR_RIGHT_DIR, LOW);
}

void setMotorLeft(int speed) {
  speed = constrain(speed, -255, 255);
  if (MOTOR_LEFT_INVERT) speed = -speed;

  if (speed > 0) {
    digitalWrite(PIN_MOTOR_LEFT_DIR, HIGH);
    analogWrite(PIN_MOTOR_LEFT_PWM, speed);
  } else if (speed < 0) {
    digitalWrite(PIN_MOTOR_LEFT_DIR, LOW);
    analogWrite(PIN_MOTOR_LEFT_PWM, -speed);
  } else {
    analogWrite(PIN_MOTOR_LEFT_PWM, 0);
  }
}

void setMotorRight(int speed) {
  speed = constrain(speed, -255, 255);
  if (MOTOR_RIGHT_INVERT) speed = -speed;

  if (speed > 0) {
    digitalWrite(PIN_MOTOR_RIGHT_DIR, HIGH);
    analogWrite(PIN_MOTOR_RIGHT_PWM, speed);
  } else if (speed < 0) {
    digitalWrite(PIN_MOTOR_RIGHT_DIR, LOW);
    analogWrite(PIN_MOTOR_RIGHT_PWM, -speed);
  } else {
    analogWrite(PIN_MOTOR_RIGHT_PWM, 0);
  }
}

void printMenu() {
  Serial.println(F("\n========================================================"));
  Serial.println(F("       MICROMOUSE ESP32-C6: MOTOR BENCH TEST MENU       "));
  Serial.println(F("========================================================"));
  Serial.println(F(" [1] Test LEFT Motor  FORWARD  (1.0s @ Speed 110)"));
  Serial.println(F(" [2] Test LEFT Motor  REVERSE  (1.0s @ Speed 110)"));
  Serial.println(F(" [3] Test RIGHT Motor FORWARD  (1.0s @ Speed 110)"));
  Serial.println(F(" [4] Test RIGHT Motor REVERSE  (1.0s @ Speed 110)"));
  Serial.println(F(" [5] Test BOTH Motors FORWARD  (1.5s @ Speed 110)"));
  Serial.println(F(" [6] Test BOTH Motors REVERSE  (1.5s @ Speed 110)"));
  Serial.println(F(" [7] Speed Sweep Test (Ramp 0 -> 180 -> 0)"));
  Serial.println(F(" [s] EMERGENCY STOP (Stop both motors immediately)"));
  Serial.println(F(" [m] Show this menu"));
  Serial.println(F("========================================================"));
  Serial.print(F("Enter command (1-7, s, m): "));
}

void setup() {
  Serial.begin(115200);

  // Handshake for ESP32-C6 native USB-CDC
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0 < 3000)) {
    delay(10);
  }
  delay(500);

  Serial.println(F("\n\n>>> INITIALIZING MOTOR CONTROLLER <<<"));

  // Configure GPIO directions
  pinMode(PIN_MOTOR_LEFT_PWM, OUTPUT);
  pinMode(PIN_MOTOR_LEFT_DIR, OUTPUT);
  pinMode(PIN_MOTOR_RIGHT_PWM, OUTPUT);
  pinMode(PIN_MOTOR_RIGHT_DIR, OUTPUT);

  // Safety first: ensure both motors are completely stopped
  stopMotors();
  Serial.println(F("[OK] GPIO 18, 19, 20, 21 initialized. Motors STOPPED."));
  Serial.println(F("[!] WARNING: Ensure wheels are elevated off the table!"));

  printMenu();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    switch (cmd) {
      case '1':
        Serial.println(F("\n>>> [TEST 1] LEFT Motor FORWARD (1.0 sec)..."));
        setMotorLeft(TEST_SPEED);
        delay(1000);
        stopMotors();
        Serial.println(F("[OK] Left motor stopped."));
        printMenu();
        break;

      case '2':
        Serial.println(F("\n>>> [TEST 2] LEFT Motor REVERSE (1.0 sec)..."));
        setMotorLeft(-TEST_SPEED);
        delay(1000);
        stopMotors();
        Serial.println(F("[OK] Left motor stopped."));
        printMenu();
        break;

      case '3':
        Serial.println(F("\n>>> [TEST 3] RIGHT Motor FORWARD (1.0 sec)..."));
        setMotorRight(TEST_SPEED);
        delay(1000);
        stopMotors();
        Serial.println(F("[OK] Right motor stopped."));
        printMenu();
        break;

      case '4':
        Serial.println(F("\n>>> [TEST 4] RIGHT Motor REVERSE (1.0 sec)..."));
        setMotorRight(-TEST_SPEED);
        delay(1000);
        stopMotors();
        Serial.println(F("[OK] Right motor stopped."));
        printMenu();
        break;

      case '5':
        Serial.println(F("\n>>> [TEST 5] BOTH Motors FORWARD (1.5 sec)..."));
        setMotorLeft(TEST_SPEED);
        setMotorRight(TEST_SPEED);
        delay(1500);
        stopMotors();
        Serial.println(F("[OK] Both motors stopped."));
        printMenu();
        break;

      case '6':
        Serial.println(F("\n>>> [TEST 6] BOTH Motors REVERSE (1.5 sec)..."));
        setMotorLeft(-TEST_SPEED);
        setMotorRight(-TEST_SPEED);
        delay(1500);
        stopMotors();
        Serial.println(F("[OK] Both motors stopped."));
        printMenu();
        break;

      case '7': {
        Serial.println(F("\n>>> [TEST 7] Speed Ramp Test (0 -> 160 -> 0)..."));
        for (int spd = 40; spd <= 160; spd += 10) {
          setMotorLeft(spd);
          setMotorRight(spd);
          delay(120);
        }
        for (int spd = 160; spd >= 0; spd -= 10) {
          setMotorLeft(spd);
          setMotorRight(spd);
          delay(120);
        }
        stopMotors();
        Serial.println(F("[OK] Ramp test completed. Motors stopped."));
        printMenu();
        break;
      }

      case 's':
      case 'S':
      case ' ':
        stopMotors();
        Serial.println(F("\n[!] EMERGENCY STOP: All PWM cleared!"));
        printMenu();
        break;

      case 'm':
      case 'M':
        printMenu();
        break;

      default:
        // Ignore newline / carriage return
        break;
    }
  }
}

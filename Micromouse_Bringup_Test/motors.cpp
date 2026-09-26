#include "motors.h"
#include "config_pins.h"

void initMotors() {
  pinMode(PIN_MOTOR_LEFT_PWM, OUTPUT);
  pinMode(PIN_MOTOR_LEFT_DIR, OUTPUT);
  pinMode(PIN_MOTOR_RIGHT_PWM, OUTPUT);
  pinMode(PIN_MOTOR_RIGHT_DIR, OUTPUT);

  // Safety first: Stop both motors immediately at boot
  stopMotors();
}

void setMotorLeft(int speed) {
  // Constrain speed to [-255, 255]
  speed = constrain(speed, -255, 255);

  if (MOTOR_LEFT_INVERT) {
    speed = -speed;
  }

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
  // Constrain speed to [-255, 255]
  speed = constrain(speed, -255, 255);

  if (MOTOR_RIGHT_INVERT) {
    speed = -speed;
  }

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

void setMotors(int leftSpeed, int rightSpeed) {
  setMotorLeft(leftSpeed);
  setMotorRight(rightSpeed);
}

void stopMotors() {
  analogWrite(PIN_MOTOR_LEFT_PWM, 0);
  analogWrite(PIN_MOTOR_RIGHT_PWM, 0);
  digitalWrite(PIN_MOTOR_LEFT_DIR, LOW);
  digitalWrite(PIN_MOTOR_RIGHT_DIR, LOW);
}

void testMotorsSequentially(uint16_t durationMs) {
  Serial.println(F("[MOTOR TEST] Testing Left Motor FORWARD..."));
  setMotorLeft(100);
  delay(durationMs);
  stopMotors();
  delay(400);

  Serial.println(F("[MOTOR TEST] Testing Left Motor REVERSE..."));
  setMotorLeft(-100);
  delay(durationMs);
  stopMotors();
  delay(600);

  Serial.println(F("[MOTOR TEST] Testing Right Motor FORWARD..."));
  setMotorRight(100);
  delay(durationMs);
  stopMotors();
  delay(400);

  Serial.println(F("[MOTOR TEST] Testing Right Motor REVERSE..."));
  setMotorRight(-100);
  delay(durationMs);
  stopMotors();
  delay(600);

  Serial.println(F("[MOTOR TEST] Test complete. Both motors stopped."));
}

// ==============================================================================
// MOTOR CONTROL MODULE (TB6612 / DRI0044)
// ==============================================================================

void initMotors() {
  pinMode(PIN_MOTOR_LEFT_PWM, OUTPUT);
  pinMode(PIN_MOTOR_LEFT_DIR, OUTPUT);
  pinMode(PIN_MOTOR_RIGHT_PWM, OUTPUT);
  pinMode(PIN_MOTOR_RIGHT_DIR, OUTPUT);

  stopMotors();
}

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

void setMotors(int leftSpeed, int rightSpeed) {
  setMotorLeft(leftSpeed);
  setMotorRight(rightSpeed);
}

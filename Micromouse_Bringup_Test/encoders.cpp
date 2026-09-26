#include "encoders.h"
#include "config_pins.h"

static volatile int32_t s_leftTicks = 0;
static volatile int32_t s_rightTicks = 0;

void IRAM_ATTR isrLeftEncA() {
  bool a = digitalRead(PIN_ENC_LEFT_A);
  bool b = digitalRead(PIN_ENC_LEFT_B);
  if (a == b) {
    s_leftTicks++;
  } else {
    s_leftTicks--;
  }
}

void IRAM_ATTR isrRightEncA() {
  bool a = digitalRead(PIN_ENC_RIGHT_A);
  bool b = digitalRead(PIN_ENC_RIGHT_B);
  if (a != b) {
    s_rightTicks++;
  } else {
    s_rightTicks--;
  }
}

void initEncoders() {
  pinMode(PIN_ENC_LEFT_A, INPUT_PULLUP);
  pinMode(PIN_ENC_LEFT_B, INPUT_PULLUP);
  pinMode(PIN_ENC_RIGHT_A, INPUT_PULLUP);
  pinMode(PIN_ENC_RIGHT_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_ENC_LEFT_A), isrLeftEncA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_RIGHT_A), isrRightEncA, CHANGE);

  resetEncoders();
}

int32_t getLeftTicks() {
  noInterrupts();
  int32_t t = s_leftTicks;
  interrupts();
  return t;
}

int32_t getRightTicks() {
  noInterrupts();
  int32_t t = s_rightTicks;
  interrupts();
  return t;
}

void resetEncoders() {
  noInterrupts();
  s_leftTicks = 0;
  s_rightTicks = 0;
  interrupts();
}

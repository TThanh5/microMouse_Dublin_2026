// ==============================================================================
// ENCODER MODULE (QUADRATURE INTERRUPTS)
// ==============================================================================

static volatile long s_encLeftTicks  = 0;
static volatile long s_encRightTicks = 0;

void IRAM_ATTR isrLeftEncoderA() {
  bool a = digitalRead(PIN_ENC_LEFT_A);
  bool b = digitalRead(PIN_ENC_LEFT_B);
  if (a == b) {
    s_encLeftTicks++;
  } else {
    s_encLeftTicks--;
  }
}

void IRAM_ATTR isrRightEncoderA() {
  bool a = digitalRead(PIN_ENC_RIGHT_A);
  bool b = digitalRead(PIN_ENC_RIGHT_B);
  if (a != b) {
    s_encRightTicks++;
  } else {
    s_encRightTicks--;
  }
}

void initEncoders() {
  pinMode(PIN_ENC_LEFT_A, INPUT_PULLUP);
  pinMode(PIN_ENC_LEFT_B, INPUT_PULLUP);
  pinMode(PIN_ENC_RIGHT_A, INPUT_PULLUP);
  pinMode(PIN_ENC_RIGHT_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_ENC_LEFT_A), isrLeftEncoderA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_RIGHT_A), isrRightEncoderA, CHANGE);

  resetEncoders();
}

long getLeftTicks() {
  noInterrupts();
  long val = s_encLeftTicks;
  interrupts();
  return val;
}

long getRightTicks() {
  noInterrupts();
  long val = s_encRightTicks;
  interrupts();
  return val;
}

void resetEncoders() {
  noInterrupts();
  s_encLeftTicks  = 0;
  s_encRightTicks = 0;
  interrupts();
}

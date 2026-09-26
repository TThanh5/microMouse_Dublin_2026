// ==============================================================================
// MOTION CONTROL MODULE (Trapezoidal Profile & Dual PID Centering)
// ==============================================================================

void moveForwardCells(int numCells) {
  if (numCells <= 0) return;

  long targetTicks = (long)numCells * TICKS_PER_CELL;
  long startLeft  = getLeftTicks();
  long startRight = getRightTicks();

  float currentSpeed = MIN_SPEED;
  float lastWallError = 0;

  while (true) {
    long curLeft  = getLeftTicks();
    long curRight = getRightTicks();
    long avgTravelled = ((curLeft - startLeft) + (curRight - startRight)) / 2;
    long remainingTicks = targetTicks - avgTravelled;

    if (remainingTicks <= 0) break;

    // Check emergency stop via Serial
    if (Serial.available()) {
      Serial.read();
      stopMotors();
      s_robotState = STATE_IDLE;
      Serial.println(F("\n[!] Emergency stop triggered!"));
      return;
    }

    // Read laser distance sensors
    readDistances();

    // Front safety stop: If front wall appears unexpectedly close, stop immediately!
    if (hasFrontWall() && remainingTicks < (TICKS_PER_CELL / 2)) {
      Serial.println(F("[Motion] Front wall reached early. Stopping cell traversal."));
      break;
    }

    // --- 1. Acceleration / Deceleration Speed Profile ---
    if (remainingTicks < 450) {
      // Decelerate as we approach target
      if (currentSpeed > MIN_SPEED) {
        currentSpeed -= 0.6f;
      }
    } else if (currentSpeed < CRUISE_SPEED) {
      // Accelerate smoothly
      currentSpeed += 0.25f;
    }

    // --- 2. Encoder Alignment (Keep both wheels at same tick count) ---
    long encDiff = (curLeft - startLeft) - (curRight - startRight);
    float encCorrection = KP_ENC * encDiff;

    // --- 3. Wall Centering Error ---
    float wallError = 0;
    if (hasLeftWall() && hasRightWall()) {
      // Centered between both walls
      wallError = (float)((int)distLeftMm - (int)distRightMm);
    } else if (hasLeftWall()) {
      // Follow left wall
      wallError = (float)(((int)distLeftMm - (int)WALL_SIDE_NOMINAL_MM) * 2);
    } else if (hasRightWall()) {
      // Follow right wall
      wallError = (float)(((int)WALL_SIDE_NOMINAL_MM - (int)distRightMm) * 2);
    }

    float dWall = wallError - lastWallError;
    lastWallError = wallError;
    float wallCorrection = (KP_WALL * wallError) + (KD_WALL * dWall);

    // If wall error is too large (e.g. at junction opening), clamp or ignore
    wallCorrection = constrain(wallCorrection, -35.0f, 35.0f);

    // Total steering adjustment
    int steer = (int)(encCorrection - wallCorrection);

    int leftPwm  = constrain((int)(currentSpeed - steer), 0, 255);
    int rightPwm = constrain((int)(currentSpeed + steer), 0, 255);

    setMotors(leftPwm, rightPwm);
    delay(5);
  }

  stopMotors();
  delay(100); // Settling pause
}

void turnAngle(int degrees) {
  if (degrees == 0) return;

  long targetTicks = (long)(abs(degrees) * TICKS_TURN_90 / 90);
  long startLeft  = getLeftTicks();
  long startRight = getRightTicks();

  if (degrees > 0) {
    // Turn RIGHT (Left motor forward, Right motor reverse)
    setMotors(TURN_SPEED, -TURN_SPEED);
    while (true) {
      if (Serial.available()) {
        Serial.read();
        stopMotors();
        s_robotState = STATE_IDLE;
        Serial.println(F("\n[!] Emergency stop during turn!"));
        return;
      }
      long dL = abs(getLeftTicks() - startLeft);
      long dR = abs(getRightTicks() - startRight);
      if ((dL + dR) / 2 >= targetTicks) break;
      delay(2);
    }
  } else {
    // Turn LEFT (Left motor reverse, Right motor forward)
    setMotors(-TURN_SPEED, TURN_SPEED);
    while (true) {
      if (Serial.available()) {
        Serial.read();
        stopMotors();
        s_robotState = STATE_IDLE;
        Serial.println(F("\n[!] Emergency stop during turn!"));
        return;
      }
      long dL = abs(getLeftTicks() - startLeft);
      long dR = abs(getRightTicks() - startRight);
      if ((dL + dR) / 2 >= targetTicks) break;
      delay(2);
    }
  }

  stopMotors();
  delay(150); // Pause to eliminate turn momentum
}

// ==============================================================================
// RUN 1: PERIMETER LOOP (Chạy thẳng 16 ô không dừng -> quẹo phải -> đi vòng chu vi)
// ==============================================================================
void runStraightToPerimeterWall(int maxCells) {
  long maxTicks = (long)maxCells * TICKS_PER_CELL;
  long startLeft  = getLeftTicks();
  long startRight = getRightTicks();

  float currentSpeed = MIN_SPEED;
  float lastWallError = 0;

  Serial.println(F("[Perimeter] SPRINTING STRAIGHT along corridor (max 16 cells)..."));

  while (true) {
    long curLeft  = getLeftTicks();
    long curRight = getRightTicks();
    long avgTravelled = ((curLeft - startLeft) + (curRight - startRight)) / 2;
    long remainingTicks = maxTicks - avgTravelled;

    // Check emergency stop via Serial
    if (Serial.available()) {
      Serial.read();
      stopMotors();
      s_robotState = STATE_IDLE;
      Serial.println(F("\n[!] Emergency stop triggered!"));
      return;
    }

    // Read laser distance sensors
    readDistances();

    // 1. Check if we reached the corner wall at the end of the corridor
    if (distFrontMm <= WALL_FRONT_STOP_MM) {
      Serial.printf("[Perimeter] Corner wall reached! (Front: %d mm). Stopping.\n", distFrontMm);
      break;
    }

    // Safety timeout by encoder distance (reached 16 cells max)
    if (remainingTicks <= 0) {
      Serial.println(F("[Perimeter] Traveled 16 cells distance. Stopping."));
      break;
    }

    // 2. Smooth acceleration / deceleration
    // Decelerate if approaching end wall (< 350mm) or near max ticks
    if (distFrontMm <= WALL_FRONT_DECEL_MM || remainingTicks < 600) {
      if (currentSpeed > MIN_SPEED) {
        currentSpeed -= 0.8f;
        if (currentSpeed < MIN_SPEED) currentSpeed = MIN_SPEED;
      }
    } else if (currentSpeed < CRUISE_SPEED) {
      currentSpeed += 0.35f;
      if (currentSpeed > CRUISE_SPEED) currentSpeed = CRUISE_SPEED;
    }

    // 3. Encoder straight-line alignment
    long encDiff = (curLeft - startLeft) - (curRight - startRight);
    float encCorrection = KP_ENC * encDiff;

    // 4. Wall Centering Error (active if walls are present)
    float wallError = 0;
    if (hasLeftWall() && hasRightWall()) {
      wallError = (float)((int)distLeftMm - (int)distRightMm);
    } else if (hasLeftWall()) {
      wallError = (float)(((int)distLeftMm - (int)WALL_SIDE_NOMINAL_MM) * 2);
    } else if (hasRightWall()) {
      wallError = (float)(((int)WALL_SIDE_NOMINAL_MM - (int)distRightMm) * 2);
    }

    float dWall = wallError - lastWallError;
    lastWallError = wallError;
    float wallCorrection = (KP_WALL * wallError) + (KD_WALL * dWall);
    wallCorrection = constrain(wallCorrection, -35.0f, 35.0f);

    // Total steering adjustment
    int steer = (int)(encCorrection - wallCorrection);

    int leftPwm  = constrain((int)(currentSpeed - steer), 0, 255);
    int rightPwm = constrain((int)(currentSpeed + steer), 0, 255);

    setMotors(leftPwm, rightPwm);
    delay(5);
  }

  stopMotors();
  delay(200); // Pause before turning
}

void executePerimeterLoopStep() {
  // 1. Chạy thẳng không dừng tới cuối 16 ô (gặp tường trước mặt)
  runStraightToPerimeterWall(16);

  if (s_robotState != STATE_RUNNING) return;

  // 2. Quẹo phải 90 độ tại góc tường
  Serial.println(F("[Perimeter] Turning RIGHT 90 degrees at corner..."));
  turnAngle(90);

  if (s_robotState != STATE_RUNNING) return;

  delay(200); // Ổn định sau khi rẽ
  Serial.println(F("[Perimeter] Completed corner turn. Continuing around perimeter!\n"));
}

// ==============================================================================
// RUN 1: DEDICATED 2800MM CORRIDOR TEST MODE (Safety Stop & 90 deg Turn)
// Spec: Start at bottom-left corner facing North, advance 2800mm with real-time
// front obstacle/wall detection, stop safely, then turn right 90 degrees.
// Decoupled from competition maze solving.
// ==============================================================================
void runTest2800mmWithTurn() {
  long targetTicks = mmToTicks(TEST_RUN_TARGET_DIST_MM);
  long startLeft  = getLeftTicks();
  long startRight = getRightTicks();

  float currentSpeed = MIN_SPEED;
  float lastWallError = 0;
  bool obstacleEncountered = false;

  Serial.println(F("\n========================================================"));
  Serial.printf(" [TEST MODE] Starting 2800 mm Corridor Sprint (Target: %ld ticks)...\n", targetTicks);
  Serial.println(F(" Robot will cruise straight, monitor front ToF in real-time,"));
  Serial.println(F(" stop safely before any obstacle/wall, then turn 90 deg RIGHT."));
  Serial.println(F("========================================================\n"));

  while (true) {
    long curLeft  = getLeftTicks();
    long curRight = getRightTicks();
    long avgTravelled = ((curLeft - startLeft) + (curRight - startRight)) / 2;
    long remainingTicks = targetTicks - avgTravelled;
    float distTravelledMm = (float)avgTravelled * (CELL_PITCH_MM / (float)TICKS_PER_CELL);

    // 1. Emergency stop check
    if (Serial.available()) {
      Serial.read();
      stopMotors();
      s_robotState = STATE_IDLE;
      Serial.println(F("\n[!] Emergency stop triggered by user!"));
      return;
    }

    // 2. Real-time laser distance scan
    readDistances();

    // 3. FRONT OBSTACLE & WALL SAFETY CHECK (Do not assume 2800mm is always clear!)
    if (distFrontMm <= SAFETY_STOP_FRONT_MM) {
      stopMotors();
      obstacleEncountered = true;
      Serial.printf("\n[!] SAFETY STOP: Front obstacle/wall detected at %d mm! (Travelled: %.1f mm)\n",
                    distFrontMm, distTravelledMm);
      break;
    }

    // 4. Target distance reached check
    if (remainingTicks <= 0) {
      stopMotors();
      Serial.printf("\n[OK] Target distance 2800 mm completed! (Actual: %.1f mm)\n", distTravelledMm);
      break;
    }

    // 5. Speed Profile (Smooth Acceleration & Deceleration)
    if (distFrontMm <= SAFETY_DECEL_FRONT_MM || remainingTicks < mmToTicks(200.0f)) {
      // Decelerate if approaching front wall (< 350mm) or near target distance (< 200mm remaining)
      if (currentSpeed > MIN_SPEED) {
        currentSpeed -= 0.8f;
        if (currentSpeed < MIN_SPEED) currentSpeed = MIN_SPEED;
      }
    } else if (currentSpeed < CRUISE_SPEED) {
      currentSpeed += 0.35f;
      if (currentSpeed > CRUISE_SPEED) currentSpeed = CRUISE_SPEED;
    }

    // 6. Dual Encoder Synchronization
    long encDiff = (curLeft - startLeft) - (curRight - startRight);
    float encCorrection = KP_ENC * encDiff;

    // 7. Side Wall Centering (hugging left wall if along left boundary)
    float wallError = 0;
    if (hasLeftWall() && hasRightWall()) {
      wallError = (float)((int)distLeftMm - (int)distRightMm);
    } else if (hasLeftWall()) {
      wallError = (float)(((int)distLeftMm - (int)WALL_SIDE_NOMINAL_MM) * 2);
    } else if (hasRightWall()) {
      wallError = (float)(((int)WALL_SIDE_NOMINAL_MM - (int)distRightMm) * 2);
    }

    float dWall = wallError - lastWallError;
    lastWallError = wallError;
    float wallCorrection = (KP_WALL * wallError) + (KD_WALL * dWall);
    wallCorrection = constrain(wallCorrection, -35.0f, 35.0f);

    int steer = (int)(encCorrection - wallCorrection);
    int leftPwm  = constrain((int)(currentSpeed - steer), 0, 255);
    int rightPwm = constrain((int)(currentSpeed + steer), 0, 255);

    setMotors(leftPwm, rightPwm);
    delay(5);
  }

  stopMotors();
  delay(300); // Settle chassis before turning

  if (s_robotState != STATE_RUNNING) return;

  // 8. Turn Right 90 Degrees
  Serial.println(F("[TEST MODE] Turning RIGHT 90 degrees..."));
  turnAngle(90);

  stopMotors();
  s_robotState = STATE_IDLE; // Completed single run

  Serial.println(F("\n========================================================"));
  Serial.println(F(" [TEST MODE COMPLETE] 2800 mm run and 90 deg turn finished!"));
  if (obstacleEncountered) {
    Serial.println(F(" Note: Stopped early due to front obstacle detection."));
  }
  Serial.println(F("========================================================\n"));
}


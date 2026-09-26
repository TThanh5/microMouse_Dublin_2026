/*
 * ==============================================================================
 * MICROMOUSE ESP32-C6: AUTONOMOUS NAVIGATION & DEDICATED TEST BENCH
 * Spec: MICROMOUSE_MASTER_PROMPT.md & Rulebook.pdf
 * Target: ESP32-C6 DevKitC-1 + 3x VL53L0X + TB6612FNG + Encoders + MPU-6050
 * ==============================================================================
 *
 * HARDWARE CONFIGURATION:
 * - MCU: ESP32-C6 DevKitC-1
 * - Motors: Left PWM=18, Left DIR=19 | Right PWM=20, Right DIR=21
 * - Encoders: Left A/B = 0/1 | Right A/B = 2/3
 * - I2C Bus: SDA=6, SCL=7 (Shared by 3x VL53L0X and MPU-6050)
 * - VL53L0X XSHUT: Left=10 (0x30), Front=5 (0x31), Right=11 (0x32)
 *
 * MODES:
 * 1. RUN 1 (Default Test Mode): Chạy thẳng 2800 mm dọc hành lang, kiểm tra
 *    vật cản phía trước bằng ToF theo thời gian thực (dừng an toàn nếu có tường/vật cản),
 *    dừng và quay phải 90 độ.
 * 2. RUN 2: Chạy vòng lặp chu vi 16x16 (Perimeter Loop).
 * 3. RUN 3: Bám góc tường trái từng ô (Left-Wall Follower).
 * 4. RUN 4: Thuật toán giải mê cung thi đấu tối ưu (Flood Fill Solver).
 *
 * SERIAL COMMANDS (115200 baud):
 * - Send 'g' or '1' : Start RUN 1 (Test 2800mm + Rẽ phải 90 độ)
 * - Send '2'         : Start RUN 2 (Perimeter Loop 16x16)
 * - Send '3'         : Start RUN 3 (Left-Wall Follower)
 * - Send '4'         : Start RUN 4 (Flood Fill Competition Solver)
 * - Send 't'         : Test & print distance sensors snapshot
 * - Send 'r'         : Reset position & encoders
 * - Send 's' / Space : EMERGENCY STOP
 * ==============================================================================
 */

#include "_config.h"

RobotState s_robotState = STATE_IDLE;
AlgorithmMode s_algoMode = ALGO_TEST_2800MM_TURN; // Default: 2800mm Corridor Test Mode

void printSensorsDiagnostic() {
  readDistances();
  Serial.println(F("\n--- SENSOR DIAGNOSTIC SNAPSHOT ---"));
  Serial.printf("Left  ToF (0x30, GPIO 10): %4d mm | Wall: %s\n", distLeftMm, hasLeftWall() ? "YES" : "NO");
  Serial.printf("Front ToF (0x31, GPIO  5): %4d mm | Wall: %s\n", distFrontMm, hasFrontWall() ? "YES" : "NO");
  Serial.printf("Right ToF (0x32, GPIO 11): %4d mm | Wall: %s\n", distRightMm, hasRightWall() ? "YES" : "NO");
  Serial.printf("IMU MPU-6050 (0x68/0x69):  %s\n", s_imuOk ? "ONLINE" : "OFFLINE");
  Serial.println(F("----------------------------------\n"));
}

void resetToStart() {
  stopMotors();
  s_robotState = STATE_IDLE;
  currentCell = 0;
  currentDir = DIR_NORTH;
  resetEncoders();
  initMaze();
  Serial.println(F("\n[Reset] Robot reset to Start Cell (Cell 0, Facing North). Encoders zeroed.\n"));
}

void startCountdown(const char *modeName) {
  Serial.printf("\n>>> STARTING: %s IN 3 SECONDS...\n", modeName);
  for (int i = 3; i > 0; i--) {
    Serial.printf("%d...\n", i);
    delay(1000);
  }
  Serial.println(F(">>> GO! <<<\n"));
  s_robotState = STATE_RUNNING;
}

void setup() {
  Serial.begin(115200);

  // USB CDC handshake
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0 < 3000)) {
    delay(10);
  }
  delay(500);

  Serial.println(F("\n\n========================================================"));
  Serial.println(F("    MICROMOUSE ESP32-C6: AUTONOMOUS & TEST PLATFORM     "));
  Serial.println(F("========================================================"));

  // 1. Initialize Motors (safety stopped)
  initMotors();
  Serial.println(F("[OK] Motor driver initialized (Motors STOPPED)."));

  // 2. Initialize Encoders
  initEncoders();
  Serial.println(F("[OK] Quadrature encoders initialized (L:0/1, R:2/3)."));

  // 3. Initialize Sensors (3x ToF Laser + MPU-6050)
  initSensors();
  Serial.println(F("[OK] Sensors initialized (Front: GPIO 5, Right: GPIO 11, Left: GPIO 10)."));

  // 4. Initialize Maze Grid
  initMaze();
  Serial.println(F("[OK] 16x16 Maze Grid initialized with goal at (7,7)-(8,8)."));

  Serial.println(F("\n--------------------------------------------------------"));
  Serial.println(F(" DEFAULT: RUN 1 - DEDICATED 2800MM TEST RUN"));
  Serial.println(F("  -> Đặt ở ô góc dưới trái, đầu hướng lên theo cạnh trái."));
  Serial.println(F("  -> Đi thẳng mục tiêu 2800mm, liên tục quét vật cản/tường"));
  Serial.println(F("     phía trước để dừng an toàn, sau đó rẽ phải 90 độ."));
  Serial.println(F("--------------------------------------------------------"));
  Serial.println(F(" COMMANDS:"));
  Serial.println(F("  [1] or [g] : Start RUN 1 (Test 2800mm thẳng -> Dừng an toàn -> Rẽ phải 90*)"));
  Serial.println(F("  [2]        : Start RUN 2 (Perimeter Loop - Đi vòng chu vi 16x16)"));
  Serial.println(F("  [3]        : Start RUN 3 (Left-Wall Follower - Bám tường trái từng ô)"));
  Serial.println(F("  [4]        : Start RUN 4 (Flood Fill - Giải mê cung thi đấu về tâm)"));
  Serial.println(F("  [t]        : Test Sensors (Đọc nhanh khoảng cách ToF & IMU)"));
  Serial.println(F("  [r]        : Reset position to Start Cell 0 & Encoders"));
  Serial.println(F("  [s] / Space: EMERGENCY STOP (Dừng khẩn cấp bất kỳ lúc nào)"));
  Serial.println(F("--------------------------------------------------------\n"));
}

void loop() {
  // Handle Serial user inputs
  if (Serial.available()) {
    char c = Serial.read();

    if (c == 'g' || c == '1') {
      s_algoMode = ALGO_TEST_2800MM_TURN;
      startCountdown("RUN 1: TEST MODE (2800mm Sprint -> Safety Stop -> Right 90 Turn)");
    } else if (c == '2') {
      s_algoMode = ALGO_PERIMETER_LOOP;
      startCountdown("RUN 2: PERIMETER LOOP (Đi vòng chu vi 16x16)");
    } else if (c == '3') {
      s_algoMode = ALGO_LEFT_WALL_FOLLOWER;
      startCountdown("RUN 3: LEFT-WALL FOLLOWER (Bám tường trái từng ô)");
    } else if (c == '4') {
      s_algoMode = ALGO_FLOOD_FILL;
      startCountdown("RUN 4: FLOOD FILL (Giải mê cung về tâm)");
    } else if (c == 't' || c == 'T') {
      printSensorsDiagnostic();
    } else if (c == 'r' || c == 'R') {
      resetToStart();
    } else if (c == 's' || c == 'S' || c == ' ') {
      stopMotors();
      s_robotState = STATE_IDLE;
      Serial.println(F("\n[!] EMERGENCY STOP: Robot stopped by user."));
    }
  }

  // Autonomous state machine execution
  if (s_robotState == STATE_RUNNING) {
    if (s_algoMode == ALGO_TEST_2800MM_TURN) {
      // Run 1: Dedicated Hardware Test Mode
      runTest2800mmWithTurn();
    }
    else if (s_algoMode == ALGO_PERIMETER_LOOP) {
      // Run 2: Đi vòng 4 cạnh chu vi 16x16
      executePerimeterLoopStep();
    }
    else if (s_algoMode == ALGO_LEFT_WALL_FOLLOWER) {
      // Run 3: Bám góc tường trái từng ô
      if (isTarget(currentCell)) {
        stopMotors();
        s_robotState = STATE_GOAL_REACHED;
        Serial.printf("\n********************************************************\n");
        Serial.printf(" VICTORY! Center Goal reached at Cell %d (r:%d, c:%d)!\n",
                      currentCell, cellRow(currentCell), cellCol(currentCell));
        Serial.printf("********************************************************\n");
        return;
      }
      executeLeftWallFollowerStep();
    }
    else if (s_algoMode == ALGO_FLOOD_FILL) {
      // Run 4: Thuật toán giải mê cung thi đấu
      if (isTarget(currentCell)) {
        stopMotors();
        s_robotState = STATE_GOAL_REACHED;
        Serial.printf("\n********************************************************\n");
        Serial.printf(" VICTORY! Center Goal reached at Cell %d (r:%d, c:%d)!\n",
                      currentCell, cellRow(currentCell), cellCol(currentCell));
        Serial.printf("********************************************************\n");
        return;
      }

      Serial.printf("\n[Nav-FloodFill] Cell: %3d (r:%d, c:%d) | Heading: %d | Flood: %d\n",
                    currentCell, cellRow(currentCell), cellCol(currentCell),
                    currentDir, s_maze[currentCell].flood);

      updateWallsFromSensors();
      runFloodFill();
      planNextCell();

      Serial.printf("[Plan] Next Cell: %3d | Turn: %d deg | Steps: %d block(s)\n",
                    targetCell, (targetRelativeDir == 1 ? 90 : (targetRelativeDir == 2 ? 180 : (targetRelativeDir == 3 ? -90 : 0))),
                    runStepBlocks);

      executeNavigationStep();
    }
  }
}

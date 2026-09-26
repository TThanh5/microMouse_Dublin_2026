#include "prerun.h"
#include "config_pins.h"
#include "motors.h"
#include "encoders.h"
#include "sensors.h"

void executeAutonomousPreRun(uint32_t maxDurationMs) {
  Serial.println(F("\n=================================================="));
  Serial.println(F(">>> STARTING AUTONOMOUS PRE-RUN (CHẠY THỬ) <<<"));
  Serial.println(F("Place robot on track. Starting in:"));
  for (int i = 3; i > 0; i--) {
    Serial.printf("%d...\n", i);
    delay(1000);
  }
  Serial.println(F("GO! (Send ANY character to Serial to E-STOP)"));
  Serial.println(F("=================================================="));

  resetEncoders();
  uint32_t startTime = millis();
  uint32_t lastPrintTime = 0;

  while (millis() - startTime < maxDurationMs) {
    // 1. Check for manual Emergency Stop
    if (Serial.available()) {
      while (Serial.available()) Serial.read(); // Clear buffer
      Serial.println(F("\n[!] EMERGENCY STOP TRIGGERED BY USER!"));
      break;
    }

    // 2. Read sensors
    SensorReadings r = readAllSensors();

    // 3. Safety check: Front wall detection
    if (r.frontValid && r.distFrontMm <= WALL_FRONT_STOP_MM) {
      Serial.printf("\n[!] FRONT WALL DETECTED at %d mm <= %d mm. Stopping!\n",
                    r.distFrontMm, WALL_FRONT_STOP_MM);
      break;
    }

    // 4. Wall centering logic (Proportional steering trim)
    int steer = 0;
    const float Kp_wall = 0.35f;

    if (r.leftValid && r.rightValid &&
        r.distLeftMm < WALL_SIDE_MAX_MM && r.distRightMm < WALL_SIDE_MAX_MM) {
      // Both walls detected -> center between them
      int diff = (int)r.distLeftMm - (int)r.distRightMm;
      steer = constrain((int)(diff * Kp_wall), -PRE_RUN_MAX_STEER, PRE_RUN_MAX_STEER);
    } else if (r.leftValid && r.distLeftMm < WALL_SIDE_MAX_MM) {
      // Only left wall detected -> maintain nominal offset
      int diff = (int)r.distLeftMm - (int)WALL_SIDE_NOMINAL_MM;
      steer = constrain((int)(diff * Kp_wall), -PRE_RUN_MAX_STEER, PRE_RUN_MAX_STEER);
    } else if (r.rightValid && r.distRightMm < WALL_SIDE_MAX_MM) {
      // Only right wall detected -> maintain nominal offset
      int diff = (int)WALL_SIDE_NOMINAL_MM - (int)r.distRightMm;
      steer = constrain((int)(diff * Kp_wall), -PRE_RUN_MAX_STEER, PRE_RUN_MAX_STEER);
    }

    // Steer positive: turn right -> increase Left speed, decrease Right speed
    int leftSpeed  = constrain(PRE_RUN_BASE_SPEED + steer, 0, 255);
    int rightSpeed = constrain(PRE_RUN_BASE_SPEED - steer, 0, 255);

    setMotors(leftSpeed, rightSpeed);

    // 5. Periodic status printing (every 250ms)
    if (millis() - lastPrintTime >= 250) {
      lastPrintTime = millis();
      Serial.printf("L: %4d mm | F: %4d mm | R: %4d mm | Steer: %3d | Enc: [L:%ld, R:%ld]\n",
                    r.distLeftMm, r.distFrontMm, r.distRightMm, steer,
                    (long)getLeftTicks(), (long)getRightTicks());
    }

    delay(20); // 50 Hz control cycle
  }

  // Stop robot immediately upon completion or timeout
  stopMotors();
  Serial.printf("\n[PRE-RUN FINISHED] Total ticks -> Left: %ld, Right: %ld\n",
                (long)getLeftTicks(), (long)getRightTicks());
  Serial.println(F("Motors stopped. Ready for next command.\n"));
}

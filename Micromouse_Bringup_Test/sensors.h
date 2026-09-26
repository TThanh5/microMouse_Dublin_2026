#pragma once
#include <Arduino.h>
#include <Wire.h>

struct SensorReadings {
  uint16_t distLeftMm;
  uint16_t distFrontMm;
  uint16_t distRightMm;
  bool leftValid;
  bool frontValid;
  bool rightValid;
  int16_t ax, ay, az;
  int16_t gx, gy, gz;
  bool imuValid;
};

void initI2C();
void runI2CScanner();
bool initVL53L0XSensors();
bool initMPU6050();
SensorReadings readAllSensors();
void printSensorTelemetry();

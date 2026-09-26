#pragma once
#include <Arduino.h>

void initMotors();
void setMotorLeft(int speed);
void setMotorRight(int speed);
void setMotors(int leftSpeed, int rightSpeed);
void stopMotors();
void testMotorsSequentially(uint16_t durationMs);

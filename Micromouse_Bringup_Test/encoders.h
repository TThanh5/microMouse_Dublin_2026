#pragma once
#include <Arduino.h>

void initEncoders();
int32_t getLeftTicks();
int32_t getRightTicks();
void resetEncoders();

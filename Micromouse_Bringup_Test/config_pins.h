#pragma once
#include <Arduino.h>

// ==============================================================================
// MICROMOUSE AUTHORITATIVE PIN & SYSTEM CONFIGURATION
// Source of truth: MICROMOUSE_MASTER_PROMPT.md
// Target: ESP32-C6 (ESP32-C6-DevKitC-1)
// ==============================================================================

// --- 3.1 Motor Driver (DFRobot DRI0044 / TB6612FNG) ---
constexpr uint8_t PIN_MOTOR_LEFT_PWM  = 18;
constexpr uint8_t PIN_MOTOR_LEFT_DIR  = 19;
constexpr uint8_t PIN_MOTOR_RIGHT_PWM = 20;
constexpr uint8_t PIN_MOTOR_RIGHT_DIR = 21;

// --- 3.2 Left Encoder ---
constexpr uint8_t PIN_ENC_LEFT_A = 0;
constexpr uint8_t PIN_ENC_LEFT_B = 1;

// --- 3.3 Right Encoder ---
constexpr uint8_t PIN_ENC_RIGHT_A = 2;
constexpr uint8_t PIN_ENC_RIGHT_B = 3;

// --- 3.4 I2C Bus ---
constexpr uint8_t PIN_I2C_SDA = 6;
constexpr uint8_t PIN_I2C_SCL = 7;

// --- 3.5 VL53L0X XSHUT & I2C Addresses ---
constexpr uint8_t PIN_XSHUT_LEFT  = 10;
constexpr uint8_t PIN_XSHUT_FRONT = 11;
constexpr uint8_t PIN_XSHUT_RIGHT = 5;   // Changed from 14 to 5 (GPIO 14 not broken out)

constexpr uint8_t ADDR_VL53L0X_DEFAULT = 0x29;
constexpr uint8_t ADDR_VL53L0X_LEFT    = 0x30;
constexpr uint8_t ADDR_VL53L0X_FRONT   = 0x31;
constexpr uint8_t ADDR_VL53L0X_RIGHT   = 0x32;
constexpr uint8_t ADDR_MPU6050         = 0x68;

// ==============================================================================
// CONFIGURABLE TBD PARAMETERS (Adjustable for Hackathon Bring-up)
// ==============================================================================

// Set to true if a motor spins backwards when given positive speed command
constexpr bool MOTOR_LEFT_INVERT  = false;
constexpr bool MOTOR_RIGHT_INVERT = false;

// Safe crawl speed for initial pre-run (0 - 255)
constexpr int PRE_RUN_BASE_SPEED = 90;

// Maximum steering adjustment speed differential
constexpr int PRE_RUN_MAX_STEER = 35;

// Distance thresholds (mm)
constexpr uint16_t WALL_FRONT_STOP_MM = 100;  // Emergency stop threshold if front wall ahead
constexpr uint16_t WALL_SIDE_MAX_MM    = 180;  // Max distance to consider a side wall valid
constexpr uint16_t WALL_SIDE_NOMINAL_MM = 85;  // Nominal centered distance in a standard cell

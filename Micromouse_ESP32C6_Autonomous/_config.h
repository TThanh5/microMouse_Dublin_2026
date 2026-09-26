#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

// ==============================================================================
// MICROMOUSE ESP32-C6: CENTRAL CONFIGURATION & SHARED DECLARATIONS
// Target: ESP32-C6-DevKitC-1
// Spec: MICROMOUSE_MASTER_PROMPT.md
// ==============================================================================

// --- 1. Motor Driver Pins (DFRobot DRI0044 / TB6612FNG) ---
constexpr uint8_t PIN_MOTOR_LEFT_PWM  = 18;
constexpr uint8_t PIN_MOTOR_LEFT_DIR  = 19;
constexpr uint8_t PIN_MOTOR_RIGHT_PWM = 20;
constexpr uint8_t PIN_MOTOR_RIGHT_DIR = 21;

// --- 2. Encoder Pins (Quadrature Hall-effect) ---
constexpr uint8_t PIN_ENC_LEFT_A  = 0;
constexpr uint8_t PIN_ENC_LEFT_B  = 1;
constexpr uint8_t PIN_ENC_RIGHT_A = 2;
constexpr uint8_t PIN_ENC_RIGHT_B = 3;

// --- 3. I2C Bus Pins ---
constexpr uint8_t PIN_I2C_SDA = 6;
constexpr uint8_t PIN_I2C_SCL = 7;

// --- 4. VL53L0X XSHUT Pins & Addresses ---
constexpr uint8_t PIN_XSHUT_LEFT  = 10;
constexpr uint8_t PIN_XSHUT_FRONT = 5;   // Front ToF on GPIO 5
constexpr uint8_t PIN_XSHUT_RIGHT = 11;  // Right ToF on GPIO 11

constexpr uint8_t ADDR_TOF_LEFT   = 0x30;
constexpr uint8_t ADDR_TOF_FRONT  = 0x31;
constexpr uint8_t ADDR_TOF_RIGHT  = 0x32;
constexpr uint8_t ADDR_MPU6050    = 0x68;

// --- 5. Physical Dimensions & Calibration (TBD: Tune on physical robot) ---
constexpr float CELL_PITCH_MM           = 180.0f;  // 1 maze cell pitch (180 mm)
constexpr float TEST_RUN_TARGET_DIST_MM = 2800.0f; // Target distance for 16-cell corridor run (2800 mm)

constexpr bool MOTOR_LEFT_INVERT  = false;
constexpr bool MOTOR_RIGHT_INVERT = false;

constexpr int CRUISE_SPEED      = 100; // Normal cruise speed
constexpr int TURN_SPEED        = 85;  // Speed during 90/180 degree turns
constexpr int MIN_SPEED         = 40;  // Minimum start speed for smooth acceleration

// Encoder Calibration (TBD: Calibrate on track with wheel diameter)
constexpr long TICKS_PER_CELL    = 1500; // Encoder counts to travel 1 cell (180mm)
constexpr long TICKS_TURN_90     = 420;  // Encoder counts to turn robot 90 degrees

inline long mmToTicks(float mm) {
  return (long)(mm * (float)TICKS_PER_CELL / CELL_PITCH_MM);
}

// Laser ToF Distance Thresholds (in mm)
constexpr uint16_t WALL_FRONT_THRESHOLD_MM = 150; // Threshold for front wall detection in maze
constexpr uint16_t WALL_SIDE_THRESHOLD_MM  = 165; // Threshold for side wall detection
constexpr uint16_t WALL_SIDE_NOMINAL_MM    = 85;  // Nominal distance when centered in cell
constexpr uint16_t WALL_FRONT_STOP_MM      = 85;  // Target front distance at cell center stop
constexpr uint16_t WALL_FRONT_DECEL_MM     = 350; // Deceleration trigger distance

// Real-time obstacle safety stop for 2800mm test mode
constexpr uint16_t SAFETY_STOP_FRONT_MM  = 120; // Emergency brake if front obstacle < 120 mm
constexpr uint16_t SAFETY_DECEL_FRONT_MM = 350; // Smooth deceleration if front obstacle < 350 mm

// PID Centering Gains
constexpr float KP_WALL = 0.35f;
constexpr float KD_WALL = 1.20f;
constexpr float KP_ENC  = 0.40f;

// --- 6. Maze Grid & Directions ---
constexpr uint8_t MAZE_ROWS = 16;
constexpr uint8_t MAZE_COLS = 16;
constexpr uint16_t TOTAL_CELLS = MAZE_ROWS * MAZE_COLS;

constexpr uint8_t DIR_NORTH = 0;
constexpr uint8_t DIR_EAST  = 1;
constexpr uint8_t DIR_SOUTH = 2;
constexpr uint8_t DIR_WEST  = 3;

struct CellInfo {
  uint8_t flood;
  uint8_t walls;   // Bit 0: North, Bit 1: East, Bit 2: South, Bit 3: West
  uint8_t visited;
};

// Navigation Algorithm & Test Modes
enum AlgorithmMode {
  ALGO_TEST_2800MM_TURN   = 1,  // Run 1 (Test Mode): Đi thẳng 2800mm kiểm tra vật cản -> Dừng an toàn -> Rẽ phải 90 độ
  ALGO_PERIMETER_LOOP     = 2,  // Run 2: Đi vòng 4 cạnh chu vi 16x16 liên tục
  ALGO_LEFT_WALL_FOLLOWER = 3,  // Run 3: Bám góc tường trái từng ô (Left-Wall Follower)
  ALGO_FLOOD_FILL         = 4   // Run 4: Thuật toán Flood Fill giải mê cung thi đấu
};

enum RobotState {
  STATE_IDLE = 0,
  STATE_RUNNING,
  STATE_GOAL_REACHED
};

// ==============================================================================
// GLOBAL SHARED STATE DECLARATIONS
// ==============================================================================
extern RobotState s_robotState;
extern AlgorithmMode s_algoMode;

// Sensors
extern uint16_t distLeftMm;
extern uint16_t distFrontMm;
extern uint16_t distRightMm;
extern bool s_tofLeftOk;
extern bool s_tofFrontOk;
extern bool s_tofRightOk;
extern bool s_imuOk;

// Maze & Algorithm
extern CellInfo s_maze[TOTAL_CELLS];
extern uint8_t currentCell;
extern uint8_t currentDir;
extern uint8_t targetCell;
extern uint8_t targetRelativeDir;
extern uint8_t runStepBlocks;

// ==============================================================================
// FUNCTION PROTOTYPES (Cross-tab communication)
// ==============================================================================
// Motors
void initMotors();
void stopMotors();
void setMotorLeft(int speed);
void setMotorRight(int speed);
void setMotors(int leftSpeed, int rightSpeed);

// Encoders
void initEncoders();
void resetEncoders();
long getLeftTicks();
long getRightTicks();

// Sensors
void initSensors();
void readDistances();
bool hasLeftWall();
bool hasFrontWall();
bool hasRightWall();

// Motion & Dedicated Tests
void moveForwardCells(int numCells);
void turnAngle(int degrees);
void runStraightToPerimeterWall(int maxCells = 16);
void executePerimeterLoopStep();
void runTest2800mmWithTurn();

// Maze & Flood Fill
void initMaze();
bool isTarget(uint8_t loc);
uint8_t cellRow(uint8_t loc);
uint8_t cellCol(uint8_t loc);
void updateWallsFromSensors();
void runFloodFill();
void planNextCell();
void executeNavigationStep();
void executeLeftWallFollowerStep();

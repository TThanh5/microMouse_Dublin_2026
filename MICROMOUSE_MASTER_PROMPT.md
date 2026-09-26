# MICROMOUSE ROBOT — MASTER PROJECT PROMPT
## Hardware Specification, Firmware Rules & Development Workflow

**Document role:** This file is the authoritative project specification for the Micromouse robot firmware.

**Primary target:** ESP32-C6 (Espressif DevKitC-1), Arduino/C++ environment.

---

## 0. INSTRUCTIONS TO THE AI / ANTIGRAVITY AGENT

Treat this document as the **source of truth for the hardware configuration and development constraints**.

### Mandatory rules

1. Do NOT change GPIO assignments unless explicitly requested by the user.
2. Do NOT invent hardware specifications that are not provided.
3. If a required specification is missing, identify it as `TBD` and ask for it or clearly isolate the assumption.
4. Do NOT change I2C addresses defined in this document.
5. Never drive an ESP32-C6 GPIO above 3.3 V.
6. Do not drive motors directly from ESP32 GPIOs.
7. Motor power and logic power are separate; they share a common GND.
8. The three VL53L0X sensors MUST be initialized sequentially using their XSHUT pins because all sensors start at address `0x29`.
9. Keep hardware abstraction separate from navigation/maze logic.
10. Prefer modular, testable firmware over a single large `main.cpp`.
11. Before adding a library/API, verify that it is appropriate for ESP32-C6 and the Arduino framework.
12. When modifying existing code, preserve working hardware interfaces unless the user explicitly asks for a redesign.
13. When uncertain about a hardware behavior, explain the uncertainty instead of silently guessing.
14. Never silently remap pins, invert motor direction, change sensor addresses, or change voltage assumptions.

---

# 1. SYSTEM OVERVIEW

## 1.1 Main controller

- MCU: **ESP32-C6**
- Board: **Espressif ESP32-C6-DevKitC-1**
- GPIO logic level: **3.3 V**
- Firmware language: **C++**
- Framework: **Arduino / Arduino-compatible ESP32-C6 environment**

## 1.2 Main peripherals

The robot consists of:

- 2 × GA12-N20 geared DC motors
- 2 × quadrature Hall-effect motor encoders
- 1 × DFRobot DRI0044 / TB6612FNG motor driver
- 1 × MPU-6050 IMU
- 3 × VL53L0X ToF distance sensors
- 1 × 2S Li-ion/LiPo battery
- 1 × 5 V buck converter (MP1584EN / LM2596)
- 1 × ESP32-C6 DevKitC-1

## 1.3 Functional concept

The robot uses:

- **Encoders** for wheel speed and odometry
- **MPU-6050** for inertial/angular information
- **VL53L0X sensors** for wall/distance detection
- **Motor driver** for bidirectional motor PWM control
- **ESP32-C6** as the central controller
- A future control/navigation stack for:
  - motor control
  - PID velocity control
  - wall following
  - odometry
  - maze mapping
  - maze solving
  - motion planning

---

# 2. POWER ARCHITECTURE

## 2.1 Battery

Battery:

- Type: 2S Li-ion / LiPo
- Nominal voltage: **7.4 V**
- Fully charged voltage: **8.4 V**

## 2.2 Motor power

The 2S battery supplies the motor driver's `VM` motor-power input directly.

```text
2S Battery (+) ───────────────> DRI0044 VM
2S Battery (-) ───────────────> Common GND
```

## 2.3 ESP32 power

The 2S battery must NOT be connected directly to the ESP32 5 V input.

Use a buck converter:

```text
2S Battery
    │
    ▼
Buck Converter
    │
    ├── OUT+ = precisely 5.00 V ──> ESP32-C6 5V/VBUS input
    └── OUT- ─────────────────────> Common GND
```

**Important:** Verify the buck converter output is approximately **5.00 V before connecting it to the ESP32.**

## 2.4 3.3 V peripherals

The ESP32-C6 3.3 V rail supplies:

- DRI0044 logic VCC
- Left encoder
- Right encoder
- MPU-6050
- VL53L0X sensors

All devices share a common GND.

### Power rule

The 3.3 V rail must not be assumed to have unlimited current capacity. If the combined peripheral current approaches the board/regulator capability, use an appropriate external 3.3 V regulator.

---

# 3. AUTHORITATIVE GPIO PIN MAP

## 3.1 Motor driver

| Function | ESP32-C6 GPIO |
|---|---:|
| Left motor PWM | GPIO 18 |
| Left motor DIR | GPIO 19 |
| Right motor PWM | GPIO 20 |
| Right motor DIR | GPIO 21 |

Firmware constants:

```cpp
constexpr uint8_t PIN_MOTOR_LEFT_PWM  = 18;
constexpr uint8_t PIN_MOTOR_LEFT_DIR  = 19;
constexpr uint8_t PIN_MOTOR_RIGHT_PWM = 20;
constexpr uint8_t PIN_MOTOR_RIGHT_DIR = 21;
```

## 3.2 Left encoder

| Function | GPIO |
|---|---:|
| Encoder A | GPIO 0 |
| Encoder B | GPIO 1 |

```cpp
constexpr uint8_t PIN_ENC_LEFT_A = 0;
constexpr uint8_t PIN_ENC_LEFT_B = 1;
```

## 3.3 Right encoder

| Function | GPIO |
|---|---:|
| Encoder A | GPIO 2 |
| Encoder B | GPIO 3 |

```cpp
constexpr uint8_t PIN_ENC_RIGHT_A = 2;
constexpr uint8_t PIN_ENC_RIGHT_B = 3;
```

Encoder signals are quadrature signals:

- Channel A: pulse/timing reference
- Channel B: direction/phase information

## 3.4 I2C bus

One shared I2C bus is used by:

- MPU-6050
- VL53L0X left
- VL53L0X front
- VL53L0X right

| I2C signal | GPIO |
|---|---:|
| SDA | GPIO 6 |
| SCL | GPIO 7 |

```cpp
constexpr uint8_t PIN_I2C_SDA = 6;
constexpr uint8_t PIN_I2C_SCL = 7;
```

## 3.5 VL53L0X XSHUT

| Sensor | XSHUT GPIO | Final I2C address |
|---|---:|---:|
| Left | GPIO 10 | `0x30` |
| Front | GPIO 11 | `0x31` |
| Right | GPIO 5 | `0x32` |

*(Note: GPIO 14 is not physically broken out on the ESP32-C6-DevKitC-1 pin headers; Right XSHUT is assigned to GPIO 5).*

```cpp
constexpr uint8_t PIN_XSHUT_LEFT  = 10;
constexpr uint8_t PIN_XSHUT_FRONT = 11;
constexpr uint8_t PIN_XSHUT_RIGHT = 5;
```

---

# 4. I2C ADDRESS MAP

| Device | Address |
|---|---:|
| VL53L0X default | `0x29` |
| VL53L0X Left | `0x30` |
| VL53L0X Front | `0x31` |
| VL53L0X Right | `0x32` |
| MPU-6050 | `0x68` |

The three VL53L0X sensors cannot remain simultaneously at `0x29`.

---

# 5. VL53L0X INITIALIZATION SEQUENCE

This sequence is mandatory.

## Step 1 — Disable all ToF sensors

Set:

```text
XSHUT_LEFT  = LOW
XSHUT_FRONT = LOW
XSHUT_RIGHT = LOW
```

This ensures all three sensors are inactive.

## Step 2 — Start I2C

Initialize:

```text
SDA = GPIO 6
SCL = GPIO 7
```

## Step 3 — Initialize Left sensor

```text
XSHUT_LEFT = HIGH
```

The left sensor appears at:

```text
0x29
```

Initialize it and change its address:

```text
0x29 → 0x30
```

## Step 4 — Initialize Front sensor

```text
XSHUT_FRONT = HIGH
```

The front sensor now appears at:

```text
0x29
```

Change:

```text
0x29 → 0x31
```

## Step 5 — Initialize Right sensor

```text
XSHUT_RIGHT = HIGH
```

The right sensor appears at:

```text
0x29
```

Change:

```text
0x29 → 0x32
```

## Final state

```text
Left  = 0x30
Front = 0x31
Right = 0x32
```

The sensors can then operate on the same I2C bus.

---

# 6. BOOT / HARDWARE INITIALIZATION WORKFLOW

Recommended initialization order:

```text
BOOT
 │
 ├── Configure safety/default GPIO states
 │
 ├── Disable all VL53L0X using XSHUT
 │
 ├── Initialize I2C
 │
 ├── Initialize VL53L0X Left
 │      └── assign 0x30
 │
 ├── Initialize VL53L0X Front
 │      └── assign 0x31
 │
 ├── Initialize VL53L0X Right
 │      └── assign 0x32
 │
 ├── Initialize MPU-6050 at 0x68
 │
 ├── Initialize motor driver GPIO
 │
 ├── Initialize PWM
 │
 ├── Initialize encoder GPIO
 │
 ├── Attach encoder interrupts
 │
 └── Enter main control loop
```

---

# 7. FIRMWARE ARCHITECTURE

The firmware should be modular.

Recommended structure:

```text
src/
│
├── main.cpp
│
├── config/
│   └── pins.h
│
├── hardware/
│   ├── motor_driver.*
│   ├── encoder.*
│   ├── imu.*
│   └── tof.*
│
├── control/
│   ├── pid.*
│   ├── velocity_controller.*
│   └── wall_controller.*
│
├── navigation/
│   ├── odometry.*
│   ├── maze_map.*
│   ├── maze_solver.*
│   └── motion_planner.*
│
└── utils/
    ├── timing.*
    └── filters.*
```

The exact directory structure may be adapted to the project, but the separation of responsibilities should remain.

---

# 8. SOFTWARE LAYERS

## Layer 1 — Hardware abstraction

Responsible for direct hardware access:

- GPIO
- PWM
- I2C
- encoder interrupts
- motor driver
- IMU
- ToF sensors

## Layer 2 — Sensor processing

Responsible for:

- encoder pulse decoding
- wheel speed calculation
- distance filtering
- IMU processing
- sensor validity checks

## Layer 3 — Control

Responsible for:

- motor PWM control
- wheel velocity PID
- differential-drive control
- wall-following control

## Layer 4 — State estimation

Responsible for:

- wheel odometry
- heading estimation
- robot position
- sensor fusion where appropriate

## Layer 5 — Navigation

Responsible for:

- detecting walls
- maintaining the maze map
- selecting movement direction
- maze solving

## Layer 6 — Motion planning

Responsible for:

- acceleration/deceleration
- cell-to-cell movement
- turns
- stopping
- velocity profiles

---

# 9. DEVELOPMENT WORKFLOW

When implementing a new feature, follow this workflow.

### Step 1 — Understand the hardware

Read this document first.

Do not start coding until the required hardware interface is understood.

### Step 2 — Check dependencies

Identify:

- required library
- required GPIO
- I2C address
- timing requirements
- interrupt requirements
- voltage requirements

### Step 3 — Implement hardware driver

Create a small isolated driver.

Example:

```text
VL53L0X driver
    ↓
test initialization
    ↓
test distance reading
```

### Step 4 — Test hardware independently

Before integrating with navigation:

- test motors
- test encoders
- test IMU
- test ToF sensors

### Step 5 — Add control logic

Example:

```text
Encoder
   ↓
Wheel speed
   ↓
PID
   ↓
PWM
   ↓
Motor
```

### Step 6 — Integrate sensors

Example:

```text
VL53L0X + Encoder + IMU
              ↓
          Robot state
```

### Step 7 — Add navigation

Only after the lower-level hardware and control systems are stable.

---

# 10. TESTING PRIORITY

Test in this order:

```text
1. Power
2. ESP32 boot
3. I2C bus
4. VL53L0X initialization/address assignment
5. MPU-6050
6. Motor driver
7. Encoder signals
8. Motor + encoder closed loop
9. PID
10. Wall sensing
11. Wall following
12. Odometry
13. Maze mapping
14. Maze solving
15. Full motion planner
```

Do not debug high-level maze logic while low-level hardware is known to be unreliable.

---

# 11. SAFETY / FAILURE HANDLING

Firmware should fail safely.

At boot:

- Motors should remain stopped until initialization succeeds.
- Invalid sensor initialization should be reported.
- I2C failures should not silently produce valid-looking measurements.
- Encoder values should be checked for unreasonable values.
- Sensor readings should have validity handling.
- Motor commands should have configurable limits.
- A watchdog or equivalent recovery mechanism may be added where appropriate.

If a critical hardware component fails, the robot should prefer stopping the motors over continuing uncontrolled motion.

---

# 12. DIAGNOSTIC MODE

A dedicated diagnostic/test mode is recommended.

It should eventually allow testing:

```text
[1] I2C scan
[2] VL53L0X test
[3] MPU-6050 test
[4] Left motor test
[5] Right motor test
[6] Encoder test
[7] PWM test
[8] PID test
[9] Sensor telemetry
```

Diagnostic code should be kept separate from the normal autonomous navigation state machine.

---

# 13. CURRENTLY UNDEFINED / TBD PARAMETERS

Do NOT invent these values.

They must be supplied before accurate motion-control calculations are finalized.

## Motors

- Motor rated voltage: `TBD`
- Gear ratio: `TBD`
- Motor maximum RPM: `TBD`
- Motor direction convention: `TBD`

## Encoders

- Encoder CPR/PPR: `TBD`
- Counts per revolution after quadrature decoding: `TBD`
- Encoder position relative to gearbox: `TBD`

## Robot geometry

- Wheel diameter: `TBD`
- Wheel circumference: `TBD`
- Wheel-to-wheel distance / track width: `TBD`
- Robot width: `TBD`
- Robot length: `TBD`
- Maze cell size: `TBD`

## Control

- Maximum motor PWM: `TBD`
- Target wheel velocity: `TBD`
- PID Kp: `TBD`
- PID Ki: `TBD`
- PID Kd: `TBD`
- Control-loop frequency: `TBD`

Do not hard-code these values until they are confirmed.

---

# 14. CODING CONVENTIONS

Prefer:

- `constexpr` for fixed hardware configuration
- strongly named constants
- small functions
- clear module boundaries
- non-blocking timing where possible
- deterministic control loops
- explicit error handling
- comments explaining hardware-specific behavior

Avoid:

- unexplained magic numbers
- excessive global mutable state
- long blocking `delay()` calls in control loops
- mixing maze algorithms with GPIO code
- silently recovering from hardware errors without reporting them

---

# 15. CHANGE MANAGEMENT

When proposing a hardware or firmware change:

1. State what is being changed.
2. Explain why.
3. Identify affected modules.
4. Identify any GPIO/I2C/power implications.
5. Preserve backward compatibility where practical.
6. Do not modify the authoritative hardware map without explicit confirmation.

For potentially destructive changes, stop and ask before applying them.

---

# 16. COMMUNICATION STYLE FOR THE AGENT

When helping with this project:

- Be technically precise.
- Distinguish confirmed hardware facts from assumptions.
- Refer to the GPIO/address tables in this document rather than inventing pin assignments.
- When debugging, start from the lowest hardware layer and move upward.
- Explain the reason behind a change, not just the code.
- Prefer incremental changes that can be tested independently.
- If a requested feature conflicts with this specification, explicitly point out the conflict.
- If the hardware specification is incomplete, use `TBD` rather than fabricating a value.

---

# 17. QUICK REFERENCE

```text
MCU:
ESP32-C6 DevKitC-1

LOGIC:
3.3 V

BATTERY:
2S Li-ion/LiPo
7.4 V nominal
8.4 V full

ESP32 POWER:
Buck converter → 5.00 V → ESP32 5V input

MOTOR DRIVER:
DRI0044 / TB6612FNG

MOTOR GPIO:
Left  PWM = GPIO18
Left  DIR = GPIO19
Right PWM = GPIO20
Right DIR = GPIO21

ENCODERS:
Left  A/B = GPIO0 / GPIO1
Right A/B = GPIO2 / GPIO3

I2C:
SDA = GPIO6
SCL = GPIO7

VL53L0X XSHUT:
Left  = GPIO10 → 0x30
Front = GPIO11 → 0x31
Right = GPIO5  → 0x32

MPU-6050:
0x68

VL53L0X DEFAULT:
0x29

COMMON GND:
Yes

PRIMARY DESIGN PRINCIPLE:
Hardware → Sensor Processing → Control → State Estimation → Navigation → Motion Planning
```

---

# 18. END OF MASTER SPECIFICATION

This document is the current authoritative reference for the Micromouse hardware and firmware workflow.

When the user provides new confirmed hardware information, update the relevant `TBD` fields and preserve the existing verified GPIO and address assignments unless explicitly changed.

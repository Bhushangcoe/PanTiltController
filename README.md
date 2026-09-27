# PanTiltController

ESP32 firmware for manually controlling a two-axis (pan/tilt) stepper motor mount using a standard RC transmitter, via the FlySky i-BUS protocol.

## What it does

- Reads stick positions from an **FS-iA6B receiver** (i-BUS protocol) connected to the ESP32.
- Converts stick movement (Channel 1 = Pan, Channel 2 = Tilt) into stepper motor speed and direction.
- Drives two stepper motors (via YKD2608MH or similar STEP/DIR drivers) — Pan and Tilt.
- Includes a **failsafe**: if the RC signal is lost for more than 300ms, both motors stop automatically.
- Prints live debug info (channel values, calculated speeds) over USB serial.

## Hardware required

- ESP32 dev board (e.g., ESP32 Dev Module / ESP32-WROOM-32)
- FlySky FS-i6 (or compatible) transmitter
- FlySky FS-iA6B receiver (i-BUS output)
- 2x Stepper motor drivers (e.g., YKD2608MH) + stepper motors
- Wiring per the pin definitions below

## Pin connections

| Function        | ESP32 Pin |
|-----------------|-----------|
| i-BUS RX        | GPIO 16   |
| Pan STEP        | GPIO 25   |
| Pan DIR         | GPIO 26   |
| Tilt STEP       | GPIO 14   |
| Tilt DIR        | GPIO 12   |

Connect the FS-iA6B's i-BUS output pin to GPIO 16 (ESP32 RX). Connect each stepper driver's STEP/DIR inputs to the pins above, and power the drivers/motors according to your driver's datasheet (do **not** power steppers directly from the ESP32).

## Software setup (VS Code + PlatformIO)

1. Install [VS Code](https://code.visualstudio.com/).
2. Install the **PlatformIO IDE** extension from the VS Code Extensions marketplace.
3. Clone this repository:

git clone
 https://github.com/Bhushangcoe/PanTiltController.git

 4. Open the cloned `PanTiltController` folder in VS Code (**File → Open Folder**).
5. PlatformIO will automatically detect `platformio.ini` and download the required ESP32 toolchain and Arduino framework on first open — just wait for it to finish (see the bottom status bar / Output panel).

## Build and upload

1. Connect your ESP32 to your PC via USB.
2. In VS Code, use the **PlatformIO icons in the bottom status bar**:
   - **✓ (checkmark)** — Build the project
   - **→ (right arrow)** — Upload to the ESP32
   - **🔌 (plug icon)** — Open Serial Monitor (to view debug output)

   Or via terminal:
   
pio run # build only
pio run --target upload # build and upload
pio device monitor -b 115200 # view live debug output


## Tuning

Adjust these constants at the top of `src/main.cpp` to match your setup:

- `DEAD_BAND` — stick deadzone around center (default: 40)
- `MAX_STEP_RATE` — top speed in steps/sec (start low for testing, default: 1000)
- `MIN_STEP_RATE` — minimum speed before motion feels sluggish (default: 80)
- `PAN_REVERSE` / `TILT_REVERSE` — flip to `true` if a motor spins the wrong direction

## Safety note

Always start testing with `MAX_STEP_RATE` set low, and ensure the mount can move freely without obstruction before increasing speed.

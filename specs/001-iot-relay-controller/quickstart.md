# Quickstart: Elmahdy Relay IoT Smart Controller

**Feature Branch**: `001-iot-relay-controller`
**Date**: 2026-05-02

---

## Prerequisites

### Hardware
- ESP8266 board (ESP-12F, NodeMCU v2, or Wemos D1 Mini) with 4MB flash
- 1–4 relay modules (active-LOW, default GPIOs: 5, 4, 14, 12)
- Optional: buzzer on GPIO 13, reset button on GPIO 16
- USB cable for initial flash

### Software
- [PlatformIO IDE](https://platformio.org/) (VS Code extension or CLI)
- [Node.js](https://nodejs.org/) (for web asset minification)
- [Tasmotizer](https://github.com/tasmota/tasmotizer) (for distribution flashing)
- Git

---

## Setup

### 1. Clone & Install

```bash
git clone <repo-url>
cd ElmahdyRelay
git checkout 001-iot-relay-controller

# Install PlatformIO (if not already)
pip install platformio

# Install web asset build tools
npm install -g html-minifier csso-cli terser
```

### 2. Build Firmware

```bash
cd firmware

# Build firmware only
pio run -e nodemcuv2

# Build LittleFS filesystem image
pio run -e nodemcuv2 -t buildfs

# Upload firmware via USB
pio run -e nodemcuv2 -t upload

# Upload filesystem via USB
pio run -e nodemcuv2 -t uploadfs
```

### 3. Web Asset Pipeline (before LittleFS build)

```bash
cd firmware/scripts

# Minify + gzip web assets (must run before buildfs)
./minify.sh
```

This minifies HTML/CSS/JS from `data/www/` source files and produces `.gz` versions.

### 4. Full Production Build

```bash
cd firmware/scripts

# Complete pipeline: minify → gzip → buildfs → compile → merge
./build.sh
```

Output: single `firmware.bin` in `firmware/.pio/build/nodemcuv2/` ready for Tasmotizer.

---

## First Boot

1. Flash the firmware via USB or Tasmotizer
2. Device creates AP: `ElmahdyRelay_XXXX` (password: `12345678`)
3. Connect phone to the AP
4. Captive portal opens automatically → setup page in Arabic
5. Scan WiFi networks → select home WiFi → enter password → Save
6. Device connects to home WiFi (AP remains active)
7. Access dashboard at: `http://elmahdyrelay.local` or the assigned IP

---

## Development Workflow

### Serial Monitor
```bash
pio device monitor -b 115200
```

### Quick Iteration (firmware only)
```bash
pio run -e nodemcuv2 -t upload && pio device monitor -b 115200
```

### OTA Update (after first USB flash)
Upload new firmware via web UI: System Settings → Firmware Update → select `.bin` file.

---

## Key Configuration

### `platformio.ini`
```ini
[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino
monitor_speed = 115200
board_build.filesystem = littlefs
board_build.ldscript = eagle.flash.4m2m.ld
build_flags =
    -Os
    -DVERSION=\"1.0.0\"
    -DRELAY_ACTIVE_LOW=true
lib_deps =
    bblanchon/ArduinoJson @ ^7.4
    esphome/ESPAsyncWebServer-esphome @ ^3.3
    marvinroger/AsyncMqttClient @ ^0.9
```

### Pin Defaults (`include/config.h`)
```cpp
#define RELAY_1_PIN  5   // D1
#define RELAY_2_PIN  4   // D2
#define RELAY_3_PIN  14  // D5
#define RELAY_4_PIN  12  // D6
#define BUZZER_PIN   13  // D7
#define RESET_PIN    16  // D0
#define LED_PIN      2   // D4 (built-in)
```

---

## Testing

### Physical Hardware Testing
1. Toggle relays from web UI → verify physical clicks
2. Open two browser tabs → toggle relay → verify both update instantly
3. Configure MQTT → use MQTT Explorer to send/receive commands
4. Set countdown timer → verify relay toggles on expiry
5. Reboot device → verify all config persists
6. Factory reset (hold reset button >10s) → verify defaults restored

### MQTT Testing
```bash
# Subscribe to all topics
mosquitto_sub -h broker.hivemq.com -t "elmahdy/#" -v

# Publish relay command
mosquitto_pub -h broker.hivemq.com -t "elmahdy/relay/1/control" -m "TOGGLE"
```

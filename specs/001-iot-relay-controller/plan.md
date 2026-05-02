# Implementation Plan: Elmahdy Relay IoT Smart Controller

**Branch**: `001-iot-relay-controller` | **Date**: 2026-05-02 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/001-iot-relay-controller/spec.md`

**Note**: This template is filled in by the `/speckit-plan` command. See `.specify/templates/plan-template.md` for the execution workflow.

## Summary

Build a production-grade ESP8266-based IoT smart relay controller (1–4 channels) with a bilingual Arabic/English web dashboard, MQTT integration with Home Assistant auto-discovery, countdown and scheduled timers, scene presets, PWA support, OTA firmware updates, and resilient LittleFS configuration storage. The firmware is written in C++ (Arduino Core), served via ESPAsyncWebServer with real-time WebSocket updates, and distributed as a single `.bin` file for Tasmotizer flashing.

## Technical Context

**Language/Version**: C++17, Arduino Core for ESP8266 v3.x, PlatformIO build system
**Primary Dependencies**:
- ESPAsyncWebServer + ESPAsyncTCP (non-blocking web server)
- ArduinoJson v7 (JSON parsing/serialization)
- AsyncMqttClient (fully async MQTT — pairs with ESPAsyncTCP already in use)
- ESP8266 Arduino Core built-ins: WiFi, mDNS, configTime (NTP), ArduinoOTA
**Storage**: LittleFS with CRC32-validated atomic writes (write-to-tmp → verify → rename)
**Testing**: Physical hardware testing on ESP8266 (ESP-12F/NodeMCU), serial monitor debugging, MQTT Explorer for protocol verification
**Target Platform**: ESP8266 (ESP-12F), 4MB flash, 80KB usable RAM, 2.4 GHz WiFi only
**Project Type**: Embedded firmware + embedded web application (single binary distribution)
**Performance Goals**:
- Boot to functional state: < 5 seconds
- Web UI load: < 2 seconds
- Relay toggle latency: < 100ms
- WebSocket state push: < 50ms
**Constraints**:
- Firmware binary: < 500KB
- LittleFS image: < 512KB (web assets + config + language packs)
- Total active flash: < 1MB
- OTA dual-bank requires ~1MB per slot
- ~80KB usable RAM; no heap-heavy patterns
- Vanilla ES5 JavaScript only (no frameworks)
- Zero external CDN/internet dependencies for local operation
**Scale/Scope**: Single device, 1–4 relay channels, max 8 timers, max 10 scenes, up to 4 concurrent WebSocket clients

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Evidence |
|-----------|--------|----------|
| I. Production-First (NON-NEGOTIABLE) | ✅ PASS | Corruption-safe write pattern (tmp → CRC32 → rename), hardware WDT, dual-bank OTA, boot-glitch-safe default GPIOs (5, 4, 14, 12), firmware < 500KB budget |
| II. Minimal Footprint Architecture | ✅ PASS | ESPAsyncWebServer (non-blocking), vanilla ES5 JS, gzip-compressed web assets, LittleFS, no external CDN, no `delay()` calls, `-Os` optimization |
| III. Dual-Mode WiFi (NON-NEGOTIABLE) | ✅ PASS | AP+STA mode always active, captive portal on first boot, AP never disabled |
| IV. Configurable Channel Architecture | ✅ PASS | 1–4 channels, configurable GPIO/name/power-on-state/pulse/interlock per channel |
| V. Resilient Configuration Storage (NON-NEGOTIABLE) | ✅ PASS | Separate JSON config files (wifi/mqtt/relays/timers/scenes/system), dual-file write with CRC32, per-section factory defaults, backup/restore |
| VI. Full Bilingual Support | ✅ PASS | JSON language packs (lang_ar.json / lang_en.json), RTL/LTR switching, Arabic default, language preference persisted |
| VII. MQTT Integration | ✅ PASS | AsyncMqttClient, configurable broker/port/auth/prefix, HA auto-discovery, LWT, exponential backoff reconnect |

**Gate Result**: ✅ ALL GATES PASS — proceeding to Phase 0.

## Project Structure

### Documentation (this feature)

```text
specs/001-iot-relay-controller/
├── plan.md              # This file (/speckit-plan command output)
├── research.md          # Phase 0 output (/speckit-plan command)
├── data-model.md        # Phase 1 output (/speckit-plan command)
├── quickstart.md        # Phase 1 output (/speckit-plan command)
├── contracts/           # Phase 1 output (/speckit-plan command)
│   ├── rest-api.md      # REST API contract
│   ├── websocket.md     # WebSocket protocol contract
│   └── mqtt.md          # MQTT topic contract
└── tasks.md             # Phase 2 output (/speckit-tasks command - NOT created by /speckit-plan)
```

### Source Code (repository root)

```text
firmware/
├── platformio.ini                # Build configuration, dependencies, partition table
├── include/
│   └── config.h                  # Pin definitions, compile-time constants, version
├── src/
│   ├── main.cpp                  # Entry point: setup() and loop()
│   ├── wifi_manager.h/.cpp       # AP+STA mode, captive portal, WiFi scanning
│   ├── web_server.h/.cpp         # ESPAsyncWebServer routes, static file serving
│   ├── websocket_handler.h/.cpp  # WebSocket event handling, broadcast
│   ├── mqtt_manager.h/.cpp       # AsyncMqttClient, HA discovery, LWT, reconnect
│   ├── relay_controller.h/.cpp   # GPIO control, interlock, pulse mode, state management
│   ├── timer_engine.h/.cpp       # Countdown + scheduled timers, NTP sync, persistence
│   ├── scene_manager.h/.cpp      # Scene CRUD, activation, MQTT integration
│   ├── config_manager.h/.cpp     # LittleFS JSON read/write, CRC32, atomic writes
│   ├── buzzer_controller.h/.cpp  # Beep patterns, anti-spam protection
│   ├── led_controller.h/.cpp     # LED status patterns (fast/slow blink, solid)
│   ├── reset_handler.h/.cpp      # Physical button press duration detection
│   ├── ota_handler.h/.cpp        # HTTP OTA upload, size validation, progress
│   └── language_manager.h/.cpp   # Language pack loading, string lookup
├── data/                         # LittleFS image source (uploaded to flash)
│   ├── www/                      # Web UI assets (minified + gzipped for deployment)
│   │   ├── index.html            # Single-page dashboard + configuration UI
│   │   ├── style.css             # Dark theme, RTL support, mobile-first
│   │   ├── app.js                # Vanilla ES5: WebSocket client, DOM manipulation
│   │   ├── manifest.json         # PWA manifest
│   │   ├── sw.js                 # Service worker for offline caching
│   │   ├── icon-48.png           # PWA icon 48×48
│   │   └── icon-192.png          # PWA icon 192×192
│   ├── lang_ar.json              # Arabic language pack
│   └── lang_en.json              # English language pack
└── scripts/
    ├── minify.sh                 # HTML/CSS/JS minification pipeline
    └── build.sh                  # Full build: minify → gzip → LittleFS → compile → merge
```

**Structure Decision**: Embedded firmware project using a flat `src/` layout with modular `.h/.cpp` pairs per functional domain (12 modules). Web assets live in `data/www/` for LittleFS packaging. This is a single-project structure (no separate frontend/backend) because the web UI is served directly from the ESP8266's flash filesystem — there is no separate build server.

## Complexity Tracking

> No constitution violations detected — table not required.

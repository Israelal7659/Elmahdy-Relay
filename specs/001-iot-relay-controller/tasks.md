# Tasks: Elmahdy Relay IoT Smart Controller

**Input**: Design documents from `/specs/001-iot-relay-controller/`
**Prerequisites**: plan.md ✅, spec.md ✅, research.md ✅, data-model.md ✅, contracts/ ✅, quickstart.md ✅

**Tests**: Not explicitly requested — test tasks omitted.

**Organization**: Tasks grouped by user story (13 stories from spec.md). Each story is independently implementable and testable.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2)
- Exact file paths included in descriptions

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization, PlatformIO config, pin definitions, build scripts

- [X] T001 Create project directory structure per plan.md layout in firmware/
- [X] T002 Create PlatformIO configuration with ESP8266 board, LittleFS, dependencies, and build flags in firmware/platformio.ini
- [X] T003 [P] Create compile-time constants, pin definitions, and version macro in firmware/include/config.h
- [X] T004 [P] Create web asset minification script in firmware/scripts/minify.sh
- [X] T005 [P] Create full production build script in firmware/scripts/build.sh

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story — config storage, web server, WebSocket handler, main entry point

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [X] T006 Implement ConfigManager with LittleFS atomic writes (tmp→CRC32→rename), per-section factory defaults, read/verify/write for all 7 JSON config files in firmware/src/config_manager.h and firmware/src/config_manager.cpp
- [X] T007 Implement RelayController with GPIO init, setState, toggle, interlock enforcement, pulse mode (millis-based), and power-on state restore in firmware/src/relay_controller.h and firmware/src/relay_controller.cpp
- [X] T008 [P] Implement WebSocket handler with JSON message parsing (relay, relayAll, scene, getState), broadcast to all clients (max 4), state/timer/info push events in firmware/src/websocket_handler.h and firmware/src/websocket_handler.cpp
- [X] T009 Implement WebServer with ESPAsyncWebServer setup, static file serving from LittleFS /www/ with gzip support, CORS headers on /api/ routes, and route registration hooks in firmware/src/web_server.h and firmware/src/web_server.cpp
- [X] T010 Create main entry point with setup() initializing all modules in dependency order and loop() calling non-blocking tick functions in firmware/src/main.cpp

**Checkpoint**: Foundation ready — ConfigManager persists data, RelayController toggles GPIOs, WebServer serves files, WebSocket broadcasts state.

---

## Phase 3: User Story 1 — First Boot & WiFi Setup (Priority: P1) 🎯 MVP

**Goal**: Device creates AP on first boot, captive portal redirects to setup page, user scans/selects/connects to home WiFi, AP+STA mode stays active.

**Independent Test**: Power on unconfigured device → connect phone to AP → complete WiFi setup → verify device joins home WiFi while AP remains accessible.

### Implementation for User Story 1

- [X] T011 [US1] Implement WiFiManager with AP+STA mode, unique SSID (ElmahdyRelay_XXXX from MAC), captive portal via DNSServer, WiFi scanning, credential save, auto-reconnect with 3 retries, and STA connection status tracking in firmware/src/wifi_manager.h and firmware/src/wifi_manager.cpp
- [X] T012 [US1] Add REST endpoints: GET /api/wifi/scan (network list with RSSI), GET /api/config/wifi, POST /api/config/wifi (save credentials, trigger connect) in firmware/src/web_server.cpp
- [X] T013 [US1] Create base HTML single-page structure with tab navigation (Dashboard/WiFi/MQTT/Relays/Timers/Scenes/System/About), WiFi setup tab with scan button, network list, password field, and save button in firmware/data/www/index.html
- [X] T014 [US1] Create dark theme CSS with mobile-first layout, RTL support via [dir="rtl"] selectors, tab navigation styles, relay card styles, form styles, and responsive breakpoints in firmware/data/www/style.css
- [X] T015 [US1] Create vanilla ES5 JavaScript with WebSocket client, tab switching, WiFi scan/save logic, DOM manipulation helpers, and language string injection in firmware/data/www/app.js

**Checkpoint**: User Story 1 complete — device boots to AP, captive portal works, WiFi setup functional.

---

## Phase 4: User Story 2 — Dashboard Relay Control (Priority: P1)

**Goal**: Dashboard shows relay cards with toggles, instant WebSocket state sync, All ON/OFF buttons, system info display (RSSI, uptime, MQTT status, version).

**Independent Test**: Open dashboard → toggle relays → verify physical clicks and real-time state updates across multiple browser clients.

### Implementation for User Story 2

- [X] T016 [US2] Add REST endpoints: GET /api/status (relay states + system info), POST /api/relay/{ch} (on/off/toggle), POST /api/relay/all in firmware/src/web_server.cpp
- [X] T017 [US2] Add dashboard tab UI: relay cards with toggle switches, channel names, All ON/All OFF buttons, system info bar (RSSI bars, uptime, MQTT status, firmware version) in firmware/data/www/index.html
- [X] T018 [US2] Add dashboard JavaScript: WebSocket relay command sending, state update handling, system info periodic display, toggle switch event handlers in firmware/data/www/app.js

**Checkpoint**: User Story 2 complete — relays toggle from dashboard, state syncs across clients in real-time.

---

## Phase 5: User Story 3 — Relay & GPIO Configuration (Priority: P1)

**Goal**: Configure 1–4 channels with custom GPIO, names (Arabic/English), power-on state; boot-sensitive GPIO warnings; corruption-safe persistence.

**Independent Test**: Configure channels with custom names/GPIOs → reboot → verify all settings persist and relays respond on correct GPIOs.

### Implementation for User Story 3

- [X] T019 [US3] Add REST endpoints: GET /api/config/relays, POST /api/config/relays (channel count, per-channel GPIO/name/powerOnState/pulseDuration/interlockGroup) with boot-sensitive GPIO validation in firmware/src/web_server.cpp
- [X] T020 [US3] Add relay settings tab UI: channel count selector (1–4), per-channel forms (GPIO dropdown, name input max 20 chars, power-on state select, pulse duration, interlock group), GPIO warning modal for pins 0/2/15 in firmware/data/www/index.html
- [X] T021 [US3] Add relay settings JavaScript: dynamic form generation based on channel count, GPIO conflict validation, save with confirmation for dangerous GPIOs in firmware/data/www/app.js

**Checkpoint**: User Story 3 complete — relay channels fully configurable, settings persist across power cycles.

---

## Phase 6: User Story 8 — Bilingual Language Switch (Priority: P1)

**Goal**: AR/EN toggle in header, instant UI switch with RTL/LTR, language packs loaded from JSON, preference persisted.

**Independent Test**: Switch language AR↔EN → verify all text changes, layout direction flips, preference persists across reboots.

### Implementation for User Story 8

- [X] T022 [P] [US8] Create Arabic language pack with all UI strings (labels, buttons, messages, errors, placeholders) in firmware/data/lang_ar.json
- [X] T023 [P] [US8] Create English language pack with all UI strings matching Arabic pack keys in firmware/data/lang_en.json
- [X] T024 [US8] Implement LanguageManager with language pack loading from LittleFS, string lookup by key, and language preference read from SystemConfig in firmware/src/language_manager.h and firmware/src/language_manager.cpp
- [X] T025 [US8] Add REST endpoint: GET /api/lang/{code} serving language pack JSON in firmware/src/web_server.cpp
- [X] T026 [US8] Add language toggle button in UI header, JavaScript for fetching language pack, DOM text replacement, dir attribute switching (rtl/ltr), and language preference save via POST /api/config/system in firmware/data/www/app.js

**Checkpoint**: User Story 8 complete — full bilingual AR/EN with instant switching, RTL/LTR, persisted preference.

---

## Phase 7: User Story 4 — MQTT Remote Control (Priority: P2)

**Goal**: MQTT connect with configurable broker/auth/prefix, relay control/status topics, LWT, HA auto-discovery, exponential backoff reconnect.

**Independent Test**: Configure MQTT → subscribe to status topics → publish control commands → verify relay toggles and HA auto-discovers the device.

### Implementation for User Story 4

- [X] T027 [US4] Implement MqttManager with AsyncMqttClient: connect/disconnect, subscribe to control topics, publish state (retained), LWT online/offline, exponential backoff reconnect (1s→30s max), system info publish in firmware/src/mqtt_manager.h and firmware/src/mqtt_manager.cpp
- [X] T028 [US4] Implement Home Assistant auto-discovery: publish retained JSON to homeassistant/switch/{device_id}/relay_{ch}/config on every MQTT connect per mqtt.md contract in firmware/src/mqtt_manager.cpp
- [X] T029 [US4] Add REST endpoints: GET /api/config/mqtt, POST /api/config/mqtt (broker, port, username, password, prefix, enabled) in firmware/src/web_server.cpp
- [X] T030 [US4] Add MQTT settings tab UI: broker, port, username, password, prefix fields, enable/disable toggle, connection status indicator in firmware/data/www/index.html
- [X] T031 [US4] Add MQTT settings JavaScript: form load/save, connection status display from WebSocket info events in firmware/data/www/app.js

**Checkpoint**: User Story 4 complete — MQTT control works, HA discovers device, local functionality unaffected when MQTT disabled.

---

## Phase 8: User Story 5 — Countdown Timer (Priority: P2)

**Goal**: Set countdown on any channel, dashboard shows remaining time, relay toggles at zero with buzzer, cancel support, persistence across power cycles. Max 8 timers.

**Independent Test**: Set 2-minute countdown → verify dashboard countdown → verify relay toggles at expiry → reboot mid-countdown → verify resume.

### Implementation for User Story 5

- [X] T032 [US5] Implement TimerEngine: countdown timer create/cancel/tick, millis-based elapsed tracking, persistence via ConfigManager (startedAt + duration), resume on boot with adjusted remaining time, max 8 limit enforcement in firmware/src/timer_engine.h and firmware/src/timer_engine.cpp
- [X] T033 [US5] Add REST endpoints: GET /api/timers, POST /api/timer (create countdown), DELETE /api/timer/{id} per rest-api.md contract in firmware/src/web_server.cpp
- [X] T034 [US5] Add WebSocket timer update broadcast (remaining time every 1 second for active countdowns) in firmware/src/websocket_handler.cpp
- [X] T035 [US5] Add timer tab UI: countdown creation form (channel select, duration input, target state), active timer list with remaining time and cancel button in firmware/data/www/index.html
- [X] T036 [US5] Add timer JavaScript: countdown form handling, WebSocket timer update display, cancel button handler in firmware/data/www/app.js

**Checkpoint**: User Story 5 complete — countdown timers functional, persist across reboots, display on dashboard.

---

## Phase 9: User Story 6 — Scheduled Timer with NTP (Priority: P2)

**Goal**: NTP time sync, scheduled timers with repeat modes (once/daily/weekdays/weekend/custom), pause when NTP unavailable, persistence.

**Independent Test**: Create schedule 2 minutes ahead → verify relay toggles at time → change timezone → verify adjustment.

### Implementation for User Story 6

- [X] T037 [US6] Add NTP sync using configTime() on WiFi connect, timezone offset from SystemConfig, NTP status tracking (synced/unsynced) in firmware/src/timer_engine.cpp
- [X] T038 [US6] Extend TimerEngine with scheduled timer support: hour/minute/repeatMode/dayMask, next-trigger calculation, pause when NTP unavailable, daily/weekdays/weekend/custom day matching in firmware/src/timer_engine.cpp
- [X] T039 [US6] Add REST endpoint: POST /api/timer (create scheduled type with hour, minute, repeatMode, dayMask) in firmware/src/web_server.cpp — timer REST endpoint handles both types
- [X] T040 [US6] Add scheduled timer UI: time picker (hour/minute), repeat mode selector, custom day checkboxes, timezone display in firmware/data/www/index.html
- [X] T041 [US6] Add scheduled timer JavaScript: form handling for scheduled type, repeat mode toggling, day mask UI in firmware/data/www/app.js

**Checkpoint**: User Story 6 complete — scheduled timers trigger at correct times, respect NTP availability and timezone.

---

## Phase 10: User Story 12 — Interlock & Pulse Mode (Priority: P2)

**Goal**: Interlock groups (only one relay ON in a group), pulse/inching mode per channel with configurable duration.

**Independent Test**: Configure interlock group → turn on relay A then B → verify A turns off. Set pulse mode → toggle → verify auto-OFF after duration.

### Implementation for User Story 12

- [X] T042 [US12] Extend RelayController interlock enforcement: on setState, turn OFF all other relays in same interlockGroup before turning ON the requested relay; enforce across all command sources in firmware/src/relay_controller.cpp
- [X] T043 [US12] Extend RelayController pulse mode: on turning ON a channel with pulseDuration > 0, set pulseEndTime = millis() + pulseDuration; in tick(), auto-OFF when millis() >= pulseEndTime in firmware/src/relay_controller.cpp
- [X] T044 [US12] Add interlock group and pulse duration fields to relay settings tab UI and JavaScript in firmware/data/www/index.html and firmware/data/www/app.js

**Checkpoint**: User Story 12 complete — interlock prevents conflicting relays, pulse mode auto-turns-off after duration.

---

## Phase 11: User Story 7 — Scene Presets (Priority: P3)

**Goal**: Named scenes grouping channel states, triggerable from dashboard/MQTT/PWA, Arabic/English names, max 10 scenes, persistence.

**Independent Test**: Create scene with specific states → trigger from dashboard and MQTT → verify all channels switch correctly.

### Implementation for User Story 7

- [X] T045 [US7] Implement SceneManager: CRUD operations, case-insensitive name uniqueness, activate (set all channels to scene states via RelayController), max 10 limit, persistence via ConfigManager in firmware/src/scene_manager.h and firmware/src/scene_manager.cpp
- [X] T046 [US7] Add REST endpoints: GET /api/scenes, POST /api/scene, POST /api/scene/{name}/activate, DELETE /api/scene/{name} per rest-api.md contract in firmware/src/web_server.cpp
- [X] T047 [US7] Add MQTT scene subscription: subscribe to {prefix}/scene/{name}/control, activate scene on "ON" payload in firmware/src/mqtt_manager.cpp
- [X] T048 [US7] Add WebSocket scene command handling: parse {type:"scene", name:"..."} and activate via SceneManager in firmware/src/websocket_handler.cpp
- [X] T049 [US7] Add scene tab UI: create form (name input, channel state toggles), scene list with activate/delete buttons; add scene quick-trigger buttons on dashboard tab in firmware/data/www/index.html
- [X] T050 [US7] Add scene JavaScript: CRUD form handling, activate button handler, dashboard scene buttons in firmware/data/www/app.js

**Checkpoint**: User Story 7 complete — scenes triggerable from all interfaces, persist across reboots.

---

## Phase 12: User Story 9 — Buzzer, LED & Reset Button (Priority: P3)

**Goal**: Configurable buzzer/LED/reset button; buzzer beep patterns with anti-spam; LED status patterns; reset button press duration detection.

**Independent Test**: Press reset at various durations → verify actions. Toggle buzzer/LED settings → verify behavior changes.

### Implementation for User Story 9

- [X] T051 [P] [US9] Implement BuzzerController with beep patterns (short=relay, double=WiFi, long=reset), anti-spam 100ms minimum, enable/disable from SystemConfig in firmware/src/buzzer_controller.h and firmware/src/buzzer_controller.cpp
- [X] T052 [P] [US9] Implement LEDController with status patterns: fast blink=AP only, slow blink=WiFi connected, solid=MQTT connected, enable/disable from SystemConfig in firmware/src/led_controller.h and firmware/src/led_controller.cpp
- [X] T053 [US9] Implement ResetHandler with GPIO input polling, press duration detection: short (<3s)=toggle relay 1, long (3-10s)=reboot, very long (>10s)=factory reset with buzzer feedback in firmware/src/reset_handler.h and firmware/src/reset_handler.cpp
- [X] T054 [US9] Add system settings tab UI: buzzer enable/disable, LED enable/disable, reset button enable/disable, buzzer pin, reset pin fields in firmware/data/www/index.html
- [X] T055 [US9] Add REST endpoints: GET /api/config/system, POST /api/config/system (buzzerEnabled, ledEnabled, resetEnabled, buzzerPin, resetPin, hostname, timezoneOffset) in firmware/src/web_server.cpp
- [X] T056 [US9] Integrate BuzzerController calls into RelayController (toggle beep), WiFiManager (connect beep), and ResetHandler (reset beep) in firmware/src/main.cpp

**Checkpoint**: User Story 9 complete — hardware feedback works, gracefully degrades when hardware not connected.

---

## Phase 13: User Story 10 — OTA Firmware Update (Priority: P3)

**Goal**: Web UI firmware upload, size validation, progress bar, auto-reboot, dual-bank protection, config preserved.

**Independent Test**: Upload new .bin → verify progress bar → verify reboot with new version → verify config intact.

### Implementation for User Story 10

- [X] T057 [US10] Implement OtaHandler with ESPAsyncWebServer onUpload handler: stream to Update class, size validation against getFreeSketchSpace(), progress tracking, auto-reboot on success — implemented inline in web_server.cpp
- [X] T058 [US10] Add REST endpoint: POST /api/system/update (multipart .bin upload) with progress WebSocket broadcast in firmware/src/web_server.cpp
- [X] T059 [US10] Add OTA UI in system settings tab: file picker, upload button, progress bar, version display in firmware/data/www/index.html
- [X] T060 [US10] Add OTA JavaScript: file upload with XMLHttpRequest progress event, progress bar update, success/error feedback in firmware/data/www/app.js

**Checkpoint**: User Story 10 complete — OTA update works with progress feedback, dual-bank protects against failures.

---

## Phase 14: User Story 11 — PWA Mobile Installation (Priority: P3)

**Goal**: PWA manifest, service worker caching static assets, Add to Home Screen, standalone mode.

**Independent Test**: Open on Android Chrome → verify A2HS prompt → install → launch standalone.

### Implementation for User Story 11

- [X] T061 [P] [US11] Create PWA manifest with app name "Elmahdy Relay", theme/background color #1a1a2e, display standalone, start_url /, icons array in firmware/data/www/manifest.json
- [X] T062 [P] [US11] Create service worker with install event pre-caching app shell (index.html, style.css, app.js, manifest.json, icons), fetch event cache-first for static assets, network-first for /api/ in firmware/data/www/sw.js
- [X] T063 [P] [US11] Generate PWA icons: 48x48 and 192x192 PNG files in firmware/data/www/icon-48.png and firmware/data/www/icon-192.png
- [X] T064 [US11] Add manifest link and service worker registration script to index.html, add meta theme-color tag in firmware/data/www/index.html

**Checkpoint**: User Story 11 complete — PWA installable on mobile, works in standalone mode on local network.

---

## Phase 15: User Story 13 — Backup & Restore (Priority: P3)

**Goal**: Download all config as single file, upload to restore, device reboots after restore.

**Independent Test**: Configure device → download backup → factory reset → restore → verify all settings restored.

### Implementation for User Story 13

- [X] T065 [US13] Add REST endpoints: GET /api/backup (combine all config JSONs into single download with Content-Disposition header), POST /api/restore (parse uploaded JSON, write each section via ConfigManager, reboot) in firmware/src/web_server.cpp
- [X] T066 [US13] Add backup/restore UI in system settings tab: Backup button triggering download, Restore file picker with upload button in firmware/data/www/index.html
- [X] T067 [US13] Add backup/restore JavaScript: backup download trigger, restore file upload and confirmation dialog in firmware/data/www/app.js

**Checkpoint**: User Story 13 complete — full config backup/restore cycle works.

---

## Phase 16: Polish & Cross-Cutting Concerns

**Purpose**: Final integration, optimization, and production readiness

- [X] T068 Add About tab with product branding: "Elmahdy Relay", © Sayed Elmahdy, contact 01093307397, firmware version, device MAC in firmware/data/www/index.html
- [X] T069 Add POST /api/system/reboot and POST /api/system/reset (factory reset) REST endpoints in firmware/src/web_server.cpp
- [X] T070 Add mDNS discovery registration with configurable hostname from SystemConfig in firmware/src/wifi_manager.cpp
- [X] T071 [P] Add WebSocket periodic system info broadcast (RSSI, uptime, MQTT status, heap) every 5 seconds per websocket.md contract in firmware/src/websocket_handler.cpp
- [X] T072 [P] Add configChanged WebSocket event broadcast when any config section is updated in firmware/src/web_server.cpp
- [X] T073 Verify all web assets minify and gzip correctly via firmware/scripts/minify.sh; confirm total LittleFS image < 512KB
- [X] T074 Verify firmware binary size < 500KB with pio run -e nodemcuv2
- [X] T075 Run full quickstart.md validation: first boot → WiFi setup → relay control → MQTT → timers → scenes → OTA → backup/restore
- [X] T076 Review and finalize all error messages in both language packs (lang_ar.json and lang_en.json) for completeness

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion — BLOCKS all user stories
- **US1 WiFi (Phase 3)**: Depends on Phase 2 — first MVP milestone
- **US2 Dashboard (Phase 4)**: Depends on Phase 3 (needs web UI foundation from US1)
- **US3 Relay Config (Phase 5)**: Depends on Phase 4 (needs dashboard for relay cards)
- **US8 Language (Phase 6)**: Depends on Phase 3 (needs web UI to apply language strings)
- **US4 MQTT (Phase 7)**: Depends on Phase 2 only — can parallel with US1-US3 but recommended after US2
- **US5 Countdown (Phase 8)**: Depends on Phase 2 — can parallel with US4
- **US6 Scheduled (Phase 9)**: Depends on Phase 8 (extends TimerEngine from US5)
- **US12 Interlock/Pulse (Phase 10)**: Depends on Phase 2 — can parallel with US4-US6
- **US7 Scenes (Phase 11)**: Depends on Phase 2; optional MQTT integration needs US4
- **US9 Buzzer/LED/Reset (Phase 12)**: Depends on Phase 2 — can parallel with any story
- **US10 OTA (Phase 13)**: Depends on Phase 2 — can parallel with any story
- **US11 PWA (Phase 14)**: Depends on Phase 3 (needs web UI files to cache)
- **US13 Backup/Restore (Phase 15)**: Depends on Phase 2 — can parallel with any story
- **Polish (Phase 16)**: Depends on all user stories being complete

### User Story Dependencies

```
Phase 1 (Setup)
    └─► Phase 2 (Foundational)
            ├─► Phase 3 (US1: WiFi) ──► Phase 4 (US2: Dashboard) ──► Phase 5 (US3: Relay Config)
            ├─► Phase 6 (US8: Language) — after Phase 3
            ├─► Phase 7 (US4: MQTT) — independent
            ├─► Phase 8 (US5: Countdown) ──► Phase 9 (US6: Scheduled)
            ├─► Phase 10 (US12: Interlock/Pulse) — independent
            ├─► Phase 11 (US7: Scenes) — independent, optional MQTT tie-in
            ├─► Phase 12 (US9: Buzzer/LED/Reset) — independent
            ├─► Phase 13 (US10: OTA) — independent
            ├─► Phase 14 (US11: PWA) — after Phase 3
            └─► Phase 15 (US13: Backup/Restore) — independent
                    └─► Phase 16 (Polish)
```

### Within Each User Story

- Firmware modules before REST endpoints
- REST endpoints before UI
- UI HTML before JavaScript
- Core implementation before integration

### Parallel Opportunities

- T003, T004, T005 (Setup) — all parallel
- T008 (WebSocket) parallel with T006, T007
- T022, T023 (language packs) — parallel
- T051, T052 (Buzzer, LED) — parallel
- T061, T062, T063 (PWA assets) — parallel
- After Phase 2: US4, US5, US7, US9, US10, US12, US13 can all run in parallel

---

## Parallel Example: After Foundational Phase

```
# These story phases can run in parallel (different modules/files):
Stream A: US1 (WiFi) → US2 (Dashboard) → US3 (Relay Config) → US8 (Language)
Stream B: US4 (MQTT) → US7 (Scenes MQTT integration)
Stream C: US5 (Countdown) → US6 (Scheduled)
Stream D: US9 (Buzzer/LED/Reset) + US10 (OTA) + US11 (PWA) + US12 (Interlock) + US13 (Backup)
```

---

## Implementation Strategy

### MVP First (User Stories 1–3 + 8)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (CRITICAL — blocks all stories)
3. Complete Phase 3: US1 — WiFi Setup
4. Complete Phase 4: US2 — Dashboard Relay Control
5. Complete Phase 5: US3 — Relay Configuration
6. Complete Phase 6: US8 — Bilingual Language Switch
7. **STOP and VALIDATE**: Test full local control flow independently

### Incremental Delivery

1. Setup + Foundational → Foundation ready
2. US1 + US2 + US3 + US8 → **MVP: Local bilingual relay control** ✅
3. US4 → MQTT remote control + HA discovery ✅
4. US5 + US6 → Timer automation ✅
5. US12 → Safety features (interlock/pulse) ✅
6. US7 → Scene presets ✅
7. US9 + US10 + US11 + US13 → Production polish ✅
8. Polish → Final validation ✅

---

## Notes

- [P] tasks = different files, no dependencies on incomplete tasks
- [Story] label maps task to specific user story for traceability
- Each user story is independently completable and testable after its dependencies
- Commit after each task or logical group
- Stop at any checkpoint to validate story independently
- All web UI changes touch shared files (index.html, style.css, app.js) — these tasks are NOT parallel within a story
- Firmware modules (.h/.cpp pairs) are independent files — parallel where marked

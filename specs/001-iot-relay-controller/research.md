# Research: Elmahdy Relay IoT Smart Controller

**Feature Branch**: `001-iot-relay-controller`
**Date**: 2026-05-02
**Status**: Complete — all NEEDS CLARIFICATION items resolved

---

## R-001: MQTT Client Library Selection

**Decision**: AsyncMqttClient (by Marvin Roger / maintained forks)

**Rationale**:
- The project already uses ESPAsyncWebServer + ESPAsyncTCP, making AsyncMqttClient a natural fit since it builds on the same async TCP layer — zero additional TCP stack overhead.
- PubSubClient is synchronous and **blocks** `loop()` during connect/publish/subscribe operations. On the ESP8266, where the main loop must remain responsive for WebSocket updates (< 50ms latency requirement), any blocking during MQTT reconnection would violate performance requirements.
- AsyncMqttClient operates via callbacks and never blocks, maintaining relay toggle responsiveness even during MQTT broker outages.
- Memory overhead is higher than PubSubClient (~2–3KB additional heap), but acceptable given the project has ~80KB usable RAM and MQTT is a single-instance module.

**Alternatives Considered**:
- **PubSubClient**: Lower memory, simpler API, but blocks during network operations — violates FR-006 (100ms relay response) during MQTT reconnection attempts that can take 5–15 seconds.
- **espMqttClient**: Newer async alternative but less battle-tested on ESP8266 than AsyncMqttClient.

---

## R-002: JSON Configuration Storage Pattern

**Decision**: Separate JSON files per functional area with CRC32 validation and atomic write pattern.

**Rationale**:
- Constitution mandates separate config files: `wifi.json`, `mqtt.json`, `relays.json`, `timers.json`, `scenes.json`, `system.json`.
- Atomic write pattern: write to `.tmp` → calculate CRC32 → append CRC to file → rename `.tmp` to `.json`. The active config file is never written directly.
- On read: parse JSON, verify CRC32 against recalculated value. If mismatch, load compiled-in `constexpr` factory defaults for THAT section only.
- LittleFS `rename()` is atomic at the filesystem level — if power is lost during rename, either the old or new file survives intact.
- ArduinoJson v7's `JsonDocument` (auto-sizing) is used for parsing. Documents are scoped to functions to release heap immediately after use.

**Alternatives Considered**:
- **Single monolithic config.json**: Simpler but violates constitution (Principle V) and risks wiping all settings if one section corrupts.
- **Binary struct storage with EEPROM emulation**: Smaller files but inflexible for variable-length data (Arabic channel names, scene names) and harder to debug.
- **NVS (Non-Volatile Storage)**: Not available on ESP8266 (ESP32 only).

---

## R-003: ArduinoJson v7 Memory Management

**Decision**: Use `JsonDocument` (v7 auto-sizing) with function-scoped allocation; parse directly from `File` streams.

**Rationale**:
- ArduinoJson v7 replaced `StaticJsonDocument`/`DynamicJsonDocument` with a single `JsonDocument` class that manages memory dynamically.
- To minimize heap fragmentation on ESP8266, all JSON documents are created as local variables within functions (destroyed when function returns).
- For large config reads (timers, scenes), use `deserializeJson(doc, file)` to stream directly from the LittleFS file handle, avoiding double-buffering.
- For JSON responses, use `serializeJson(doc, response)` to stream directly to the AsyncWebServer response, avoiding intermediate `String` allocations.
- Maximum expected JSON document size: ~2KB (scenes with 10 entries × 4 channels) — well within heap limits.

**Alternatives Considered**:
- **Manual JSON string construction**: Error-prone, no validation, harder to maintain.
- **ArduinoJson v6**: Deprecated; v7 is the current supported version with better memory management.

---

## R-004: Web UI Architecture & Asset Optimization

**Decision**: Single-page HTML with vanilla ES5 JavaScript, dark theme CSS, gzip-compressed, served from LittleFS via ESPAsyncWebServer.

**Rationale**:
- Constitution mandates: "Vanilla HTML5 + CSS + ES5 JavaScript only — NO frameworks."
- ESPAsyncWebServer natively handles `Content-Encoding: gzip` when serving `.gz` files — the server checks for `filename.gz` automatically.
- Build pipeline: `html-minifier` → `csso` → `terser` → `gzip` — produces assets typically 60–80% smaller than uncompressed.
- Single-page design with tab-based navigation (Dashboard / WiFi / MQTT / Relays / Timers / Scenes / System / About) avoids multiple HTTP requests and keeps the SPA feel.
- WebSocket client in `app.js` maintains a persistent connection for real-time state sync.
- Language strings loaded dynamically from `/api/lang/{code}` endpoint; DOM text nodes updated in-place on language switch.
- RTL support via `[dir="rtl"]` CSS selectors and `document.dir = "rtl"` in JavaScript.

**Alternatives Considered**:
- **Multiple HTML pages**: More HTTP requests, slower navigation, harder to maintain WebSocket connection across pages.
- **Server-side template rendering**: Too memory-intensive on ESP8266; would require generating HTML in RAM.
- **Preact/lit-html**: Smaller frameworks but still add binary weight and violate constitution's "no frameworks" rule.

---

## R-005: OTA Update Strategy

**Decision**: HTTP-based OTA via ESPAsyncWebServer file upload with dual-bank protection using ESP8266's built-in `eboot` bootloader.

**Rationale**:
- ESP8266 4MB flash layout: ~1MB Sketch Slot 1 + ~1MB OTA Buffer (Slot 2) + ~2MB LittleFS.
- The `Update` library (built into ESP8266 Arduino Core) handles writing to the inactive OTA slot, verification, and bootloader flag setting.
- ESPAsyncWebServer's `onUpload` handler streams the binary directly to the `Update` class — no need to buffer the entire file in RAM.
- On success, `ESP.restart()` triggers the bootloader to switch to the new firmware.
- On failure (power loss, corrupt upload), the bootloader reverts to the previous working firmware — the device is never bricked.
- Size validation: check `contentLength` against `ESP.getFreeSketchSpace()` before starting the flash.
- LittleFS partition is untouched during OTA — all user configuration persists.

**Alternatives Considered**:
- **ArduinoOTA (UDP-based)**: Designed for development, not user-facing; requires IDE or `espota.py`.
- **Cloud OTA (HTTP pull)**: Requires internet access; constitution mandates offline operation.

---

## R-006: Home Assistant MQTT Auto-Discovery

**Decision**: Publish retained JSON discovery payloads to `homeassistant/switch/{device_id}/relay_{ch}/config` on every MQTT connect.

**Rationale**:
- Home Assistant monitors `homeassistant/+/+/+/config` topics for auto-discovery.
- Each relay channel publishes a discovery payload with: `name`, `unique_id`, `command_topic`, `state_topic`, `availability_topic`, `device` object (groups all channels under one device card).
- Payloads MUST be published with `retain: true` so HA discovers the device even if it restarts after the ESP8266.
- `unique_id` format: `elmahdy_relay_{mac}_{ch}` — globally unique, stable across reboots.
- `device` block includes: `identifiers: [mac]`, `name: "Elmahdy Relay"`, `model: "ESP8266-4CH"`, `manufacturer: "Elmahdy"`.
- On MQTT disconnect, LWT publishes "offline" to `{prefix}/system/status`; HA marks entity as unavailable.

**Alternatives Considered**:
- **No auto-discovery (manual HA YAML config)**: Worse user experience; HA users expect zero-config discovery.
- **ESPHome-style discovery**: Proprietary format; standard HA MQTT discovery is more universal.

---

## R-007: PWA Implementation on ESP8266

**Decision**: Serve `manifest.json` and `sw.js` from LittleFS; use cache-first strategy for static assets.

**Rationale**:
- PWA requires: (1) valid `manifest.json` linked in HTML, (2) service worker registered from a secure context, (3) icons at required sizes.
- ESP8266 serves HTTP (not HTTPS), but modern browsers treat local network IPs (`192.168.x.x`) and `localhost` as secure contexts for service workers.
- Service worker caches: `index.html`, `style.css`, `app.js`, `manifest.json`, icons. API calls are NOT cached (they must always be live).
- Cache strategy: Install event pre-caches the app shell; Fetch event uses cache-first for static assets, network-first for API endpoints.
- `manifest.json`: `display: "standalone"`, `theme_color: "#1a1a2e"`, `background_color: "#1a1a2e"`, `start_url: "/"`.
- Icons: 48×48 and 192×192 PNG (< 10KB total).

**Alternatives Considered**:
- **No PWA**: Would still work in browser but loses "Add to Home Screen" and standalone mode — diminished user experience.
- **Full offline-first PWA**: Overly complex; device must be on the same network anyway, so true offline (no WiFi) adds no value.

---

## R-008: Timer Persistence Across Power Cycles

**Decision**: Store timer state including `startedAt` epoch and `durationMs` in `timers.json`; on boot, recalculate remaining time from RTC or NTP.

**Rationale**:
- Countdown timers use elapsed-time tracking. On save: store `startedAt` (millis or epoch when timer started), `durationMs`, and `targetState`.
- On reboot: if NTP is available, calculate `elapsed = now - startedAt`; remaining = `durationMs - elapsed`. If remaining ≤ 0, execute the action immediately.
- If NTP is NOT available, use `millis()` delta from last known checkpoint. Constitution mandates: countdown timers continue without NTP; scheduled timers pause.
- Scheduled timers store: `hour`, `minute`, `repeatMode`, `dayMask`, `targetState`, `enabled`. On boot, the timer engine checks current NTP time and calculates next trigger.
- Timer state is written to `timers.json` on creation/cancellation/completion only (not every tick) to minimize flash writes.

**Alternatives Considered**:
- **RTC module (DS3231)**: Adds hardware cost and complexity; NTP + millis() fallback is sufficient.
- **Store only "remaining seconds"**: Loses accuracy across reboots if boot takes variable time.

---

## R-009: Interlock & Pulse Mode Implementation

**Decision**: Interlock enforced at the RelayController level; pulse mode uses millis()-based non-blocking timers.

**Rationale**:
- Interlock groups stored in `relays.json` as `interlockGroup` field per channel (0 = no group, 1–4 = group ID).
- When any relay in a group is turned ON, the RelayController first turns OFF all other relays in the same group, then turns ON the requested relay. This sequence is atomic from the API perspective.
- Interlock is enforced regardless of command source (web UI, MQTT, physical button, scene, timer).
- Pulse mode: `pulseDuration` field per channel (0 = disabled, >0 = milliseconds). When a relay is turned ON with pulse mode, a `millis()`-based timer automatically turns it OFF after the duration.
- Pulse timers are NOT persisted to flash (they are transient — if the device reboots, the pulse is lost, which is the expected behavior for momentary actions like gate openers).

**Alternatives Considered**:
- **Interlock at API layer**: Would leave gaps — physical button or MQTT could bypass. Must be at the controller level.
- **Blocking `delay()` for pulse**: Violates constitution (Principle II).

---

## R-010: Captive Portal Implementation

**Decision**: Use `DNSServer` to redirect all DNS queries to `192.168.4.1` when in AP-only mode or first boot.

**Rationale**:
- ESP8266 `DNSServer` library captures all DNS requests from clients connected to the AP and responds with the device's IP (`192.168.4.1`).
- This triggers the captive portal detection on iOS/Android, automatically opening a browser to the setup page.
- Captive portal is active only when: (a) no WiFi credentials are stored (first boot), or (b) STA connection has failed.
- Once STA is connected, the DNS server stops redirecting (allows normal internet access through the router).
- The portal serves the same web UI as normal operation — no separate "setup-only" page needed.

**Alternatives Considered**:
- **No captive portal (manual IP entry)**: Poor UX; users wouldn't know to navigate to 192.168.4.1.
- **Separate captive portal HTML**: Unnecessary duplication; the single-page UI handles both setup and operation.

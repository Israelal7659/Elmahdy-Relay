/*
 * main.cpp — Firmware entry point: setup() and loop()
 *
 * Target : ESP8266 (ESP-12F / NodeMCU), Arduino Core 3.x, C++17
 * Build  : PlatformIO (espressif8266)
 *
 * Initialization order (dependency-driven):
 *   1. Serial          — debug output before anything else
 *   2. ConfigManager   — mounts LittleFS; every other module reads config
 *   3. RelayController — GPIO init must happen before network comes up to
 *                        prevent relay state depending on Wi-Fi boot order
 *   4. WebServer       — starts HTTP server; registers WS endpoint via
 *                        WebSocketHandler internally
 *
 *   Modules WiFiManager, MqttManager, TimerEngine, SceneManager,
 *   BuzzerController, LEDController, ResetHandler, OtaHandler, and
 *   LanguageManager are scaffolded below with TODO markers and will be
 *   wired in during their respective implementation tasks.
 *
 * loop() discipline:
 *   - No delay() calls — the system is fully non-blocking.
 *   - Each module exposes a tick() that returns quickly.
 *   - ArduinoOTA and ESPAsyncWebServer handle their own ISR/callback
 *     scheduling; we only need to call yield() implicitly via the Arduino
 *     core's main loop wrapper.
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>     // WiFi.macAddress()

#include "../include/config.h"
#include "config_manager.h"
#include "relay_controller.h"
#include "websocket_handler.h"
#include "web_server.h"
#include "timer_engine.h"    // T032/T033/T034
#include "mqtt_manager.h"   // T027

#include "scene_manager.h"

// ─────────────────────────────────────────────────────────────────────────────
// Global module instances — static storage, never heap-allocated.
//
// Destruction order: reverse of construction (C++ standard §3.6.3).
// On ESP8266 the firmware never reaches global destructors during normal
// operation, but the ordering is documented here for correctness.
// ─────────────────────────────────────────────────────────────────────────────

// Core infrastructure (no dependencies on other modules)
ConfigManager    configManager;

// Relay GPIO layer — depends on ConfigManager for channel definitions
RelayController  relayController;

// WebSocket layer — depends on RelayController and SceneManager references;
// the AsyncWebServer object is owned by webServer below.
WebSocketHandler webSocketHandler;

// HTTP server — owns the AsyncWebServer instance; wires WebSocketHandler in
// begin(); depends on ConfigManager and RelayController.
WebServer        webServer;

// Scene preset manager
SceneManager     sceneManager;

// Countdown timer engine (T032)
TimerEngine      timerEngine;

// MQTT client manager (T027)
MqttManager      mqttManager;

// ─────────────────────────────────────────────────────────────────────────────
// Modules not yet implemented — global instances will be added here when each
// task lands.  Placeholders document the intended declaration site.
// ─────────────────────────────────────────────────────────────────────────────

// T011
#include "wifi_manager.h"

// TODO: T024 — #include "language_manager.h"
//              LanguageManager languageManager;


#include "buzzer_controller.h"
#include "led_controller.h"
#include "reset_handler.h"

// ─────────────────────────────────────────────────────────────────────────────
// setup()
// ─────────────────────────────────────────────────────────────────────────────

void setup() {
    // ── 1. Serial ────────────────────────────────────────────────────────────
    // Initialise first so that every subsequent module can log to the console.
    // 115 200 baud matches platformio.ini monitor_speed.
    Serial.begin(115200);
    // Brief pause to let the host-side terminal open before first output.
    // This is the ONLY delay() call in the entire firmware; it belongs in
    // setup() where blocking is acceptable, and it is kept to 100 ms to stay
    // well within the 5-second boot budget.
    delay(100);

    // Print boot banner — version from config.h, MAC for device identification
    // on a shared network.
    Serial.println();
    Serial.println(F("=== Elmahdy Relay IoT Smart Controller ==="));
    Serial.print(F("Firmware version : "));
    Serial.println(F(VERSION));
    Serial.print(F("MAC address      : "));
    Serial.println(WiFi.macAddress());
    Serial.print(F("Chip ID          : 0x"));
    Serial.println(ESP.getChipId(), HEX);
    Serial.print(F("Flash size       : "));
    Serial.print(ESP.getFlashChipSize() / 1024);
    Serial.println(F(" KB"));
    Serial.print(F("Free heap        : "));
    Serial.print(ESP.getFreeHeap());
    Serial.println(F(" bytes"));
    Serial.println(F("=========================================="));

    // ── 2. ConfigManager ─────────────────────────────────────────────────────
    // Mounts LittleFS and loads all 7 config sections into RAM.  Every module
    // that follows depends on config being available.  A failed mount is fatal
    // on ESP8266 (no SD fallback); configManager.begin() will log the error
    // and the system will operate on factory defaults.
    Serial.println(F("[main] Initialising ConfigManager..."));
    configManager.begin();

    // ── 3. RelayController ───────────────────────────────────────────────────
    // Drives GPIO pins to the safe off-state (honouring RELAY_ACTIVE_LOW)
    // before the network stack starts.  This prevents relay chatter caused by
    // floating lines during the Wi-Fi/TCP init sequence.
    Serial.println(F("[main] Initialising RelayController..."));
    relayController.begin(configManager);

    // ── 4. WebServer (registers WebSocketHandler internally) ─────────────────
    // WebServer::begin() calls webSocketHandler.begin() and attaches the /ws
    // endpoint to the AsyncWebServer it owns, then binds all REST routes.
    Serial.println(F("[main] Initialising WebServer + WebSocketHandler..."));
    webServer.begin(configManager, relayController, webSocketHandler, sceneManager);

    // ── T011 ─────────────────────────────────────────────────────────────────
    Serial.println(F("[main] Initialising WiFiManager..."));
    wifiManager.begin(configManager);
    wifiManager.setOnConnected([]() {
        Serial.println(F("[main] Wi-Fi STA connected"));
        ledController.setWifiConnected(true);
        buzzerController.beepDouble();
    });
    webServer.setWifiManager(wifiManager);

    // ── TODO: T024 ───────────────────────────────────────────────────────────
    // languageManager.begin(configManager);

    // ── T027 — MqttManager ─────────────────────────────────────────────────────
    Serial.println(F("[main] Initialising MqttManager..."));
    mqttManager.begin(configManager, relayController);

    // ── T032/T033/T034 — TimerEngine ─────────────────────────────────────────
    // Wire expiry callback: fires the relay and broadcasts the updated state.
    timerEngine.setOnExpiry([](uint8_t channel, const char* targetState) {
        Serial.printf_P(PSTR("[main] Timer expired: ch=%u → %s\n"),
                        channel, targetState);
        // broadcastState() so the UI reflects the relay change immediately.
        webSocketHandler.broadcastState();
    });

    // Wire tick callback (T034): build a JsonArray of remaining times and push
    // it to every connected WebSocket client approximately every second.
    timerEngine.setOnTick([]() {
        if (webSocketHandler.socket().count() == 0) return;
        JsonDocument doc;
        timerEngine.getTimers(doc);
        JsonArray arr = doc["timers"].as<JsonArray>();
        webSocketHandler.broadcastTimerUpdate(arr);
    });

    timerEngine.begin(configManager, relayController);

    // Wire the engine into the REST layer so HTTP endpoints become functional.
    webServer.setTimerEngine(timerEngine);

    // ── T045 — SceneManager ───────────────────────────────────────────────────
    Serial.println(F("[main] Initialising SceneManager..."));
    sceneManager.begin(configManager, relayController);
    webServer.setSceneManager(sceneManager);

    // ── T047 — Wire scene/timer into MQTT dispatcher ──────────────────────────
    mqttManager.setSceneManager(sceneManager);
    mqttManager.setTimerEngine(timerEngine);

    // ── T051 — BuzzerController ───────────────────────────────────────────────
    Serial.println(F("[main] Initialising BuzzerController..."));
    buzzerController.begin(configManager);

    // ── T052 — LEDController ──────────────────────────────────────────────────
    Serial.println(F("[main] Initialising LEDController..."));
    ledController.begin(configManager);

    // ── T053 — ResetHandler ───────────────────────────────────────────────────
    Serial.println(F("[main] Initialising ResetHandler..."));
    resetHandler.begin(configManager);

    Serial.print(F("[main] Setup complete. Free heap: "));
    Serial.print(ESP.getFreeHeap());
    Serial.println(F(" bytes"));
}

// ─────────────────────────────────────────────────────────────────────────────
// loop()
//
// INVARIANT: No delay() calls.  Every tick() must return within microseconds.
// The ESPAsyncWebServer and AsyncMqttClient pump their own state machines
// from interrupt/callback context; we only need to keep the scheduler running
// by returning from loop() promptly.
// ─────────────────────────────────────────────────────────────────────────────

void loop() {
    // ── RelayController ───────────────────────────────────────────────────────
    // Checks active pulse-mode timers and calls setState(ch, false) for any
    // channel whose pulseDuration has elapsed.  O(MAX_CHANNELS) millis() reads.
    relayController.tick();

    // ── WebSocketHandler ──────────────────────────────────────────────────────
    // Calls AsyncWebSocket::cleanupClients() to reclaim slots held by
    // disconnected clients.  Must run regularly to prevent the client table
    // from filling with stale entries under high connect/disconnect churn.
    webSocketHandler.tick();

    // ── T011 — wifiManager.tick() ────────────────────────────────────────────
    // Drives AP+STA reconnect state machine, captive-portal DNS, mDNS updates.
    wifiManager.tick();

    // ── T027 — mqttManager.tick() ───────────────────────────────────────────────
    // Drives exponential-backoff reconnect and publishes any queued outbound
    // messages.
    mqttManager.tick();

    // ── T032 — timerEngine.tick() ────────────────────────────────────────────
    // Fires expired countdown timers; calls onTick ~every 1 s for WS broadcast.
    timerEngine.tick();

    // ── SceneManager — scheduled scene firing ────────────────────────────────
    sceneManager.tick();

    // ── T051 — Buzzer ─────────────────────────────────────────────────────────
    buzzerController.tick();

    // ── T052 — LED ────────────────────────────────────────────────────────────
    ledController.tick();

    // ── T053 — Reset button ───────────────────────────────────────────────────
    resetHandler.tick();
}

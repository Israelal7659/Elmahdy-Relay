#include "reset_handler.h"
#include "config_manager.h"
#include "../include/config.h"

ResetHandler resetHandler;

ResetHandler::ResetHandler()
    : _pin(RESET_PIN), _enabled(false), _lastState(HIGH),
      _pressed(false), _pressStartMs(0), _config(nullptr)
{}

void ResetHandler::begin(ConfigManager& config) {
    _config  = &config;
    auto& sys = config.systemConfig();
    _enabled = sys["resetEnabled"] | true;
    _pin     = sys["resetPin"]     | static_cast<uint8_t>(RESET_PIN);
    // RESET_PIN = GPIO16 has no internal pull-up; use external resistor.
    // INPUT_PULLUP not available on GPIO16; use INPUT.
    pinMode(_pin, INPUT);
    _lastState = digitalRead(_pin);
}

void ResetHandler::tick() {
    if (!_enabled || !_config) { return; }
    bool state = digitalRead(_pin);

    if (state == LOW && _lastState == HIGH) {
        // Button pressed (active-LOW)
        _pressed      = true;
        _pressStartMs = millis();
    } else if (state == HIGH && _lastState == LOW && _pressed) {
        // Button released — measure hold duration
        _pressed      = false;
        uint32_t held = millis() - _pressStartMs;

        if (held >= 10000UL) {
            Serial.println(F("[reset] Factory reset triggered (>10s)"));
            _config->resetAll();
            delay(200);
            ESP.restart();
        } else if (held >= 3000UL) {
            Serial.println(F("[reset] Reboot triggered (3-10s)"));
            delay(100);
            ESP.restart();
        } else {
            Serial.printf_P(PSTR("[reset] Short press (%ums) — ignored\n"),
                            static_cast<unsigned>(held));
        }
    }
    _lastState = state;
}

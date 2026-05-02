#include "led_controller.h"
#include "config_manager.h"
#include "../include/config.h"

LEDController ledController;

LEDController::LEDController()
    : _pin(LED_PIN), _enabled(false), _wifiConnected(false),
      _mqttConnected(false), _ledOn(false), _lastToggleMs(0),
      _intervalMs(200)
{}

void LEDController::begin(ConfigManager& config) {
    auto& sys = config.systemConfig();
    _enabled = sys["ledEnabled"] | true;
    _pin     = LED_PIN; // LED pin is fixed; no config override
    // Active-low on NodeMCU (GPIO2): HIGH = off
    digitalWrite(_pin, HIGH);
    pinMode(_pin, OUTPUT);
    _updateInterval();
}

void LEDController::setWifiConnected(bool connected) {
    _wifiConnected = connected;
    _updateInterval();
}

void LEDController::setMqttConnected(bool connected) {
    _mqttConnected = connected;
    _updateInterval();
}

void LEDController::_updateInterval() {
    if (_mqttConnected)      { _intervalMs = 0;    } // solid on
    else if (_wifiConnected) { _intervalMs = 1000; } // slow blink
    else                     { _intervalMs = 200;  } // fast blink (AP-only)
}

void LEDController::tick() {
    if (!_enabled) {
        digitalWrite(_pin, HIGH); // off
        return;
    }
    if (_intervalMs == 0) {
        // Solid on (active-low: LOW = on)
        if (!_ledOn) {
            digitalWrite(_pin, LOW);
            _ledOn = true;
        }
        return;
    }
    uint32_t now = millis();
    if (now - _lastToggleMs >= _intervalMs) {
        _lastToggleMs = now;
        _ledOn = !_ledOn;
        digitalWrite(_pin, _ledOn ? LOW : HIGH); // active-low
    }
}

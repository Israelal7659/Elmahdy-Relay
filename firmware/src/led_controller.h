#ifndef LED_CONTROLLER_H_
#define LED_CONTROLLER_H_

#include <Arduino.h>

class ConfigManager;

class LEDController {
public:
    LEDController();
    ~LEDController() = default;

    void begin(ConfigManager& config);
    void tick();

    void setWifiConnected(bool connected);
    void setMqttConnected(bool connected);

private:
    uint8_t  _pin;
    bool     _enabled;
    bool     _wifiConnected;
    bool     _mqttConnected;
    bool     _ledOn;
    uint32_t _lastToggleMs;
    uint16_t _intervalMs;  // 0 = solid on; >0 = blink period

    void _updateInterval();
};

extern LEDController ledController;

#endif /* LED_CONTROLLER_H_ */

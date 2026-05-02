#ifndef RESET_HANDLER_H_
#define RESET_HANDLER_H_

#include <Arduino.h>

class ConfigManager;

class ResetHandler {
public:
    ResetHandler();
    ~ResetHandler() = default;

    void begin(ConfigManager& config);
    void tick();

private:
    uint8_t        _pin;
    bool           _enabled;
    bool           _lastState;
    bool           _pressed;
    uint32_t       _pressStartMs;
    ConfigManager* _config;
};

extern ResetHandler resetHandler;

#endif /* RESET_HANDLER_H_ */

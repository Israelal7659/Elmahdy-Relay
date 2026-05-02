#ifndef SCENE_MANAGER_H_
#define SCENE_MANAGER_H_

#include <Arduino.h>
#include <ArduinoJson.h>

class ConfigManager;
class RelayController;

class SceneManager {
public:
    SceneManager() : _config(nullptr), _relay(nullptr) {}
    ~SceneManager() = default;

    void begin(ConfigManager& config, RelayController& relay);

    // Returns false if name already exists (case-insensitive) or limit (10) reached.
    bool createScene(const char* name, const JsonObject& states);

    // Activate a named scene: sets relay channels to stored states.
    // Returns false if scene not found.
    bool activate(const char* name);

    // Delete a scene by name. Returns false if not found.
    bool deleteScene(const char* name);

    // Populate out with {"scenes":[{"name":"...","states":{...}}, ...]}.
    void getScenes(JsonDocument& out) const;

    uint8_t count() const;

private:
    ConfigManager*   _config;
    RelayController* _relay;

    // Returns array index of named scene (case-insensitive), or -1 if not found.
    int _findScene(const char* name) const;
};

#endif /* SCENE_MANAGER_H_ */

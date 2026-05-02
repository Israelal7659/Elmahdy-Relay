#include "scene_manager.h"
#include "config_manager.h"
#include "relay_controller.h"

void SceneManager::begin(ConfigManager& config, RelayController& relay) {
    _config = &config;
    _relay  = &relay;
}

bool SceneManager::createScene(const char* name, const JsonObject& states) {
    if (!_config || !name || name[0] == '\0') { return false; }

    JsonDocument& doc = _config->sceneConfigMut();
    if (!doc["scenes"].is<JsonArray>()) {
        doc["scenes"].to<JsonArray>();
    }
    JsonArray arr = doc["scenes"].as<JsonArray>();
    if (arr.size() >= 10) { return false; }
    if (_findScene(name) >= 0) { return false; }

    JsonObject scene  = arr.add<JsonObject>();
    scene["name"]     = name;
    JsonObject stObj  = scene["states"].to<JsonObject>();
    for (JsonPair kv : states) {
        stObj[kv.key()] = kv.value();
    }
    _config->saveScenes();
    return true;
}

bool SceneManager::activate(const char* name) {
    if (!_config || !_relay || !name) { return false; }
    int idx = _findScene(name);
    if (idx < 0) { return false; }

    JsonDocument& doc = _config->sceneConfigMut();
    JsonArray arr     = doc["scenes"].as<JsonArray>();
    JsonObject scene  = arr[idx].as<JsonObject>();
    JsonObject states = scene["states"].as<JsonObject>();

    for (JsonPair kv : states) {
        int ch = atoi(kv.key().c_str());
        if (ch < 1) { continue; }
        const char* val = kv.value() | "off";
        bool on = (strcmp(val, "on") == 0);
        _relay->setState(static_cast<uint8_t>(ch), on);
    }
    return true;
}

bool SceneManager::deleteScene(const char* name) {
    if (!_config || !name) { return false; }
    int idx = _findScene(name);
    if (idx < 0) { return false; }

    JsonDocument& doc = _config->sceneConfigMut();
    JsonArray arr     = doc["scenes"].as<JsonArray>();
    int last = static_cast<int>(arr.size()) - 1;
    if (idx != last) {
        arr[idx].set(arr[last]);
    }
    arr.remove(last);
    _config->saveScenes();
    return true;
}

void SceneManager::getScenes(JsonDocument& out) const {
    JsonArray dst = out["scenes"].to<JsonArray>();
    if (!_config) { return; }

    JsonDocument& doc = _config->sceneConfigMut();
    if (!doc["scenes"].is<JsonArray>()) { return; }
    JsonArray src = doc["scenes"].as<JsonArray>();

    for (JsonObject sc : src) {
        JsonObject dsc      = dst.add<JsonObject>();
        dsc["name"]         = sc["name"];
        JsonObject dstates  = dsc["states"].to<JsonObject>();
        if (sc["states"].is<JsonObject>()) {
            for (JsonPair kv : sc["states"].as<JsonObject>()) {
                dstates[kv.key()] = kv.value();
            }
        }
    }
}

uint8_t SceneManager::count() const {
    if (!_config) { return 0; }
    JsonDocument& doc = _config->sceneConfigMut();
    if (!doc["scenes"].is<JsonArray>()) { return 0; }
    return static_cast<uint8_t>(doc["scenes"].as<JsonArray>().size());
}

int SceneManager::_findScene(const char* name) const {
    if (!_config) { return -1; }
    JsonDocument& doc = _config->sceneConfigMut();
    if (!doc["scenes"].is<JsonArray>()) { return -1; }
    JsonArray arr = doc["scenes"].as<JsonArray>();
    for (int i = 0; i < static_cast<int>(arr.size()); i++) {
        const char* n = arr[i]["name"] | "";
        if (strcasecmp(n, name) == 0) { return i; }
    }
    return -1;
}

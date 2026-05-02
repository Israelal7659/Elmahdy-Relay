/*
 * language_manager.cpp — Language pack loading from LittleFS, string lookup by key.
 *
 * Target : ESP8266 (ESP-12F / NodeMCU), Arduino Core 3.x, C++17
 * Build  : PlatformIO (espressif8266)
 */

#include "language_manager.h"
#include "config_manager.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

/* ─────────────────────────────────────────────────────────────────────────────
 * Global singleton definition
 * ───────────────────────────────────────────────────────────────────────────*/
LanguageManager languageManager;

/* ─────────────────────────────────────────────────────────────────────────────
 * Constructor
 * ───────────────────────────────────────────────────────────────────────────*/
LanguageManager::LanguageManager() {
    _currentLanguage[0] = '\0';
}

/* ─────────────────────────────────────────────────────────────────────────────
 * begin()
 * ───────────────────────────────────────────────────────────────────────────*/
void LanguageManager::begin(ConfigManager& config) {
    // Read the persisted language preference; fall back to Arabic.
    const char* lang = config.systemConfig()["language"] | "ar";

    Serial.printf("[Lang] Initialising with language: %s\n", lang);

    if (!loadPack(lang)) {
        // If the preferred pack is missing (e.g. first flash, data not yet
        // uploaded) try the other language before giving up.
        Serial.printf("[Lang] Pack '%s' not found — trying fallback 'ar'\n", lang);
        if (!loadPack("ar")) {
            Serial.println(F("[Lang] WARNING: no language pack loaded"));
        }
    }
}

/* ─────────────────────────────────────────────────────────────────────────────
 * loadPack()
 * ───────────────────────────────────────────────────────────────────────────*/
bool LanguageManager::loadPack(const char* code) {
    if (!code || code[0] == '\0') {
        Serial.println(F("[Lang] loadPack: empty code"));
        return false;
    }

    // Build the file path: /lang_{code}.json
    String path;
    path.reserve(16);
    path = "/lang_";
    path += code;
    path += ".json";

    if (!LittleFS.exists(path)) {
        Serial.printf("[Lang] loadPack: file not found: %s\n", path.c_str());
        return false;
    }

    File f = LittleFS.open(path, "r");
    if (!f) {
        Serial.printf("[Lang] loadPack: cannot open %s\n", path.c_str());
        return false;
    }

    _strings.clear();
    DeserializationError err = deserializeJson(_strings, f);
    f.close();

    if (err) {
        Serial.printf("[Lang] loadPack: JSON parse failed for %s: %s\n",
                      path.c_str(), err.c_str());
        _strings.clear();
        return false;
    }

    // Commit the language code only after a successful parse.
    // strncpy guarantees NUL-termination within _currentLanguage.
    strncpy(_currentLanguage, code, sizeof(_currentLanguage) - 1);
    _currentLanguage[sizeof(_currentLanguage) - 1] = '\0';

    Serial.printf("[Lang] Loaded pack '%s' (%u keys)\n",
                  _currentLanguage,
                  static_cast<unsigned>(_strings.size()));
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * get()
 * ───────────────────────────────────────────────────────────────────────────*/
const char* LanguageManager::get(const char* key) const {
    if (!key) return "";

    // ArduinoJson v7: operator[] on a JsonDocument returns a JsonVariant.
    // .as<const char*>() returns nullptr if the key is absent or the value is
    // not a string — in that case we fall back to returning key itself.
    const char* value = _strings[key].as<const char*>();
    return (value != nullptr) ? value : key;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * getLanguage() / getCurrentLanguage()
 * ───────────────────────────────────────────────────────────────────────────*/
const char* LanguageManager::getLanguage() const {
    return _currentLanguage;
}

const char* LanguageManager::getCurrentLanguage() const {
    return _currentLanguage;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * getPackJson()
 * ───────────────────────────────────────────────────────────────────────────*/
String LanguageManager::getPackJson(const char* code) const {
    if (!code || code[0] == '\0') return String();

    String path;
    path.reserve(16);
    path = "/lang_";
    path += code;
    path += ".json";

    if (!LittleFS.exists(path)) {
        Serial.printf("[Lang] getPackJson: file not found: %s\n", path.c_str());
        return String();
    }

    File f = LittleFS.open(path, "r");
    if (!f) {
        Serial.printf("[Lang] getPackJson: cannot open %s\n", path.c_str());
        return String();
    }

    // Pre-allocate based on file size to avoid repeated String realloc.
    const size_t fileSize = f.size();
    String json;
    if (fileSize > 0) {
        json.reserve(fileSize);
    }

    while (f.available()) {
        json += static_cast<char>(f.read());
    }
    f.close();

    return json;
}

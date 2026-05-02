/*
 * language_manager.h — Language pack loading from LittleFS, string lookup by key.
 *
 * Target : ESP8266 (ESP-12F / NodeMCU), Arduino Core 3.x, C++17
 * Build  : PlatformIO (espressif8266)
 *
 * Design notes
 * ────────────
 * • Language packs are flat JSON objects stored at the LittleFS root:
 *     /lang_ar.json — Arabic strings (default)
 *     /lang_en.json — English strings
 *   Keys are dot-separated identifiers; values are translated strings.
 *
 * • The active pack is parsed once into _strings (ArduinoJson v7 JsonDocument)
 *   during begin() / loadPack().  Steady-state lookups via get() do not
 *   allocate any heap memory.
 *
 * • Dynamic heap allocation happens only during loadPack() (JSON parse) and
 *   getPackJson() (file read into String).  It is never triggered from an
 *   ISR or time-critical context.
 *
 * • No delay() calls anywhere in this module.
 */

#ifndef LANGUAGE_MANAGER_H_
#define LANGUAGE_MANAGER_H_

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// Full header not needed in TUs that only include this file.
class ConfigManager;

/* ─────────────────────────────────────────────────────────────────────────────
 * LanguageManager
 * ───────────────────────────────────────────────────────────────────────────*/
class LanguageManager {
public:
    LanguageManager();
    ~LanguageManager() = default;

    /*
     * begin() — read the language preference from SystemConfig and load the
     * corresponding language pack into _strings.
     *
     * Reads systemConfig()["language"] (default "ar") from ConfigManager.
     * Calls loadPack() to parse the pack into memory.
     * Must be called after ConfigManager::begin() and LittleFS::begin().
     */
    void begin(ConfigManager& config);

    /*
     * loadPack() — open /lang_{code}.json from LittleFS and parse it into
     * _strings.  Stores code in _currentLanguage on success.
     *
     * Returns true on success.
     * Returns false if the file does not exist or JSON parsing fails; in that
     * case _strings is cleared and _currentLanguage is unchanged.
     */
    bool loadPack(const char* code);

    /*
     * get() — look up key in the active language pack.
     * Returns the translated string value, or key itself as a fail-safe
     * fallback when the key is absent (so the UI always shows something
     * intelligible rather than an empty string).
     */
    const char* get(const char* key) const;

    /*
     * getLanguage() — return the currently-loaded language code ("ar" / "en").
     * Returns an empty string if no pack has been loaded yet.
     */
    const char* getLanguage() const;

    /*
     * getCurrentLanguage() — alias for getLanguage(); exposed for symmetry with
     * the ConfigManager field name.
     */
    const char* getCurrentLanguage() const;

    /*
     * getPackJson() — read the raw JSON file for code from LittleFS into a
     * String and return it.  No parsing is performed — this is intended for
     * the REST endpoint that streams the file directly to the browser.
     *
     * Returns an empty String if the file does not exist or cannot be read.
     */
    String getPackJson(const char* code) const;

private:
    // In-memory parsed language pack.  Populated by loadPack().
    JsonDocument _strings;

    // Null-terminated language code ("ar", "en", or "" before begin()).
    // Fixed-size buffer avoids a heap String member in steady state.
    char _currentLanguage[4];  // max 2-char code + NUL, with one spare byte
};

/* ─────────────────────────────────────────────────────────────────────────────
 * Global singleton — extern declaration; definition is in language_manager.cpp
 * ───────────────────────────────────────────────────────────────────────────*/
extern LanguageManager languageManager;

#endif /* LANGUAGE_MANAGER_H_ */

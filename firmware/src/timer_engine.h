/*
 * timer_engine.h — Countdown timer management with LittleFS persistence.
 *
 * T032 scope: countdown timers only (type="countdown").
 * Scheduled timers (type="scheduled") are added in T037/T038.
 *
 * Target : ESP8266 (ESP-12F / NodeMCU), Arduino Core 3.x, C++17
 * Build  : PlatformIO (espressif8266)
 *
 * Design notes
 * ─────────────
 * • All timer slots are held in a fixed-size static array — no heap allocation
 *   after begin().
 * • tick() is called every loop() iteration; it uses millis() deltas for
 *   remaining-time tracking so it never blocks.
 * • Persistence delegates to ConfigManager::saveTimers() which performs the
 *   CRC32-validated atomic write described in config_manager.h.
 * • startedAt is stored as epoch seconds (time(nullptr)) when NTP is
 *   available, otherwise millis()/1000 is used as a monotonic fallback.
 *   On reload after power-cycle, the elapsed wall-clock time is subtracted
 *   from duration to compute the adjusted remaining time.
 * • Timers that have already expired at load time fire immediately and are
 *   removed so no stale entries accumulate.
 * • No delay() calls anywhere — entirely non-blocking.
 */

#ifndef TIMER_ENGINE_H_
#define TIMER_ENGINE_H_

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include <ctime>

#include "../include/config.h"

// Forward declarations — full headers included only in the .cpp.
class ConfigManager;
class RelayController;

/* ─────────────────────────────────────────────────────────────────────────────
 * TimerEntry — internal representation of one active countdown or scheduled timer.
 *
 * Fields mirror the data-model.md timer object.
 * ───────────────────────────────────────────────────────────────────────────*/
struct TimerEntry {
    uint16_t id;              // Auto-incremented unique identifier (> 0)
    uint8_t  channel;         // Relay channel (1–4)
    char     targetState[8];  // "on" | "off" | "toggle"
    bool     enabled;         // Whether the timer is active
    char     type[12];        // "countdown" | "scheduled"

    // Countdown fields (only used when type == "countdown")
    uint32_t duration;        // Original duration in milliseconds
    uint32_t startedAt;       // Epoch seconds when timer was created/resumed

    // Scheduled fields (only used when type == "scheduled")
    uint8_t  hour;            // 0–23
    uint8_t  minute;          // 0–59
    char     repeatMode[8];   // "once" | "daily" | "weekly" | "custom"
    uint8_t  dayMask;         // bit n = weekday n (0=Sun..6=Sat); 0 = every day

    // Runtime (not persisted)
    uint32_t remainingMs;     // Countdown: ms left; Scheduled: unused (0)
    uint32_t lastMillis;      // Countdown: millis() snapshot; Scheduled: unused (0)
};

/* ─────────────────────────────────────────────────────────────────────────────
 * TimerEngine
 * ───────────────────────────────────────────────────────────────────────────*/
class TimerEngine {
public:
    TimerEngine();
    ~TimerEngine() = default;

    /*
     * begin() — load persisted timers from ConfigManager and start tracking.
     *
     * For each loaded countdown timer:
     *   - Compute adjusted remaining = duration - (now_epoch - startedAt)*1000
     *   - If remaining <= 0: fire expiry immediately, remove entry, save.
     *   - Otherwise: restore with adjusted remaining so the countdown
     *     survives a power-cycle or reboot.
     *
     * Must be called once from setup() after ConfigManager::begin().
     */
    void begin(ConfigManager& config, RelayController& relay);

    /*
     * tick() — call every loop() iteration.
     *
     * Responsibilities:
     *   1. Advance remaining time for every active countdown timer using
     *      millis() delta since the last tick.
     *   2. Fire and remove any timers whose remaining time has reached zero.
     *   3. Every ~1000 ms call the onTick callback so the WebSocket handler
     *      can broadcast remaining times to connected clients.
     *
     * Never blocks; uses only millis() arithmetic.
     */
    void tick();

    /*
     * createCountdown() — add a new countdown timer.
     *
     * Parameters:
     *   channel     — relay channel (1–4)
     *   durationMs  — countdown duration in milliseconds (> 0)
     *   targetState — "on" | "off" | "toggle"
     *
     * Returns the assigned timer ID (> 0) on success.
     * Returns 0 if the MAX_TIMERS limit has been reached or inputs are
     * invalid (channel out of range, zero duration, unknown targetState).
     *
     * On success the timer is persisted immediately via ConfigManager.
     */
    uint16_t createCountdown(uint8_t channel,
                             uint32_t durationMs,
                             const char* targetState);

    /*
     * createScheduled() — add a new scheduled timer (T038).
     *
     * The timer fires every day (or on specific days per dayMask) at the
     * given hour:minute in local time (as configured via NTP/configTime).
     *
     * Parameters:
     *   channel     — relay channel (1–4)
     *   targetState — "on" | "off" | "toggle"
     *   hour        — 0–23 local hour
     *   minute      — 0–59 local minute
     *   repeatMode  — "once" | "daily" | "weekly" | "custom"
     *   dayMask     — bitmask: bit 0=Sun … bit 6=Sat; 0 = every day
     *
     * Returns the assigned timer ID (> 0) on success, 0 on error.
     * Requires NTP to be synced; fires will be silently skipped otherwise.
     */
    uint16_t createScheduled(uint8_t channel,
                             const char* targetState,
                             uint8_t hour,
                             uint8_t minute,
                             const char* repeatMode,
                             uint8_t dayMask);

    /*
     * cancel() — remove a timer by ID.
     *
     * Returns true if the timer was found and removed; false if not found.
     * On success the updated timer list is persisted via ConfigManager.
     */
    bool cancel(uint16_t timerId);

    /*
     * getTimers() — populate outDoc with the current timer list.
     *
     * outDoc is cleared and filled with a "timers" JsonArray.  Each element
     * contains: id, channel, type, targetState, enabled, duration, remaining.
     * Returns the number of timers written.
     */
    uint8_t getTimers(JsonDocument& outDoc) const;

    /*
     * getRemaining() — return remaining milliseconds for a countdown timer.
     *
     * Returns 0 if the timer is not found or has expired.
     * The value is computed from the last-known remainingMs minus the
     * millis() delta since that snapshot was taken.
     */
    uint32_t getRemaining(uint16_t timerId) const;

    /*
     * setOnExpiry() — register a callback invoked when any timer fires.
     *
     * The callback receives the relay channel and the target state string.
     * It is called from tick() context (main loop, not ISR).
     * Signature: void(uint8_t channel, const char* targetState)
     */
    void setOnExpiry(std::function<void(uint8_t, const char*)> cb);

    /*
     * setOnTick() — register a callback invoked approximately every second
     * while at least one countdown timer is active.
     *
     * Intended use: build a JsonArray of remaining times and call
     * webSocketHandler.broadcastTimerUpdate().
     * Signature: void()
     */
    void setOnTick(std::function<void()> cb);

private:
    // ── Storage ───────────────────────────────────────────────────────────────

    // Fixed-size array; no heap allocation after begin().
    TimerEntry _timers[MAX_TIMERS];
    uint8_t    _count;   // Number of active entries in _timers[]

    // Non-owning pointers; valid after begin().
    ConfigManager*   _config;
    RelayController* _relay;

    // ── Callbacks ─────────────────────────────────────────────────────────────
    std::function<void(uint8_t, const char*)> _onExpiry;
    std::function<void()>                     _onTick;

    // ── Timing ────────────────────────────────────────────────────────────────

    // millis() at the last 1-second tick broadcast.
    uint32_t _lastBroadcastMs;

    // Epoch-minute (epoch/60*60) at which scheduled timers were last checked.
    // Ensures scheduled timers fire at most once per minute.
    uint32_t _lastScheduledMinEpoch;

    // ── Internal helpers ──────────────────────────────────────────────────────

    /*
     * _findById() — linear search for a timer by id.
     * Returns pointer into _timers[] or nullptr if not found.
     */
    TimerEntry*       _findById(uint16_t id);
    const TimerEntry* _findById(uint16_t id) const;

    /*
     * _removeAt() — remove the entry at index i by swapping with the last
     * entry and decrementing _count.  O(1), preserves no ordering guarantee.
     */
    void _removeAt(uint8_t index);

    /*
     * _fire() — apply the timer action to the relay and invoke _onExpiry.
     * Called just before the entry is removed.
     */
    void _fire(const TimerEntry& entry);

    /*
     * _persist() — serialise _timers[] back into ConfigManager's timer doc
     * and call saveTimers().
     *
     * The in-memory doc (_config->timerConfigMut()) is rebuilt from scratch
     * each call to keep it consistent with _timers[].
     */
    void _persist();

    /*
     * _epochNow() — return current epoch seconds.
     *
     * Uses time(nullptr) when NTP has set the system clock (year >= 2020).
     * Falls back to millis()/1000 as a monotonic approximation when NTP
     * is not yet available.  This approximation is safe because startedAt
     * is only compared against itself across reboots where millis() resets;
     * if millis() overflowed the worst case is an early expiry, not a hang.
     */
    static uint32_t _epochNow();

    /*
     * _tickScheduled() — check all scheduled timers against the current local
     * time and fire any whose hour:minute:dayMask matches.
     * Called from tick() at most once per minute when NTP is available.
     */
    void _tickScheduled();

    /*
     * _validateTargetState() — returns true for "on", "off", or "toggle".
     */
    static bool _validateTargetState(const char* s);

    /*
     * _validateRepeatMode() — returns true for "once", "daily", "weekly", "custom".
     */
    static bool _validateRepeatMode(const char* s);
};

#endif /* TIMER_ENGINE_H_ */

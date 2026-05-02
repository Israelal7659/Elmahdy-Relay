# Data Model: Elmahdy Relay IoT Smart Controller

**Feature Branch**: `001-iot-relay-controller`
**Date**: 2026-05-02
**Storage**: LittleFS JSON files with CRC32 integrity validation

---

## Storage Architecture

All configuration is stored as separate JSON files in LittleFS. Each file has an independent lifecycle — corruption of one file affects only that functional area.

| File | Max Size | Contents |
|------|----------|----------|
| `/wifi.json` | ~256B | WiFi credentials, AP config, static IP |
| `/mqtt.json` | ~256B | MQTT broker settings |
| `/relays.json` | ~512B | Channel definitions (up to 4) |
| `/relays_state.json` | ~64B | Last relay ON/OFF states |
| `/timers.json` | ~1KB | Timer definitions (up to 8) |
| `/scenes.json` | ~1KB | Scene definitions (up to 10) |
| `/system.json` | ~256B | System preferences |

**Write Pattern**: write to `.tmp` → CRC32 → flush → remove old → rename `.tmp` to `.json`
**Read Pattern**: open `.json` → verify CRC32 → parse JSON → if invalid, load factory defaults for that section only

---

## Entity Definitions

### WiFiConfig (`/wifi.json`)

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `ssid` | string | `""` | Home WiFi SSID (max 32) |
| `password` | string | `""` | Home WiFi password (max 63) |
| `staticIp` | string | `""` | Static IP or empty=DHCP |
| `gateway` | string | `""` | Gateway for static IP |
| `subnet` | string | `"255.255.255.0"` | Subnet mask |
| `apPassword` | string | `"12345678"` | AP password (8–63 chars) |
| `crc` | uint32 | — | Integrity checksum |

Derived: AP SSID = `ElmahdyRelay_` + last 4 hex of MAC. AP IP = `192.168.4.1`.

### MqttConfig (`/mqtt.json`)

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `enabled` | bool | `false` | MQTT on/off |
| `broker` | string | `"broker.hivemq.com"` | Broker host (max 64) |
| `port` | uint16 | `1883` | Broker port |
| `username` | string | `""` | Auth username (max 32) |
| `password` | string | `""` | Auth password (max 32) |
| `prefix` | string | `"elmahdy"` | Topic prefix (max 20) |
| `crc` | uint32 | — | Integrity checksum |

### RelayChannel (array in `/relays.json`)

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | uint8 | 1–4 | Channel identifier |
| `pin` | uint8 | 5,4,14,12 | Output GPIO |
| `name` | string | `"قناة X"` | Name, max 20 UTF-8 chars |
| `powerOnState` | enum | `"last"` | `"last"` / `"on"` / `"off"` |
| `pulseDuration` | uint32 | `0` | Pulse ms (0=disabled) |
| `interlockGroup` | uint8 | `0` | Group ID (0=none) |

Container: `channelCount` (uint8, default 2, range 1–4), `channels[]`, `crc`.

Runtime (in-memory): `state` (bool), `pulseEndTime` (uint32 millis).
Persisted state (`/relays_state.json`): `states[]` (bool[4]), `crc`.

### Timer (array in `/timers.json`)

| Field | Type | Description |
|-------|------|-------------|
| `id` | uint16 | Auto-increment unique ID |
| `channel` | uint8 | Target relay (1–4) |
| `type` | enum | `"countdown"` or `"scheduled"` |
| `targetState` | enum | `"on"` / `"off"` / `"toggle"` |
| `enabled` | bool | Active flag |
| `duration` | uint32 | Countdown ms (countdown only) |
| `startedAt` | uint32 | Epoch when started (countdown only) |
| `hour` | uint8 | 0–23 (scheduled only) |
| `minute` | uint8 | 0–59 (scheduled only) |
| `repeatMode` | enum | `"once"/"daily"/"weekdays"/"weekend"/"custom"` |
| `dayMask` | uint8 | Bitmask, bit0=Sun (scheduled custom only) |

Container: `nextId` (auto-increment), `timers[]` (max 8), `crc`.

### Scene (array in `/scenes.json`)

| Field | Type | Description |
|-------|------|-------------|
| `name` | string | Max 20 UTF-8, case-insensitive unique |
| `states[]` | object[] | Channel-state pairs (1–4 entries) |
| `states[].channel` | uint8 | Channel ID (1–4) |
| `states[].state` | bool | Target ON/OFF |

Container: `scenes[]` (max 10), `crc`.

### SystemConfig (`/system.json`)

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `buzzerEnabled` | bool | `true` | Buzzer feedback on/off |
| `ledEnabled` | bool | `true` | LED status on/off |
| `resetEnabled` | bool | `true` | Reset button on/off |
| `buzzerPin` | uint8 | `13` | Buzzer GPIO |
| `resetPin` | uint8 | `16` | Reset button GPIO |
| `hostname` | string | `"elmahdyrelay"` | mDNS hostname (max 20) |
| `language` | enum | `"ar"` | `"ar"` or `"en"` |
| `timezoneOffset` | int16 | `120` | UTC offset minutes |
| `crc` | uint32 | — | Integrity checksum |

---

## Validation Rules

1. Timer/scene channel refs MUST match valid relay channel IDs.
2. Max 8 timers, max 10 scenes (hard limits).
3. GPIO pins must be unique across relays; no conflict with buzzer/reset pins.
4. GPIO 0, 2, 15 trigger UI warning (override with confirmation).
5. Scene names: case-insensitive uniqueness.
6. Channel names: max 20 UTF-8 characters.

## Flash Budget

LittleFS ~2MB partition. Total estimated usage: ~115KB (config ~4KB + lang packs ~7KB + web assets gzipped ~100KB + temp files ~1KB). Budget remaining: ~1.9MB.

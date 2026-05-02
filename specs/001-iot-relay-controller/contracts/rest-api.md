# REST API Contract: Elmahdy Relay

**Base URL**: `http://{device-ip}` (default AP: `192.168.4.1`)
**Content-Type**: `application/json`
**CORS**: `Access-Control-Allow-Origin: *` on all `/api/` endpoints

---

## Device Info

### `GET /api/info`
Returns API metadata and available endpoints.

**Response** `200`:
```json
{
  "name": "Elmahdy Relay",
  "version": "1.0.0",
  "mac": "AA:BB:CC:DD:EE:FF",
  "channels": 2,
  "endpoints": ["/api/status", "/api/relay", "/api/config", "/api/timers", "/api/scenes", "/api/system"]
}
```

---

## Relay Control

### `GET /api/status`
Returns all relay states and system info.

**Response** `200`:
```json
{
  "relays": [
    { "id": 1, "name": "قناة 1", "state": true, "pulse": false },
    { "id": 2, "name": "قناة 2", "state": false, "pulse": false }
  ],
  "wifi": { "ssid": "HomeNet", "rssi": -45, "ip": "192.168.1.100" },
  "mqtt": { "connected": true, "broker": "broker.hivemq.com" },
  "uptime": 3600,
  "version": "1.0.0",
  "ntpSynced": true,
  "time": "2026-05-02T14:30:00"
}
```

### `POST /api/relay/{ch}`
Control a single relay. `ch` = 1–4.

**Request**:
```json
{ "action": "on" }
```
`action`: `"on"`, `"off"`, or `"toggle"`.

**Response** `200`:
```json
{ "id": 1, "state": true }
```

**Error** `400`: `{ "error": "Invalid channel" }`

### `POST /api/relay/all`
Control all relays.

**Request**:
```json
{ "action": "on" }
```

**Response** `200`:
```json
{ "relays": [{ "id": 1, "state": true }, { "id": 2, "state": true }] }
```

---

## Configuration

### `GET /api/config/{section}`
Read a config section. `section`: `wifi`, `mqtt`, `relays`, `system`.

**Response** `200`: Section-specific JSON (see data-model.md).

### `POST /api/config/{section}`
Update a config section. Body = section-specific JSON fields to update.

**Response** `200`: `{ "success": true, "reboot": false }`
WiFi changes set `"reboot": true` (device reboots to apply).

---

## Timers

### `GET /api/timers`
List all timers.

**Response** `200`:
```json
{
  "timers": [
    { "id": 1, "channel": 1, "type": "countdown", "targetState": "off", "enabled": true, "duration": 1800000, "remaining": 900000 },
    { "id": 2, "channel": 2, "type": "scheduled", "targetState": "on", "enabled": true, "hour": 18, "minute": 0, "repeatMode": "daily" }
  ],
  "count": 2,
  "max": 8
}
```

### `POST /api/timer`
Create or update a timer.

**Request** (countdown):
```json
{ "channel": 1, "type": "countdown", "targetState": "off", "duration": 1800000 }
```

**Request** (scheduled):
```json
{ "channel": 2, "type": "scheduled", "targetState": "on", "hour": 18, "minute": 0, "repeatMode": "daily" }
```

**Response** `200`: `{ "id": 3, "success": true }`
**Error** `400`: `{ "error": "Timer limit reached (max 8)" }`

### `DELETE /api/timer/{id}`
Delete a timer.

**Response** `200`: `{ "success": true }`
**Error** `404`: `{ "error": "Timer not found" }`

---

## Scenes

### `GET /api/scenes`
List all scenes.

**Response** `200`:
```json
{
  "scenes": [
    { "name": "وضع النوم", "states": [{ "channel": 1, "state": false }, { "channel": 3, "state": true }] }
  ],
  "count": 1,
  "max": 10
}
```

### `POST /api/scene`
Create or update a scene.

**Request**:
```json
{ "name": "Night Mode", "states": [{ "channel": 1, "state": false }, { "channel": 2, "state": false }] }
```

**Response** `200`: `{ "success": true }`
**Error** `400`: `{ "error": "Scene limit reached (max 10)" }`

### `POST /api/scene/{name}/activate`
Activate a scene by name.

**Response** `200`: Updated relay states.
**Error** `404`: `{ "error": "Scene not found" }`

### `DELETE /api/scene/{name}`
Delete a scene.

**Response** `200`: `{ "success": true }`

---

## System

### `POST /api/system/reboot`
Reboot the device.

**Response** `200`: `{ "success": true, "message": "Rebooting..." }`

### `POST /api/system/reset`
Factory reset all configuration.

**Response** `200`: `{ "success": true, "message": "Factory reset, rebooting..." }`

### `POST /api/system/update`
OTA firmware upload. Multipart form with `.bin` file.

**Response** `200`: `{ "success": true, "message": "Update complete, rebooting..." }`
**Error** `400`: `{ "error": "File too large for OTA partition" }`

### `GET /api/backup`
Download all configuration as a single JSON file.

**Response** `200`: JSON with all config sections combined.
`Content-Disposition: attachment; filename="elmahdy-backup.json"`

### `POST /api/restore`
Upload and restore configuration from backup JSON.

**Response** `200`: `{ "success": true, "message": "Restored, rebooting..." }`

### `GET /api/wifi/scan`
Scan for available WiFi networks.

**Response** `200`:
```json
{ "networks": [{ "ssid": "HomeNet", "rssi": -45, "secure": true }] }
```

### `GET /api/lang/{code}`
Get language strings. `code`: `ar` or `en`.

**Response** `200`: Full language pack JSON.

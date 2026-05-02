# WebSocket Contract: Elmahdy Relay

**Endpoint**: `ws://{device-ip}/ws`
**Protocol**: Raw WebSocket (no sub-protocol)
**Max Clients**: 4 concurrent connections

---

## Message Format

All messages are JSON objects with a `type` field.

### Client → Server (Commands)

#### Relay Control
```json
{ "type": "relay", "id": 1, "action": "toggle" }
```
`action`: `"on"`, `"off"`, `"toggle"`

#### All Relays
```json
{ "type": "relayAll", "action": "on" }
```

#### Scene Activation
```json
{ "type": "scene", "name": "Night Mode" }
```

#### Request Full State
```json
{ "type": "getState" }
```

---

### Server → Client (Events)

#### State Update (broadcast to all clients)
```json
{
  "type": "state",
  "relays": [
    { "id": 1, "state": true },
    { "id": 2, "state": false }
  ]
}
```
Sent on: any relay state change (from any source: web, MQTT, button, timer, scene).

#### Timer Update
```json
{
  "type": "timer",
  "timers": [
    { "id": 1, "remaining": 845000 }
  ]
}
```
Sent: every 1 second for active countdown timers.

#### System Info (periodic)
```json
{
  "type": "info",
  "rssi": -45,
  "uptime": 3600,
  "mqtt": true,
  "heap": 32000
}
```
Sent: every 5 seconds.

#### Config Changed
```json
{ "type": "configChanged", "section": "relays" }
```
Sent when any config section is updated (triggers UI reload of that section).

---

## Connection Lifecycle

1. Client connects to `ws://{ip}/ws`
2. Server sends initial `state` message with all relay states
3. Server sends `info` message with system status
4. Bidirectional communication begins
5. Server broadcasts state changes to ALL connected clients
6. On disconnect, client slot is freed

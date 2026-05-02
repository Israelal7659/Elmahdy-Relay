# MQTT Contract: Elmahdy Relay

**Default Broker**: `broker.hivemq.com:1883`
**Default Prefix**: `elmahdy`
**QoS**: 0 for state updates, 1 for control commands and LWT

---

## Topic Structure

All topics use the configurable `{prefix}`. Default: `elmahdy`.

### Relay Control & Status

| Topic | Direction | Payload | Description |
|-------|-----------|---------|-------------|
| `{prefix}/relay/{ch}/control` | Subscribe | `ON`, `OFF`, `TOGGLE` | Control single relay |
| `{prefix}/relay/{ch}/status` | Publish | `ON`, `OFF` | Relay state (retained) |
| `{prefix}/relay/all/control` | Subscribe | `ON`, `OFF` | Control all relays |
| `{prefix}/relay/all/status` | Publish | JSON: `{"1":"ON","2":"OFF"}` | All states |

### System

| Topic | Direction | Payload | Description |
|-------|-----------|---------|-------------|
| `{prefix}/system/status` | Publish (LWT) | `online`, `offline` | Device availability |
| `{prefix}/system/info` | Publish | JSON | Version, uptime, RSSI, IP |

### Timers & Scenes

| Topic | Direction | Payload | Description |
|-------|-----------|---------|-------------|
| `{prefix}/timer/{id}/control` | Subscribe | `CANCEL` | Cancel timer |
| `{prefix}/scene/{name}/control` | Subscribe | `ON` | Activate scene |

---

## Home Assistant Auto-Discovery

Published on every MQTT connect with `retain: true`.

### Discovery Topic
`homeassistant/switch/{device_id}/relay_{ch}/config`

Where `{device_id}` = `elmahdy_relay_` + MAC (no colons, lowercase).

### Discovery Payload (per relay channel)
```json
{
  "name": "Elmahdy Relay CH1",
  "unique_id": "elmahdy_relay_aabbccddeeff_1",
  "command_topic": "elmahdy/relay/1/control",
  "state_topic": "elmahdy/relay/1/status",
  "payload_on": "ON",
  "payload_off": "OFF",
  "availability_topic": "elmahdy/system/status",
  "payload_available": "online",
  "payload_not_available": "offline",
  "device": {
    "identifiers": ["elmahdy_relay_aabbccddeeff"],
    "name": "Elmahdy Relay",
    "model": "ESP8266-4CH",
    "manufacturer": "Elmahdy",
    "sw_version": "1.0.0"
  }
}
```

---

## Last Will & Testament (LWT)

| Setting | Value |
|---------|-------|
| Topic | `{prefix}/system/status` |
| Payload | `offline` |
| QoS | 1 |
| Retain | true |

On successful connect, device publishes `online` to the same topic (retained).

## Reconnection

Exponential backoff: 1s → 2s → 4s → 8s → 16s → 30s (max). Resets on successful connect.

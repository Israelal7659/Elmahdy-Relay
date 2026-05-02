# Feature Specification: Elmahdy Relay IoT Smart Controller

**Feature Branch**: `001-iot-relay-controller`  
**Created**: 2026-05-02  
**Status**: Draft  
**Input**: Full-product specification for a production-grade ESP8266 IoT smart relay controller (1–4 channels) with bilingual Arabic/English support, sold as commercial hardware.

## User Scenarios & Testing *(mandatory)*

### User Story 1 — First Boot & WiFi Setup (Priority: P1)

A user powers on the Elmahdy Relay device for the first time. The device creates a WiFi access point named `ElmahdyRelay_XXXX` (last 4 hex chars of MAC) with password `12345678`. The user connects their phone to this AP. A captive portal automatically redirects the phone browser to `192.168.4.1`, showing the setup page in Arabic. The user scans for available home WiFi networks, selects one, enters its password, and saves. The device connects to the home WiFi while keeping the AP active simultaneously (AP+STA mode). If the home WiFi is unavailable, the AP always remains active for local control.

**Why this priority**: Without WiFi setup, no other feature is accessible. This is the absolute first interaction every user has with the product.

**Independent Test**: Power on an unconfigured device, connect to its AP from a phone, complete WiFi setup, and verify the device joins the home network while AP remains accessible.

**Acceptance Scenarios**:

1. **Given** a factory-fresh device, **When** it is powered on, **Then** it creates an AP named `ElmahdyRelay_XXXX` within 5 seconds and the setup page loads in Arabic at `192.168.4.1`.
2. **Given** the setup page is open, **When** the user taps "Scan Networks", **Then** nearby 2.4 GHz WiFi networks appear within 10 seconds with signal strength indicators.
3. **Given** a network is selected and password entered, **When** the user taps "Save", **Then** the device connects to the home WiFi within 15 seconds and shows a success message with the assigned IP address.
4. **Given** the device is connected to home WiFi, **When** the user checks the AP, **Then** the AP remains active and the setup/dashboard page is still accessible at `192.168.4.1`.
5. **Given** a wrong WiFi password is entered, **When** the device fails to connect, **Then** it shows an error message, keeps the AP running, retries 3 times, then stops retrying with clear feedback.

---

### User Story 2 — Dashboard Relay Control (Priority: P1)

A user opens the dashboard from a phone or desktop browser. They see relay cards with toggle switches (one per configured channel). Each card shows the channel name (in Arabic or English). Tapping a toggle turns the relay ON or OFF. State updates are instant via WebSocket with no page reload. "All ON" and "All OFF" buttons control all channels simultaneously. The dashboard displays WiFi signal strength (RSSI bars), device uptime, MQTT connection status, and firmware version. A buzzer beeps on toggle if enabled. The built-in LED reflects connection state. A language toggle (AR/EN) in the header switches the entire UI direction and labels.

**Why this priority**: This is the core value proposition — controlling relays from a web interface. Users interact with this page most frequently.

**Independent Test**: Open the dashboard, toggle relays, verify physical relay clicks and real-time state updates across multiple browser clients.

**Acceptance Scenarios**:

1. **Given** the dashboard is loaded, **When** the user views the page, **Then** relay cards appear for each configured channel with correct names and current ON/OFF states.
2. **Given** a relay is OFF, **When** the user taps its toggle switch, **Then** the relay physically clicks ON within 100ms and all connected WebSocket clients update within 50ms.
3. **Given** multiple channels are configured, **When** the user taps "All ON", **Then** all relays turn ON simultaneously.
4. **Given** the UI language is Arabic, **When** the user taps the language toggle, **Then** the entire UI switches to English LTR layout instantly without page reload.
5. **Given** two users are controlling the same relay simultaneously, **When** both send commands, **Then** the last command wins and all WebSocket clients reflect the final state.

---

### User Story 3 — Relay & GPIO Configuration (Priority: P1)

A user navigates to Configuration > Relay Settings. They select the number of active channels (1 to 4), assign a GPIO pin per channel (defaults: GPIO5, GPIO4, GPIO14, GPIO12), name each channel in Arabic or English (max 20 characters), and set power-on behavior per channel (Last State, Always ON, or Always OFF). The firmware warns if the user selects a boot-sensitive GPIO (0, 2, 15). All settings are saved with corruption-safe write patterns and persist across all power cycles and reboots.

**Why this priority**: Channel configuration determines how many relays the device controls and how they behave. Essential for initial device setup.

**Independent Test**: Configure channels with custom names and GPIOs, reboot the device, verify all settings persist and relays respond on the correct GPIO pins.

**Acceptance Scenarios**:

1. **Given** the Relay Settings page, **When** the user sets active channels to 3, **Then** three channel configuration forms appear with default GPIO assignments.
2. **Given** a channel name field, **When** the user types "إضاءة المطبخ" (14 chars), **Then** the name is accepted and displayed correctly on the dashboard.
3. **Given** a user selects GPIO 0 for a channel, **When** they attempt to save, **Then** a warning about boot-sensitive GPIO appears, allowing override with confirmation.
4. **Given** channels are configured, **When** the device is power-cycled 100 times, **Then** all channel settings (names, GPIOs, power-on states) persist with zero loss.
5. **Given** power-on state is set to "Last State" and the relay was ON, **When** the device reboots, **Then** the relay returns to ON state after boot.

---

### User Story 4 — MQTT Remote Control (Priority: P2)

A user configures MQTT from the Configuration page (default broker: `broker.hivemq.com`, port `1883`, prefix: `elmahdy`). The device connects and publishes LWT online status. Sending "ON", "OFF", or "TOGGLE" to `{prefix}/relay/{ch}/control` toggles the relay. The device publishes state to `{prefix}/relay/{ch}/status`. A system info topic publishes firmware version, uptime, RSSI, and IP. On boot, the device publishes Home Assistant-compatible MQTT Auto-Discovery messages for zero-config detection.

**Why this priority**: MQTT enables remote control and smart home integration, a key differentiator for the product, but the device must be fully functional locally without it.

**Independent Test**: Configure MQTT, subscribe to status topics, publish control commands, verify relay toggles and state feedback. Verify Home Assistant auto-discovers the device.

**Acceptance Scenarios**:

1. **Given** MQTT is configured with valid broker details, **When** the device boots, **Then** it connects to the broker and publishes an "online" LWT message within 10 seconds.
2. **Given** MQTT is connected, **When** "ON" is published to `elmahdy/relay/1/control`, **Then** relay 1 turns ON and `elmahdy/relay/1/status` publishes "ON".
3. **Given** the MQTT broker becomes unreachable, **When** the device loses connection, **Then** it retries with exponential backoff and the device remains fully functional via local web UI.
4. **Given** Home Assistant is running on the same network, **When** the device boots with MQTT enabled, **Then** Home Assistant automatically discovers all relay channels with correct names.
5. **Given** MQTT is disabled in settings, **When** the device operates, **Then** all local features work normally with zero MQTT-related errors or delays.

---

### User Story 5 — Countdown Timer (Priority: P2)

A user sets a countdown on any channel — e.g., "turn OFF channel 2 after 30 minutes." The timer shows remaining time on the dashboard. When the countdown reaches zero, the relay toggles and the buzzer beeps. The user can cancel an active timer. Timers persist across power cycles — if the device reboots mid-countdown, the timer resumes with adjusted remaining time. Maximum 8 timers total across all channels.

**Why this priority**: Countdown timers are one of the most requested features for relay controllers (e.g., turning off lights after a set period).

**Independent Test**: Set a 2-minute countdown, verify dashboard shows countdown, verify relay toggles when time expires, reboot mid-countdown and verify it resumes.

**Acceptance Scenarios**:

1. **Given** the timer settings page, **When** the user sets a 30-minute countdown for channel 2 to turn OFF, **Then** the timer starts and the dashboard shows remaining time.
2. **Given** a countdown is active, **When** the timer reaches zero, **Then** the relay toggles to the target state and the buzzer beeps (if enabled).
3. **Given** a countdown with 15 minutes remaining, **When** the device reboots, **Then** the timer resumes with approximately 15 minutes minus reboot time remaining.
4. **Given** 8 timers are already active, **When** the user tries to create a 9th, **Then** a clear error message is shown in the current language.
5. **Given** an active countdown, **When** the user cancels it, **Then** the timer stops and the relay remains in its current state.

---

### User Story 6 — Scheduled Timer with NTP (Priority: P2)

The device syncs time via NTP on WiFi connect (timezone configurable). A user creates a schedule such as "turn ON channel 1 at 18:00." Repeat modes include: once, daily, weekdays only, weekend only, and custom days (select specific days). Schedules persist across power cycles. If NTP is unavailable, scheduled timers pause (no false triggers) while countdown timers continue working.

**Why this priority**: Scheduled automation is essential for daily routines (e.g., lights at sunset, irrigation schedules).

**Independent Test**: Create a schedule for 2 minutes in the future, verify relay toggles at the scheduled time, change timezone and verify schedule adjusts.

**Acceptance Scenarios**:

1. **Given** the device is connected to WiFi, **When** it boots, **Then** it syncs time via NTP and displays the current time on the dashboard.
2. **Given** the timer page, **When** the user creates a daily schedule "ON channel 1 at 18:00", **Then** the schedule saves and triggers daily at 18:00.
3. **Given** a weekdays-only schedule, **When** Saturday arrives, **Then** the schedule does not trigger.
4. **Given** NTP is unavailable, **When** a scheduled timer's time arrives, **Then** the schedule pauses and does not trigger (no false triggers).
5. **Given** a schedule exists, **When** the device reboots, **Then** the schedule persists and triggers at the next scheduled time.

---

### User Story 7 — Scene Presets (Priority: P3)

A user creates named scenes grouping multiple channel states — e.g., "Night Mode": CH1=OFF, CH2=OFF, CH3=ON, CH4=OFF. Scene names support Arabic and English. Scenes are triggerable from dashboard buttons, MQTT (`{prefix}/scene/{name}/control`), and the PWA. Scenes persist across power cycles. Maximum 10 scenes.

**Why this priority**: Scenes add convenience by allowing one-tap control of multiple relays, but require relay control (P1) to be working first.

**Independent Test**: Create a scene with specific channel states, trigger it from dashboard and MQTT, verify all channels switch to the defined states.

**Acceptance Scenarios**:

1. **Given** the scene settings page, **When** the user creates a scene "وضع النوم" with CH1=OFF, CH2=OFF, CH3=ON, **Then** the scene is saved and appears on the dashboard.
2. **Given** a saved scene, **When** the user taps the scene button on the dashboard, **Then** all channels switch to the scene's defined states within 100ms.
3. **Given** MQTT is connected, **When** "ON" is published to `elmahdy/scene/night/control`, **Then** the "night" scene activates.
4. **Given** 10 scenes exist, **When** the user tries to create an 11th, **Then** a clear error message is shown in the current language.
5. **Given** scenes are configured, **When** the device reboots, **Then** all scenes persist and remain triggerable.

---

### User Story 8 — Bilingual Language Switch (Priority: P1)

A language toggle button in the UI header (AR/EN) allows instant switching. Tapping it switches the entire UI: all labels, buttons, messages, errors, and placeholders. Arabic activates full RTL layout. English activates LTR. The switch is instant with no page reload. Language preference persists across sessions. Default language: Arabic.

**Why this priority**: The product is sold in Arabic-speaking markets. Full bilingual support is a core product requirement, not a nice-to-have.

**Independent Test**: Switch language from Arabic to English and back, verify all UI text changes, layout direction flips, and preference persists across reboots.

**Acceptance Scenarios**:

1. **Given** the UI is in Arabic, **When** the user taps the language toggle, **Then** all UI text switches to English and the layout changes from RTL to LTR instantly.
2. **Given** the language is set to English, **When** the device reboots, **Then** the UI loads in English.
3. **Given** Arabic mode is active, **When** viewing any page, **Then** all text, buttons, labels, error messages, and placeholders are in Arabic with correct RTL layout and no broken alignments.

---

### User Story 9 — Buzzer, LED & Reset Button (Priority: P3)

System Settings allows enabling/disabling the buzzer, built-in LED, and reset button independently. The buzzer produces short beeps on relay toggle, double-beep on WiFi connect, and long beep on factory reset, with anti-spam protection (100ms minimum between activations). The LED indicates: fast blink = AP mode only, slow blink = WiFi connected, solid = MQTT connected. The reset button (default GPIO16/D0) supports: short press (<3s) = toggle relay 1, long press (3–10s) = reboot, very long press (>10s) = factory reset.

**Why this priority**: Hardware feedback is important for production polish but the product is fully functional without buzzer/LED/reset button hardware.

**Independent Test**: Press reset button at various durations, verify correct actions. Toggle buzzer/LED settings, verify behavior changes.

**Acceptance Scenarios**:

1. **Given** the buzzer is enabled, **When** a relay toggles, **Then** a short beep sounds and no additional beep occurs within 100ms.
2. **Given** the reset button is pressed for <3 seconds, **When** released, **Then** relay 1 toggles.
3. **Given** the reset button is pressed for >10 seconds, **When** released, **Then** the device performs a factory reset (rapid buzzer beep) and reboots to AP mode.
4. **Given** the LED is enabled and WiFi is connected, **When** MQTT connects, **Then** the LED changes from slow blink to solid.
5. **Given** buzzer is disabled in settings, **When** a relay toggles, **Then** no beep occurs.

---

### User Story 10 — OTA Firmware Update (Priority: P3)

System Settings has a "Firmware Update" section. The user uploads a `.bin` file via file picker. The firmware validates file size before flashing. A progress bar shows during the flash. The device auto-reboots after successful update. All user configuration is preserved after OTA. If the update fails or power is lost during flash, dual-bank OTA preserves the previous working firmware.

**Why this priority**: OTA updates are essential for product lifecycle but are used infrequently compared to daily relay control.

**Independent Test**: Upload a new firmware file, verify progress bar, verify device reboots with new version, verify all config is preserved.

**Acceptance Scenarios**:

1. **Given** the OTA page, **When** the user selects a valid `.bin` file, **Then** the firmware validates the file size fits in the OTA partition before flashing.
2. **Given** a valid file is uploaded, **When** flashing is in progress, **Then** a progress bar shows completion percentage.
3. **Given** OTA completes successfully, **When** the device reboots, **Then** the new firmware version is shown and all user configuration is intact.
4. **Given** power is lost during OTA, **When** the device restarts, **Then** it boots the previous working firmware.

---

### User Story 11 — PWA Mobile Installation (Priority: P3)

The device serves a full PWA manifest with app name "Elmahdy Relay", theme color, and icons (48×48 and 192×192). A service worker caches static assets for offline local network access. The browser prompts "Add to Home Screen" on supported devices. The app opens as a standalone app without browser chrome.

**Why this priority**: PWA enhances the user experience but the web UI works fine in a regular browser tab.

**Independent Test**: Open the dashboard on Android Chrome, verify "Add to Home Screen" prompt appears, install, launch from home screen and verify standalone mode.

**Acceptance Scenarios**:

1. **Given** the dashboard is opened in a supported mobile browser, **When** the browser detects the PWA manifest, **Then** an "Add to Home Screen" prompt appears.
2. **Given** the app is installed on the home screen, **When** launched, **Then** it opens in standalone mode without browser chrome.
3. **Given** the PWA is installed, **When** the phone has no internet but is on the local WiFi, **Then** cached static assets load and the app functions normally.

---

### User Story 12 — Interlock & Pulse Mode (Priority: P2)

Users define interlock groups in Relay Settings where only one relay can be ON at a time (safety for motors, curtains). When relay A in a group turns ON, relay B auto-turns OFF. Pulse/inching mode per channel: the relay stays ON for a configured duration (e.g., 2 seconds) then automatically turns OFF (for garage doors, gate openers). Duration is configurable per channel.

**Why this priority**: Interlock is a safety feature preventing hardware damage. Pulse mode expands the product's use cases.

**Independent Test**: Configure two relays in an interlock group, turn on relay A then relay B, verify A turns off. Set pulse mode on a channel, toggle it, verify it auto-turns OFF after the configured duration.

**Acceptance Scenarios**:

1. **Given** relays 1 and 2 are in an interlock group, **When** relay 1 is ON and the user turns ON relay 2, **Then** relay 1 automatically turns OFF before relay 2 turns ON.
2. **Given** pulse mode is set to 2 seconds on channel 3, **When** the user toggles channel 3 ON, **Then** it turns ON, waits 2 seconds, then automatically turns OFF.
3. **Given** interlock and MQTT are both active, **When** an MQTT command turns ON a grouped relay, **Then** the interlock rule is enforced and the other relay in the group turns OFF.

---

### User Story 13 — Backup & Restore (Priority: P3)

System Settings has a "Backup" button that downloads all device configuration as a single file. A "Restore" button uploads the file and restores all configuration. The device reboots after restore. This is useful for cloning settings across multiple devices or recovering after a factory reset.

**Why this priority**: Backup/restore streamlines multi-device deployment but is not needed for basic device operation.

**Independent Test**: Configure a device fully, download backup, factory reset, restore from backup, verify all settings are restored.

**Acceptance Scenarios**:

1. **Given** the system settings page, **When** the user taps "Backup", **Then** a single file containing all device configuration is downloaded.
2. **Given** a backup file from another device, **When** the user uploads it via "Restore", **Then** all configuration is applied and the device reboots with the restored settings.
3. **Given** a factory-reset device, **When** the user restores from a backup, **Then** WiFi, MQTT, relay, timer, scene, and system settings are all restored.

---

### Edge Cases

- **Storage full**: When device storage is full, new timer or scene creation is rejected with a clear error message in the current language.
- **Wrong WiFi password**: Device shows an error, keeps the AP running, retries 3 times then stops retrying with user feedback.
- **MQTT broker unreachable**: Device remains fully functional locally; MQTT retries in the background with exponential backoff.
- **NTP server unreachable**: Scheduled timers pause (no false triggers); countdown timers continue working since they use elapsed-time tracking.
- **Boot-sensitive GPIO selected**: A warning is displayed in the UI; the user can override with explicit confirmation.
- **Config file corrupted**: Factory defaults load for only the corrupted section; the user is notified which section was reset.
- **Power failure during OTA**: Dual-bank OTA preserves the previous working firmware; the device boots to the last known good version.
- **Simultaneous control by two users**: The last command wins; all connected clients receive the updated state.
- **Maximum limits exceeded**: 8 timers and 10 scenes are the hard limits; clear error messages are shown when limits are reached.
- **Buzzer/LED/reset button not wired**: Features degrade gracefully; the device operates normally without optional hardware.

## Clarifications

### Session 2026-05-02

- Q: How should scene and timer identity/uniqueness be enforced? → A: Scenes are uniquely identified by name (case-insensitive); timers are uniquely identified by auto-generated numeric ID.
- Q: Is web UI password protection in-scope? → A: No. Local network trust model is sufficient — no password protection for the web UI. Removed entirely (not deferred).
- Q: What should the dashboard show when zero channels are configured? → A: Default 2 channels on first boot; minimum channel count is 1 (no zero-channel state possible).
- Q: Should relay output polarity be user-configurable from the web UI? → A: No. Polarity is a compile-time constant (`#define`), default active-LOW. Changed only by rebuilding firmware — not a runtime/web UI setting.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Device MUST create a WiFi access point on first boot within 5 seconds with a unique SSID based on its hardware identifier and a default password.
- **FR-002**: Device MUST run in simultaneous AP+STA mode, keeping the access point active at all times regardless of home WiFi connectivity status.
- **FR-003**: Device MUST provide a captive portal that automatically redirects connected clients to the setup page on first boot.
- **FR-004**: Device MUST scan for and display available 2.4 GHz WiFi networks with signal strength indicators.
- **FR-005**: Device MUST support 1 to 4 independently configurable relay channels (default: 2 channels on first boot, minimum: 1), each with a configurable output pin, user-defined name (max 20 characters in Arabic or English), and power-on state (Last State, Always ON, Always OFF).
- **FR-006**: Device MUST toggle relay output within 100ms of receiving a command from any source (web UI, MQTT, physical button).
- **FR-007**: Device MUST push relay state changes to all connected clients via real-time bidirectional communication within 50ms of state change.
- **FR-008**: Device MUST provide "All ON" and "All OFF" controls that affect all configured channels simultaneously.
- **FR-009**: Device MUST support MQTT with configurable broker, port, username, password, and topic prefix for remote control and monitoring.
- **FR-010**: Device MUST publish MQTT auto-discovery messages compatible with Home Assistant on every boot when MQTT is enabled.
- **FR-011**: Device MUST publish a Last Will and Testament (LWT) message for online/offline status detection.
- **FR-012**: Device MUST support countdown timers on any channel with configurable duration, persisting across power cycles with adjusted remaining time on reboot.
- **FR-013**: Device MUST support scheduled timers with NTP time synchronization, supporting repeat modes: once, daily, weekdays, weekend, and custom day selection.
- **FR-014**: Device MUST pause scheduled timers when NTP time is unavailable to prevent false triggers; countdown timers MUST continue functioning using elapsed-time tracking.
- **FR-015**: Device MUST enforce a maximum of 8 total timers (countdown and scheduled combined) and 10 scenes, with clear error messages when limits are reached.
- **FR-016**: Device MUST support named scene presets that set multiple channels to defined states simultaneously, triggerable from web UI, MQTT, and PWA.
- **FR-017**: Device MUST support full bilingual operation (Arabic and English) with instant language switching, RTL/LTR layout changes, and language preference persistence.
- **FR-018**: Device MUST store all language strings as external language packs, with Arabic as the default language.
- **FR-019**: Device MUST provide configurable buzzer feedback with anti-spam protection (minimum 100ms between activations) and distinct patterns for relay toggle, WiFi connect, and factory reset.
- **FR-020**: Device MUST provide LED status indication with distinct patterns for AP-only mode (fast blink), WiFi connected (slow blink), and MQTT connected (solid).
- **FR-021**: Device MUST support a configurable physical reset button with three press durations: short (<3s) toggles relay 1, long (3–10s) reboots, very long (>10s) factory resets.
- **FR-022**: Device MUST support OTA firmware updates via file upload through the web UI with size validation, progress indication, and automatic reboot on success.
- **FR-023**: Device MUST preserve all user configuration across firmware updates.
- **FR-024**: Device MUST provide dual-bank OTA protection — if an update fails or power is lost during flash, the previous working firmware is preserved.
- **FR-025**: Device MUST serve a valid PWA manifest enabling "Add to Home Screen" installation on supported mobile browsers, with a service worker caching static assets.
- **FR-026**: Device MUST support interlock groups where only one relay in a group can be ON at a time, automatically turning OFF other relays in the group when one is activated.
- **FR-027**: Device MUST support pulse/inching mode per channel with configurable duration — the relay turns ON for the specified time then automatically turns OFF.
- **FR-028**: Device MUST provide backup functionality that exports all configuration as a single downloadable file.
- **FR-029**: Device MUST provide restore functionality that imports a configuration file and applies all settings with a device reboot.
- **FR-030**: Device MUST store all configuration in separate files per functional area (WiFi, MQTT, relays, timers, scenes, system) with corruption-safe write patterns (write-to-temp, verify integrity, rename-to-active).
- **FR-031**: Device MUST validate each configuration file independently on boot; if any file is corrupted, load factory defaults for ONLY that section and notify the user.
- **FR-032**: Device MUST warn users when selecting boot-sensitive output pins (GPIO 0, 2, 15) and require explicit confirmation before saving.
- **FR-033**: Device MUST auto-reconnect to MQTT with exponential backoff when the broker becomes unreachable, without affecting local functionality.
- **FR-034**: Device MUST display real-time system information on the dashboard: WiFi signal strength (RSSI bars), uptime, MQTT connection status, and firmware version.
- **FR-035**: Device MUST support mDNS discovery with a configurable hostname.
- **FR-036**: Device MUST provide a complete set of web endpoints accessible from external applications with appropriate cross-origin headers.
- **FR-037**: Device MUST support configurable timezone offset for NTP time synchronization.
- **FR-038**: Device MUST gracefully degrade when optional hardware (buzzer, reset button) is not physically connected — all other features MUST continue working.

### Key Entities

- **Relay Channel**: Represents a single controllable output. Attributes: identifier (1–4), output pin, name (Arabic or English, max 20 chars), ON/OFF state, power-on behavior (last/on/off), pulse duration, interlock group membership.
- **Timer**: Represents a scheduled or countdown automation action. Identity: auto-generated numeric ID (unique across all timers). Attributes: identifier, associated channel, type (countdown or scheduled), duration or scheduled time, repeat mode (once/daily/weekdays/weekend/custom), day selection mask, enabled state, remaining time (for persistence). A single channel may have multiple timers.
- **Scene**: Represents a named preset of multiple channel states. Identity: name (case-insensitive uniqueness — "Night" and "night" are considered the same scene). Attributes: name (Arabic or English), list of channel-state pairs, enabled state.
- **WiFi Configuration**: Stores network connectivity settings. Attributes: home network name, home network password, static IP/gateway/subnet (optional), access point password.
- **MQTT Configuration**: Stores remote messaging settings. Attributes: broker address, port, username, password, topic prefix, enabled state.
- **System Configuration**: Stores device-wide preferences. Attributes: buzzer enabled, LED enabled, reset button enabled, buzzer output pin, reset button input pin, hostname, language (ar/en), timezone offset, firmware version.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Device boots to a functional and controllable state within 5 seconds of power-on.
- **SC-002**: Web interface loads completely within 2 seconds on the local network.
- **SC-003**: Relay responds to any toggle command within 100ms (from command receipt to physical output change).
- **SC-004**: Real-time state updates reach all connected clients within 50ms of a state change.
- **SC-005**: Zero configuration loss across 100 consecutive power cycles on physical hardware.
- **SC-006**: Firmware binary remains under 500KB; web assets and configuration storage remain under 512KB.
- **SC-007**: MQTT reconnects within 30 seconds of the broker becoming available after a disconnection.
- **SC-008**: Factory reset via physical button restores all defaults and reboots within 3 seconds.
- **SC-009**: All 13 user stories pass their acceptance scenarios on physical target hardware.
- **SC-010**: UI is fully functional in both Arabic (RTL) and English (LTR) with no layout breaks, truncated text, or misaligned elements.
- **SC-011**: PWA is installable on Android Chrome and iOS Safari, opening in standalone mode without browser chrome.
- **SC-012**: All web endpoints are accessible from external applications via cross-origin requests.
- **SC-013**: Device remains fully controllable via web UI when internet is unavailable (MQTT and NTP are optional enhancements only).
- **SC-014**: Countdown timers survive device reboots and resume with adjusted remaining time accurate to within 5 seconds.
- **SC-015**: Users can complete first-time WiFi setup in under 3 minutes from power-on to connected state.

## Assumptions

- Target hardware modules have 4MB flash memory (ESP-12F or equivalent form factor).
- Relay outputs are active-LOW by default (LOW signal = relay ON). This is a compile-time constant (`#define RELAY_ACTIVE_LOW true`), changeable only by rebuilding firmware — not configurable from the web UI.
- Users have a modern smartphone with a web browser for initial setup and daily operation.
- Home WiFi networks are 2.4 GHz (hardware limitation of the target platform).
- Maximum 8 timers total (countdown + scheduled combined) and 10 scenes are hard limits due to memory constraints.
- Default channel count on first boot is 2; minimum configurable channel count is 1 (dashboard always shows at least one relay card).
- The default public MQTT broker requires no authentication; private brokers support username/password.
- Single-user control model — no authentication of any kind for the web UI. The device trusts all clients on the local network. No password protection is implemented (not deferred — explicitly out of scope).
- Initial firmware flash is performed via USB programming tool; subsequent updates are OTA only.
- Buzzer and reset button are optional hardware components — features degrade gracefully if not physically connected.
- No activity/event logging is implemented (avoids flash wear and unnecessary complexity).
- No physical wall switch inputs — control is exclusively via web UI, MQTT, web endpoints, and PWA.
- The "About" page displays product branding: "Elmahdy Relay", © Sayed Elmahdy, contact: 01093307397.
- Device does not support 5 GHz WiFi networks (hardware limitation).
- Time synchronization requires internet access; offline devices rely solely on elapsed-time countdown timers.

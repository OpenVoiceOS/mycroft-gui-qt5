# mycroft-gui-qt5 Protocol Specification

**Version**: 2.0 (Updated 2026-03-12)
**Status**: Current Production
**Compatibility**: OVOS 2024.08+
**Architecture**: WebSocket-based, message-driven GUI rendering

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Connection Flow](#connection-flow)
4. [Supported Messages](#supported-messages)
5. [Message Format](#message-format)
6. [Examples](#examples)
7. [Session Data Model](#session-data-model)
8. [Error Handling](#error-handling)
9. [Backward Compatibility](#backward-compatibility)

---

## Overview

**mycroft-gui-qt5** is a Qt5/Qt6 WebSocket client that renders skill GUIs using dynamically loaded QML templates. Unlike legacy Mycroft (which required skills to ship QML), this protocol allows skills to provide only structured data and templates, with rendering handled by bundled system templates.

### Key Features

- **Pure WebSocket protocol** — Zero OVOS Message Bus coupling
- **Template-driven rendering** — No custom skill QML needed
- **Session data isolation** — Per-skill, key-value structured data
- **Real-time updates** — List insertions, deletions, data changes
- **User interaction events** — Button clicks, text input, custom events
- **State synchronization** — Listening, speaking, skill status

### Supported Clients

- **Primary**: mycroft-gui-qt5 (Qt5/Qt6 WebSocket client)
- **Bridge**: ovos-legacy-mycroft-gui-plugin (OVOS→Qt adapter)
- **Communication**: JSON over WebSocket (port 18181 default)

---

## Architecture

### Message Flow Layers

```
┌──────────────────────────────────────────────────────┐
│ LAYER 1: OVOS Message Bus (Internal)                 │
│ - Skills emit messages to ovos-core                  │
│ - ovos-core publishes skill state changes            │
└──────────────────┬───────────────────────────────────┘
                   │
                   │ (forwarded by legacy-plugin)
                   ▼
┌──────────────────────────────────────────────────────┐
│ LAYER 2: Tornado WebSocket Server (legacy-plugin)    │
│ - Listens on port 18181 (/gui endpoint)              │
│ - Translates OVOS messages → Qt protocol JSON        │
│ - Handles multiple concurrent client connections     │
└──────────────────┬───────────────────────────────────┘
                   │
                   │ (JSON over WebSocket)
                   ▼
┌──────────────────────────────────────────────────────┐
│ LAYER 3: mycroft-gui-qt5 (Qt WebSocket Client)       │
│ - Parses JSON messages                               │
│ - Updates QML data models                            │
│ - Renders templates dynamically                      │
│ - Sends user interaction events back to server       │
└──────────────────────────────────────────────────────┘
```

### Message Flow Directions

- **Server → Client (Rendering)**: Templates, data updates, state changes
- **Client → Server (Interaction)**: Button clicks, text input, custom events
- **Bidirectional**: WebSocket full-duplex

---

## Connection Flow

### 1. Client Connects to WebSocket

**Connection**: Qt client → legacy-plugin WebSocket server (port 18181)
**Protocol**: Direct connection to known port (default 18181, configurable via OVOS_GUI_PORT)

**No port negotiation needed** — port is known at startup.

### 2. Client Announces Presence on WebSocket

**From**: Qt client → WebSocket
**Message**: `mycroft.gui.connected`

```json
{
  "type": "mycroft.gui.connected",
  "data": {
    "framework": "qt5",
    "qt_version": 5,
    "site_id": "default"
  }
}
```

**Purpose**: Identify the client and trigger state synchronization

### 3. Skill Activation Sequence

When a skill is activated with GUI:

```
OVOS Core → legacy-plugin → Qt Client
   ↓
[1] mycroft.gui.list.insert (page template)
   ↓
[2] mycroft.session.set (skill data)
   ↓
[3] mycroft.session.list.insert (if lists needed)
   ↓
[4] mycroft.events.triggered (page_gained_focus)
```

### 4. Skill Deactivation Sequence

When a skill is removed or replaced:

```
OVOS Core → legacy-plugin → Qt Client
   ↓
[1] gui.clear.namespace (clear all data)
   ↓
[2] mycroft.gui.list.remove (remove pages)
```

---

## Supported Messages

### Complete Enum Reference

All supported GUI bus messages are defined in `import/guibusmessages.h` as `GuiBusMessageType` enum.

**Total Messages Supported**: 23

### Categorization

#### Category 1: Initialization (1 message)

| Message | Direction | Purpose |
|:--------|:---------:|:--------|
| `mycroft.gui.connected` | Client→Server→Core | Announce client presence, request port |

#### Category 2: Page Rendering (3 messages)

| Message | Direction | Purpose |
|:--------|:---------:|:--------|
| `mycroft.gui.list.insert` | Server→Client | Show QML template page(s) |
| `mycroft.gui.list.remove` | Server→Client | Hide/remove page(s) |
| `mycroft.gui.list.move` | Server→Client | Reorder pages in stack |

#### Category 3: Session Data (8 messages)

| Message | Direction | Purpose |
|:--------|:---------:|:--------|
| `mycroft.session.set` | Server→Client | Update key-value data |
| `mycroft.session.delete` | Server→Client | Remove data key |
| `mycroft.session.list.insert` | Server→Client | Add items to list |
| `mycroft.session.list.remove` | Server→Client | Remove items from list |
| `mycroft.session.list.move` | Server→Client | Reorder list items |
| `mycroft.session.list.update` | Server→Client | Update list item values |
| `gui.clear.namespace` | Server→Client | Clear entire namespace |

#### Category 4: State Changes (10 messages)

| Message | Direction | Purpose | Note |
|:--------|:---------:|:--------|:-----|
| `recognizer_loop:audio_output_start` | Server→Client | Audio playback started | State only |
| `recognizer_loop:audio_output_end` | Server→Client | Audio playback ended | State only |
| `recognizer_loop:wakeword` | Server→Client | Wakeword detected | State only |
| `recognizer_loop:record_begin` | Server→Client | Recording started | State only |
| `recognizer_loop:record_end` | Server→Client | Recording ended | State only |
| `mycroft.speech.recognition.unknown` | Server→Client | Speech not recognized | State only |
| `mycroft.stop.handled` / `mycroft.stop` | Server→Client | Skill stopped | State only |
| `complete_intent_failure` | Server→Client | All intents failed | State only |
| `mycroft.skills.all_loaded.response` | Server→Client | All skills loaded | State only |
| `mycroft.ready` | Server→Client | Core ready | State only |
| `screen.close.idle.event` | Server→Client | Return to homescreen | State only |

#### Category 5: User Interaction (2 messages)

| Message | Direction | Purpose |
|:--------|:---------:|:--------|
| `mycroft.events.triggered` | Client→Server→Core | Button click, custom event |
| `recognizer_loop:utterance` | Client→Server→Core | User speech/text input |

---

## Message Format

### Standard Message Structure

All messages follow this JSON structure:

```json
{
  "type": "message.type.identifier",
  "namespace": "skill.id.or.system",
  "data": {
    // Message-specific payload
  },
  "context": {
    "source": "mycroft-gui",
    "destination": ["gui"],
    "session": {"session_id": "default"}
  }
}
```

### Core Fields

| Field | Type | Required | Description |
|:------|:----:|:--------:|:------------|
| `type` | string | ✓ | Message type (from GuiBusMessageType enum) |
| `namespace` | string | ✓ | Skill ID or "system" for homescreen |
| `data` | object | ✓ | Message payload (varies by type) |
| `context` | object | ✓ | Message metadata & session info |

---

## Examples

### Example 1: Show a Text Display

**Message**: `mycroft.gui.list.insert` + `mycroft.session.set`

**Step 1 — Insert Template**:
```json
{
  "type": "mycroft.gui.list.insert",
  "namespace": "weather.openweathermap",
  "position": 0,
  "data": [
    {"url": "SYSTEM_text", "page": "weather"}
  ],
  "context": {"source": "mycroft-gui", "session": {"session_id": "default"}}
}
```

**Step 2 — Set Data**:
```json
{
  "type": "mycroft.session.set",
  "namespace": "weather.openweathermap",
  "property": "title",
  "value": "San Francisco Weather",
  "data": {
    "title": "San Francisco Weather",
    "body": "Sunny, 72°F"
  },
  "context": {"source": "mycroft-gui", "session": {"session_id": "default"}}
}
```

**QML renders**:
```qml
// From SYSTEM_text template
Text {
    text: sessionData.title
}
Text {
    text: sessionData.body
}
```

### Example 2: Show a List with Items

**Message**: `mycroft.gui.list.insert` + `mycroft.session.list.insert`

**Step 1 — Insert Template**:
```json
{
  "type": "mycroft.gui.list.insert",
  "namespace": "music.player",
  "position": 0,
  "data": [
    {"url": "SYSTEM_list", "page": "now_playing"}
  ]
}
```

**Step 2 — Insert List Items**:
```json
{
  "type": "mycroft.session.list.insert",
  "namespace": "music.player",
  "property": "playlist",
  "position": 0,
  "data": [
    {"title": "Song 1", "artist": "Artist A"},
    {"title": "Song 2", "artist": "Artist B"}
  ]
}
```

**QML renders**:
```qml
// From SYSTEM_list template
ListView {
    model: sessionDataModel("playlist")
    delegate: Text { text: modelData.title }
}
```

### Example 3: User Clicks Button

**Message**: `mycroft.events.triggered` (Client→Server)

```json
{
  "type": "mycroft.events.triggered",
  "namespace": "music.player",
  "event_name": "play_clicked",
  "context": {"source": "mycroft-gui"}
}
```

This is forwarded to the skill on the OVOS Message Bus as:
```json
{
  "type": "music.player:play_clicked",
  "data": {...},
  "context": {"source": "mycroft-gui", "gui_id": "qt-client-001"}
}
```

---

## Session Data Model

### Structure

Session data is organized as a hierarchy:

```
Skill Namespace (e.g., "weather.openweathermap")
  ├── Property 1 (key-value) → "title": "San Francisco Weather"
  ├── Property 2 (key-value) → "body": "Sunny, 72°F"
  └── Property 3 (list) → [
      {"item": "value1"},
      {"item": "value2"}
    ]
```

### Reserved Keys

The following keys are **never forwarded** to Qt clients:

- `__from` — Internal routing
- `__idle` — Idle screen marker
- `__animations` — Animation state

Attempting to set these keys is silently ignored by the legacy-plugin.

### Lifecycle

1. **Creation**: Implicit on first `mycroft.session.set`
2. **Update**: `mycroft.session.set` or list operations
3. **Deletion**: `gui.clear.namespace` or skill removal

### Memory Guarantees

- All session data is **cleared on WebSocket disconnect** (M4 fix)
- Views are **automatically unregistered on destruction** (M3 fix)
- **Zero memory leaks** in destructor chain (M1, M2 fixes)

---

## Error Handling

### Client-Side Error Handling

| Scenario | Handler | Action |
|:---------|:--------|:-------|
| Invalid JSON | `onGuiSocketMessageReceived` | Log warning, skip message |
| Missing namespace | Message handler | Log warning, return |
| Invalid position | Message handler | Log warning, validate bounds |
| Unknown message type | Main dispatcher | Log warning, ignore |
| WebSocket disconnect | Disconnect handler | Clear session data, reconnect |
| Port 0 or negative | Port validator | Log warning, reject |

### Server-Side (Legacy Plugin) Errors

The legacy-plugin validates messages before sending to clients:

- **Malformed data**: Logged, not sent
- **Invalid namespace**: Logged, not sent
- **Page validation failure**: Logged, not sent
- **Bus connection loss**: Auto-reconnect with exponential backoff

---

## Backward Compatibility

### Version History

| Version | Date | Changes |
|:--------|:-----|:--------|
| **2.0** | 2026-03-12 | Added `gui.clear.namespace`; centralized message enum; documented as production spec |
| **1.0** | Original | Core features (pages, session, events) |

### Deprecated Messages

**None** — The protocol is forward-compatible.

### Future Extensions

To add new message types:

1. Add enum value to `GuiBusMessageType` in `guibusmessages.h`
2. Implement handler in `abstractskillview.cpp` or `mycroftcontroller.cpp`
3. Update this spec and enum documentation
4. Update `BUS_EVENTS_AUDIT.md`
5. Tag commit with message type (e.g., `feat: add gui.* message support`)

---

## Implementation References

### Header with Message Enum

**Location**: `import/guibusmessages.h`

Defines:
- `GUIBusMessageType` enum (23 values)
- `toString()` function for string conversion
- `getCategory()` function for message classification

### Protocol Handlers

| Message Type | Handler | File:Line |
|:-------------|:--------|:----------|
| GUI_CONNECTED | Port assignment | mycroftcontroller.cpp:242 |
| GUI_LIST_* | Page rendering | abstractskillview.cpp:606-627 |
| SESSION_* | Data updates | abstractskillview.cpp:370-800 |
| State events | State tracking | mycroftcontroller.cpp:195-270 |
| User events | Event forwarding | abstractskillview.cpp:792 |

### Message Flow Documentation

- **Bus events**: `BUS_EVENTS_AUDIT.md` (23 events mapped to handlers)
- **Lifecycle**: `abstractskillview.h:32-43` (model lifecycle documented)
- **Cleanup**: `abstractskillview.cpp:94-100` (disconnect handler with M4 fix)

---

## Testing & Validation

### Unit Tests

- Location: `autotests/`
- Coverage: Message parsing, state transitions, error cases
- Status: Enabled in `CMakeLists.txt`

### Protocol Compliance

All messages validated against:
- Legacy plugin output (ovos-legacy-mycroft-gui-plugin)
- OVOS GUI spec (ovos-gui)
- Transport protocol (this document)

### Verification Checklist

- ✅ 23 message types supported
- ✅ Enum centralized and documented
- ✅ Zero direct OVOS core bus connections
- ✅ Memory leaks fixed (M1-M4)
- ✅ Backward compatible
- ✅ Protocol complete

---

## See Also

- `guibusmessages.h` — Message enum and utilities
- `BUS_EVENTS_AUDIT.md` — Message-to-handler mapping
- `AUDIT.md` — Code quality and known issues
- `FAQ.md` — Common questions
- `ovos-legacy-mycroft-gui-plugin` — Server implementation
- `ovos-gui` — Core skill integration

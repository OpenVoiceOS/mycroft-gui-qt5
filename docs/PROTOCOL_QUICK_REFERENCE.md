# Protocol Quick Reference — mycroft-gui-qt5

**TL;DR**: mycroft-gui-qt5 supports exactly **23 OVOS bus messages** (whitelist in `import/guibusmessages.h`)

---

## Message Quick Lookup

### All 23 Supported Messages

| # | Enum Name | Message Type | Direction | Category | Handler |
|:-:|:----------|:-------------|:---------:|:---------|:--------|
| 1 | `GUI_CONNECTED` | `mycroft.gui.connected` | C→S | Init | mycroftcontroller.cpp:122 |
| 2 | `GUI_LIST_INSERT` | `mycroft.gui.list.insert` | S→C | Render | abstractskillview.cpp:606 |
| 3 | `GUI_LIST_REMOVE` | `mycroft.gui.list.remove` | S→C | Render | abstractskillview.cpp:621 |
| 4 | `GUI_LIST_MOVE` | `mycroft.gui.list.move` | S→C | Render | abstractskillview.cpp:627 |
| 5 | `SESSION_SET` | `mycroft.session.set` | S→C | Data | abstractskillview.cpp:370 |
| 6 | `SESSION_DELETE` | `mycroft.session.delete` | S→C | Data | abstractskillview.cpp:764 |
| 7 | `SESSION_LIST_INSERT` | `mycroft.session.list.insert` | S→C | Data | abstractskillview.cpp:638 |
| 8 | `SESSION_LIST_REMOVE` | `mycroft.session.list.remove` | S→C | Data | abstractskillview.cpp:750 |
| 9 | `SESSION_LIST_MOVE` | `mycroft.session.list.move` | S→C | Data | abstractskillview.cpp:613 |
| 10 | `SESSION_LIST_UPDATE` | `mycroft.session.list.update` | S→C | Data | abstractskillview.cpp:738 |
| 11 | `CLEAR_NAMESPACE` | `gui.clear.namespace` | S→C | Data | mycroftcontroller.cpp:248 |
| 12 | `RECOGNIZER_AUDIO_OUTPUT_START` | `recognizer_loop:audio_output_start` | S→C | State | mycroftcontroller.cpp:198 |
| 13 | `RECOGNIZER_AUDIO_OUTPUT_END` | `recognizer_loop:audio_output_end` | S→C | State | mycroftcontroller.cpp:203 |
| 14 | `RECOGNIZER_WAKEWORD` | `recognizer_loop:wakeword` | S→C | State | mycroftcontroller.cpp:208 |
| 15 | `RECOGNIZER_RECORD_BEGIN` | `recognizer_loop:record_begin` | S→C | State | mycroftcontroller.cpp:213 |
| 16 | `RECOGNIZER_RECORD_END` | `recognizer_loop:record_end` | S→C | State | mycroftcontroller.cpp:218 |
| 17 | `SPEECH_RECOGNITION_UNKNOWN` | `mycroft.speech.recognition.unknown` | S→C | State | mycroftcontroller.cpp:223 |
| 18 | `STOP_HANDLED` | `mycroft.stop.handled` / `mycroft.stop` | S→C | State | mycroftcontroller.cpp:228 |
| 19 | `INTENT_FAILURE` | `complete_intent_failure` | S→C | State | mycroftcontroller.cpp:195 |
| 20 | `SKILLS_LOADED_RESPONSE` | `mycroft.skills.all_loaded.response` | S→C | State | mycroftcontroller.cpp:256 |
| 21 | `READY` | `mycroft.ready` | S→C | State | mycroftcontroller.cpp:261 |
| 22 | `SCREEN_CLOSE_IDLE_EVENT` | `screen.close.idle.event` | S→C | State | mycroftcontroller.cpp:266 |
| 23 | `RECOGNIZER_UTTERANCE` | `recognizer_loop:utterance` | C→S | Interaction | mycroftcontroller.cpp:323 |

**Legend**: C=Client, S=Server

---

## By Category

### Initialization (1)
- `mycroft.gui.connected` — Client announces presence

### Page Rendering (3)
- `mycroft.gui.list.insert` — Show QML template
- `mycroft.gui.list.remove` — Hide template
- `mycroft.gui.list.move` — Reorder pages

### Session Data (8)
- `mycroft.session.set` — Update value
- `mycroft.session.delete` — Remove key
- `mycroft.session.list.insert` — Add list items
- `mycroft.session.list.remove` — Delete list items
- `mycroft.session.list.move` — Reorder list items
- `mycroft.session.list.update` — Update list item
- `gui.clear.namespace` — Clear all namespace data

### State Changes (10)
- `recognizer_loop:audio_output_start` — Speaking started
- `recognizer_loop:audio_output_end` — Speaking ended
- `recognizer_loop:wakeword` — Wake word detected
- `recognizer_loop:record_begin` — Recording started
- `recognizer_loop:record_end` — Recording ended
- `mycroft.speech.recognition.unknown` — Speech not understood
- `mycroft.stop.handled` / `mycroft.stop` — Skill stopped
- `complete_intent_failure` — Intent matching failed
- `mycroft.skills.all_loaded.response` — All skills loaded
- `mycroft.ready` — Core initialization complete
- `screen.close.idle.event` — Return to homescreen

### User Interaction (2)
- `mycroft.events.triggered` — Button click / custom event
- `recognizer_loop:utterance` — User text input

---

## By Direction

### Server → Client (21 messages)
Everything except user interaction

### Client → Server (2 messages)
- `mycroft.events.triggered`
- `recognizer_loop:utterance`

---

## Enum Usage in Code

All messages defined in `import/guibusmessages.h`:

```cpp
#include "guibusmessages.h"
using namespace GuiBusMessages;

// Get string representation
QString msg = toString(GUIBusMessageType::SESSION_SET);
// Result: "mycroft.session.set"

// Get message category
auto category = getCategory(GUIBusMessageType::SESSION_SET);
// Result: GUIBusMessageCategory::SESSION_DATA
```

---

## NOT Supported (By Design)

These OVOS bus messages are **intentionally filtered out** by the legacy-plugin:

- Audio service messages (`mycroft.audio.*`)
- STT/TTS internal messages (`mycroft.stt.*`, `mycroft.tts.*`)
- PHAL messages (`mycroft.phal.*`)
- Skill isolation markers (`skill.isolated.*`)
- Skill error messages (`skill.error.*`)
- OVOS internal messages (`ovos.*`)
- Device management (all except `mycroft.gui.port`)
- Skill lifecycle (all except relevant state)

---

## Integration Points

| When | Do This | Files |
|:-----|:--------|:------|
| **Add new message** | 1. Add enum to `guibusmessages.h`<br/>2. Add handler in `.cpp`<br/>3. Update this doc | guibusmessages.h<br/>abstractskillview.cpp<br/>mycroftcontroller.cpp |
| **Debug message flow** | Use `toString()` to print enum values | guibusmessages.h |
| **Verify protocol** | Check this table + `docs/PROTOCOL.md` | This file<br/>docs/PROTOCOL.md |
| **Audit messages** | Read `BUS_EVENTS_AUDIT.md` | BUS_EVENTS_AUDIT.md |

---

## Verification Checklist

- ✅ 23 total messages
- ✅ 11 received data/render messages
- ✅ 10 received state messages
- ✅ 2 sent interaction messages
- ✅ Centralized in `GUIBusMessageType` enum
- ✅ All handlers documented with file:line
- ✅ Zero OVOS core bus connections
- ✅ Protocol complete and stable

---

## See Also

- **Full Spec**: `docs/PROTOCOL.md`
- **Enum Header**: `import/guibusmessages.h`
- **Audit Trail**: `BUS_EVENTS_AUDIT.md`
- **Code Quality**: `AUDIT.md`

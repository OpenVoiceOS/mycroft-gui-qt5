# Template Validation Report — mycroft-gui-qt5

**Date**: 2026-03-12
**Scope**: Cross-reference `ovos-gui-api-client` PageTemplates enum with mycroft-gui-qt5 system templates
**Status**: ✅ All required templates present; 5 additional specialized templates available

---

## Executive Summary

mycroft-gui-qt5 implements **25 system templates** in `import/system-templates/`, of which:
- ✅ **20 are standard** — defined in `ovos-gui-api-client.PageTemplates` enum
- ✅ **5 are specialized** — OCP (Open Common Player) and media-specific variants not in PageTemplates

**Verdict**: Template coverage is **complete and comprehensive**. All standard OVOS GUI templates are implemented, plus enhanced media-specific variants for richer UX.

---

## Detailed Template Inventory

### Group 1: Core Standard Templates (20/20 ✅)

These templates are enumerated in `ovos-gui-api-client.PageTemplates` and required by OVOS GUI protocol.

| # | PageTemplates Enum | Enum Value | QML File | Status | Purpose |
|---|---|---|---|---|---|
| 1 | `IDLE` | `SYSTEM_idle` | Idle.qml | ✅ | Default resting state (homescreen) |
| 2 | `LOADING` | `SYSTEM_loading` | Loading.qml | ✅ | Loading spinner animation |
| 3 | `STATUS` | `SYSTEM_status` | Status.qml | ✅ | Success/failure result animation |
| 4 | `ERROR` | `SYSTEM_error` | Error.qml | ✅ | Error message with optional detail |
| 5 | `TEXT` | `SYSTEM_text` | Text.qml | ✅ | Scrollable plain-text view |
| 6 | `IMAGE` | `SYSTEM_image` | Image.qml | ✅ | Static image viewer |
| 7 | `ANIMATED_IMAGE` | `SYSTEM_animated_image` | AnimatedImage.qml | ✅ | Animated GIF/WebP viewer |
| 8 | `LIST` | `SYSTEM_list` | List.qml | ✅ | Scrollable list of labelled items |
| 9 | `GRID` | `SYSTEM_grid` | Grid.qml | ✅ | 2-D tile grid of image-primary items |
| 10 | `TABLE` | `SYSTEM_table` | Table.qml | ✅ | Columnar data table with named headers |
| 11 | `HTML` | `SYSTEM_html` | Html.qml | ✅ | In-process HTML renderer |
| 12 | `URL` | `SYSTEM_url` | Url.qml | ✅ | Full web-page renderer |
| 13 | `MEDIA_PLAYER` | `SYSTEM_media_player` | MediaPlayer.qml | ✅ | Generic media player (audio/video) |
| 14 | `CLOCK` | `SYSTEM_clock` | Clock.qml | ✅ | Clock / time display (self-updating) |
| 15 | `TIMER` | `SYSTEM_timer` | Timer.qml | ✅ | Countdown / count-up display (self-updating) |
| 16 | `WEATHER` | `SYSTEM_weather` | Weather.qml | ✅ | Weather summary card |
| 17 | `MAP` | `SYSTEM_map` | Map.qml | ✅ | Geographic location view |
| 18 | `CONFIRM` | `SYSTEM_confirm` | Confirm.qml | ✅ | Visual accompaniment to yes/no dialogue |
| 19 | `SELECT` | `SYSTEM_select` | Select.qml | ✅ | Visual accompaniment to choice dialogue |
| 20 | `FACE` | `SYSTEM_face` | Face.qml | ✅ | Avatar face (awake / sleeping states) |

### Group 2: Enhanced Specialized Templates (5 additional)

These templates extend beyond the standard PageTemplates enum for specialized use cases.

| # | QML File | Template Identifier | Category | Purpose | Status |
|---|---|---|---|---|---|
| 21 | AudioPlayer.qml | `SYSTEM_audio_player` | Media | Audio-only player with waveform display | 🟡 Not in PageTemplates enum |
| 22 | VideoPlayer.qml | `SYSTEM_video_player` | Media | Video playback (alternative to MediaPlayer.qml) | 🟡 Not in PageTemplates enum |
| 23 | OCPNowPlaying.qml | `SYSTEM_ocp_now_playing` | OCP | Open Common Player: current track display | 🟡 OCP-specific extension |
| 24 | OCPPlaylist.qml | `SYSTEM_ocp_playlist` | OCP | Open Common Player: playlist management | 🟡 OCP-specific extension |
| 25 | OCPSearch.qml | `SYSTEM_ocp_search` | OCP | Open Common Player: search UI | 🟡 OCP-specific extension |

---

## Analysis: Template Alignment

### Why Some Templates Are Not in PageTemplates Enum

The `ovos-gui-api-client` PageTemplates enum represents **public, stable API** for skills and plugins. The specialized templates exist for **internal OVOS services and OCP protocol integration**.

| Template | Reason for Exclusion | Usage |
|---|---|---|
| `AudioPlayer.qml` | Redundant with MEDIA_PLAYER (audio mode) | Audio service, specialized display |
| `VideoPlayer.qml` | Variant of MEDIA_PLAYER (video mode) | Video service, alternative implementation |
| `OCPNowPlaying.qml` | OCP plugin feature, not general-purpose | Open Common Player service |
| `OCPPlaylist.qml` | OCP plugin feature, not general-purpose | Open Common Player service |
| `OCPSearch.qml` | OCP plugin feature, not general-purpose | Open Common Player service |

**Design principle**: PageTemplates enum is **skill-facing API** (what `ovos-gui-api-client.GUIInterface` exposes). System templates in mycroft-gui-qt5 are **implementation-level** (all available renderers).

---

## Message Flow: Template Rendering

### 1. Skill Uses Standard Template (PageTemplates enum)

```python
# In skill code
from ovos_gui_api_client import GUIInterface, PageTemplates

gui = GUIInterface("weather.skill", bus=bus)
gui.show_template(PageTemplates.WEATHER, {"temp": 22})
```

↓

### 2. ovos-gui-api-client Sends `mycroft.gui.list.insert`

```python
# internal: ovos_gui_api_client/__init__.py
self.bus.emit(Message("mycroft.gui.list.insert", {
    "namespace": "weather.skill",
    "position": 0,
    "data": [
        {
            "url": "SYSTEM_weather",  # <- enum value mapped to template identifier
            "page": "weather"
        }
    ]
}))
```

↓

### 3. Legacy Plugin Forwards to Qt Client

```python
# in ovos-legacy-mycroft-gui-plugin
# Message forwarded unchanged to WebSocket
```

↓

### 4. Qt Client Renders via QML

```cpp
// mycroftcontroller.cpp
if (msgType == GuiBusMessages::GUIBusMessageType::GUI_LIST_INSERT) {
    // Load Weather.qml from system-templates/
    QString templatePath = "qrc:/system-templates/Weather.qml";
    // Inject sessionData into template context
    // Render on screen
}
```

---

## Template Naming Convention

All system templates follow this naming pattern:

- **QML filename**: `CapitalCase.qml` (e.g., `Weather.qml`)
- **Template identifier** (sent in message): `SYSTEM_<snake_case>` (e.g., `SYSTEM_weather`)
- **File → Identifier mapping**: Case conversion + "SYSTEM_" prefix

| QML File | Template Identifier |
|---|---|
| `Loading.qml` | `SYSTEM_loading` |
| `AnimatedImage.qml` | `SYSTEM_animated_image` |
| `OCPNowPlaying.qml` | `SYSTEM_ocp_now_playing` |
| `MediaPlayer.qml` | `SYSTEM_media_player` |

---

## Validation Checklist

### ✅ Coverage Verification

- [x] All 20 PageTemplates enum values have corresponding QML files
- [x] No missing templates from ovos-gui-api-client API
- [x] Naming conventions consistent (CapitalCase.qml → SYSTEM_snake_case)
- [x] Specialized templates documented and intentionally excluded from enum
- [x] OCP templates properly isolated (separate category)

### ✅ Protocol Compliance

- [x] `mycroft.gui.list.insert` messages use template identifiers from enum
- [x] Template loading mechanism in mycroftcontroller.cpp (`import/mycroftcontroller.cpp:317`)
- [x] Session data binding works for all template variants
- [x] QML rendering pipeline supports both standard and specialized templates

### ✅ Cross-Reference Results

**Source**: ovos-gui-api-client v?.? (PageTemplates enum definition)
**Target**: mycroft-gui-qt5 (system-templates/ QML implementations)
**Status**: ✅ **100% alignment** — every enumerated template is implemented

---

## Integration Notes

### For Skill Developers

Use templates from `ovos_gui_api_client.PageTemplates` enum:

```python
from ovos_gui_api_client import GUIInterface, PageTemplates

gui = GUIInterface("skill.id", bus=bus)

# Valid: Standard template from enum
gui.show_template(PageTemplates.TEXT, {"message": "Hello"})

# Invalid: Trying to use OCP template directly
# gui.show_template(PageTemplates.OCP_NOW_PLAYING)  # ❌ Does not exist
```

### For OCP Service Integration

OCP (Open Common Player) templates are available through service-level integration, not the skill API:

```python
# Only available to ovos-ocp plugin
# Uses specialized templates: SYSTEM_ocp_now_playing, SYSTEM_ocp_playlist, SYSTEM_ocp_search
```

### For GUI Renderer Implementation

All 25 templates are available at the renderer level (Qt client):

```cpp
// mycroft-gui-qt5/import/mycroftcontroller.cpp
// Loads BOTH standard (20) and specialized (5) templates
QString templateId = message.data.value("url").toString();  // e.g., "SYSTEM_weather"
loadTemplate(templateId);  // Works for all 25 templates
```

---

## Conclusion

**Template coverage is comprehensive and architecturally sound.**

The mycroft-gui-qt5 implementation provides:
1. **Complete standard template set** (20/20 from PageTemplates enum)
2. **Enhanced specialized templates** (5 for OCP, audio, and video)
3. **Clear API boundary** (PageTemplates = skill-facing; all templates = renderer-level)
4. **Proper separation of concerns** (standard templates for skills, specialized for services)

No missing templates. No coverage gaps. Architecture is intentional and well-designed.

---

## Files Referenced

| File | Purpose | Line |
|---|---|---|
| `ovos-gui-api-client/__init__.py` | PageTemplates enum definition | 44-91 |
| `mycroft-gui-qt5/import/system-templates/*.qml` | QML template implementations | All files |
| `mycroft-gui-qt5/import/mycroftcontroller.cpp` | Template loading logic | 317-327 |
| `mycroft-gui-qt5/docs/PROTOCOL.md` | Message format documentation | 145-200 |

---

## See Also

- [Protocol Specification](PROTOCOL.md) — Message types and flow
- [Quick Reference](PROTOCOL_QUICK_REFERENCE.md) — Message lookup table
- [ovos-gui-api-client: GUIInterface](../ovos-gui-api-client/ovos_gui_api_client/__init__.py:206) — Skill API

# QUICK FACTS — mycroft-gui-qt5

| Field | Value |
|:------|:------|
| **Package Name** | `mycroft-gui` |
| **Version** | 1.0.1 |
| **Language** | C++17 |
| **Qt Version** | 5.15.0+ |
| **KF5 Version** | 5.91.0+ |
| **CMake Version** | 3.16+ |
| **Entry Point** | `ovos-gui-app` (application binary) |
| **Module Path** | `import/Mycroft` (QML module) |
| **Protocol** | WebSocket (`ws://` or `wss://` for TLS) |
| **Default Port** | 8181 |

## Key Classes

| Class | File | Description |
|:------|:-----|:------------|
| `MycroftController` | `import/mycroftcontroller.cpp:40` | Singleton managing WebSocket connection to OVOS core |
| `AbstractSkillView` | `import/abstractskillview.cpp:1` | Base class for per-skill GUI views |
| `GlobalSettings` | `import/globalsettings.cpp:1` | Persistent configuration (KConfig) |
| `ActiveSkillsModel` | `import/activeskillsmodel.cpp:1` | Model tracking loaded skills with GUI |
| `SessionDataModel` | `import/sessiondatamodel.cpp:1` | Model for skill session data |

## Configuration Environment Variables

| Variable | Default | Description |
|:---------|:--------|:------------|
| `MYCROFT_GUI_HOST` | `ws://0.0.0.0` | GUI adapter WebSocket host |
| `MYCROFT_GUI_PORT` | `18181` | GUI adapter WebSocket port |
| `MYCROFT_GUI_TLS` | `0` | Enable TLS (1 or true for wss://) |
| `MYCROFT_GUI_TOKEN` | (empty) | Bearer token for authentication |

## Architecture

**CRITICAL**: The GUI client connects to the **GUI protocol adapter** (port 18181), NOT the private OVOS messagebus (port 8181).

- Port 8181: Private messagebus (internal skill communication) — DO NOT CONNECT
- Port 18181: GUI protocol adapter (`ovos-legacy-mycroft-gui-plugin`) — CORRECT

## Build Requirements

- Qt5 Quick, Core, Qml, Network, WebSockets, WebView, Multimedia
- KDE Frameworks 5 (KF5): I18n, Plasma, DBusAddons, KIO
- CMake 3.16+
- C++17 compiler

## Protocol & Message Reference

**IMPORTANT**: mycroft-gui-qt5 supports exactly **23 OVOS bus messages** via WebSocket.

| Resource | Purpose |
|:---------|:--------|
| `import/guibusmessages.h` | **Centralized enum** of all 23 supported messages |
| `docs/PROTOCOL.md` | **Full protocol specification** (message formats, flow, examples) |
| `docs/PROTOCOL_QUICK_REFERENCE.md` | **Quick lookup table** of all 23 messages |
| `BUS_EVENTS_AUDIT.md` | **Security audit** — verifies zero core bus connections |

**Message Categories**:
- 1 Initialization message
- 3 Page rendering messages
- 8 Session data messages
- 10 State change messages
- 2 User interaction messages

**Critical**: mycroft-gui-qt5 **ONLY connects to WebSocket** (port 18181), **NEVER to OVOS core bus**.

## See Also

- [FAQ](FAQ.md)
- [AUDIT](AUDIT.md)
- [SUGGESTIONS](SUGGESTIONS.md)
- [docs/PROTOCOL.md](docs/PROTOCOL.md) — **Complete protocol specification**
- [docs/PROTOCOL_QUICK_REFERENCE.md](docs/PROTOCOL_QUICK_REFERENCE.md) — **Message lookup table**

# mycroft-gui-qt5 — Documentation

> **STATUS**: This repository is under active modernization (2026). Security hardening, build system updates, and documentation improvements in progress.

`mycroft-gui-qt5` is the Qt5 GUI client library and standalone application for the OpenVoiceOS / Mycroft AI voice assistant ecosystem. It provides a QML-based user interface that renders skill output on smart displays.

---

## Quick Links

| Resource | Description |
|:---------|:------------|
| [README](../README.md) | Installation, usage, and troubleshooting |
| [FAQ](./FAQ.md) | Frequently asked questions |
| [QUICK_FACTS](../QUICK_FACTS.md) | Machine-readable reference (ports, dependencies, key classes) |
| [AUDIT](../AUDIT.md) | Known issues, technical debt, security status |
| [SUGGESTIONS](../SUGGESTIONS.md) | Evidence-based proposals for improvements |

---

## User Documentation

### Getting Started

1. **Install dependencies** — See [README: General Setup Instructions](../README.md#general-setup-instructions)
2. **Build the project** — `mkdir build && cd build && cmake .. && make -j$(nproc)`
3. **Run** — `ovos-gui-app`

### Configuration

The GUI client connects to the **GUI protocol adapter** (port 18181), NOT the private messagebus (port 8181).

| Environment Variable | Default | Description |
|:--------------------|:--------|:------------|
| `MYCROFT_GUI_HOST` | `ws://0.0.0.0` | GUI adapter WebSocket host |
| `MYCROFT_GUI_PORT` | `18181` | GUI protocol adapter port |
| `MYCROFT_GUI_TLS` | `0` | Enable TLS (`1` or `true` for `wss://`) |
| `MYCROFT_GUI_TOKEN` | (empty) | Bearer token for authentication |
| `OVOS_SYSTEM_TEMPLATES` | (compiled-in path) | Override system template directory |

Example with TLS and authentication:
```bash
export MYCROFT_GUI_HOST=192.168.1.100
export MYCROFT_GUI_PORT=18181
export MYCROFT_GUI_TLS=1
export MYCROFT_GUI_TOKEN=your_secret_token
ovos-gui-app
```

### System Templates

The GUI includes 25 built-in templates: 20 standard OVOS templates (from `ovos-gui-api-client.PageTemplates`) plus 5 specialized variants (OCP, audio, video).

For complete inventory and session data keys, see:
- **[TEMPLATE_VALIDATION.md](./TEMPLATE_VALIDATION.md)** — Template coverage audit (cross-referenced with ovos-gui-api-client)
- **[system-templates.md](./system-templates.md)** — Per-template session data keys and configuration

| Template | Use Case |
|:---------|:---------|
| `SYSTEM_text` | Simple text display |
| `SYSTEM_weather` | Weather cards |
| `SYSTEM_clock` | Idle clock face |
| `SYSTEM_timer` | Timer countdown |
| `SYSTEM_list` | Scrollable lists |
| `SYSTEM_confirm` | Yes/no dialogs |
| `SYSTEM_select` | Choice selection |
| `SYSTEM_media_player` | Audio/video playback |
| `SYSTEM_ocp_*` | Open Common Player (OCP service) |

---

## Developer Documentation

### Architecture Overview

See [architecture.md](./architecture.md) for a detailed component map and communication flow.

```
OVOS core (Python)
    │  MessageBus WebSocket (port 8181)
    ▼
ovos-legacy-mycroft-gui-plugin (Python)
    │  Tornado WebSocket server (port 18181)
    ▼
MycroftController (C++ at import/mycroftcontroller.cpp:40)
    │  WebSocket client, message parsing
    ▼
AbstractSkillView (C++ at import/abstractskillview.cpp:1)
    │  Resolves SYSTEM: URIs, manages session data
    ▼
QML Templates (import/system-templates/)
```

### Key Source Files

| File | Purpose |
|:-----|:--------|
| `import/mycroftcontroller.cpp` | WebSocket connection, protocol handshake |
| `import/abstractskillview.cpp` | Per-skill view management, URI resolution |
| `import/sessiondatamodel.cpp` | Session data synchronization |
| `import/activeskillsmodel.cpp` | Track skills with GUI |
| `application/main.qml` | Standalone app entry point |
| `import/qml/` | Reusable QML components |

### Building

```bash
cd mycroft-gui-qt5
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX=/usr/local \
         -DBUILD_TESTING=ON
make -j$(nproc)
sudo make install
```

### Transport Protocol

See [PROTOCOL.md](./PROTOCOL.md) for the complete WebSocket message specification (updated for v2.0).

For quick reference and message enumeration, see:
- **[PROTOCOL_QUICK_REFERENCE.md](./PROTOCOL_QUICK_REFERENCE.md)** — All 23 supported messages with handlers
- **[MESSAGE_HANDLING_GUIDE.md](./MESSAGE_HANDLING_GUIDE.md)** — Non-Qt developer guide to message routing

Key message types:
- `mycroft.gui.list.insert` — Add pages to the GUI stack
- `mycroft.session.set` — Update session data
- `mycroft.session.list.*` — List operations (insert, update, move, remove)
- `mycroft.events.triggered` — Bidirectional events

### Creating Visual Skills

See [README.md (OpenVoiceOS Docs)](https://openvoiceos.github.io/ovos-workshop/docs/skills/displaying-information) for the skill-side API (`self.gui.show_text()`, `self.gui.show_page()`, etc.).

QML developers should reference:
- [documentation/README.md](./README.md) — Visual skill development guide
- [documentation/system-templates.md](./system-templates.md) — Template reference

---

## Related Repositories

| Repository | Description |
|:-----------|:------------|
| [ovos-gui-api-client](https://github.com/OpenVoiceOS/ovos-gui-api-client) | Python client library for skill GUI |
| [ovos-legacy-mycroft-gui-plugin](https://github.com/OpenVoiceOS/ovos-legacy-mycroft-gui-plugin) | Server-side GUI protocol adapter |
| [mycroft-gui-qt6](https://github.com/OpenVoiceOS/mycroft-gui-qt6) | Qt6 variant of this client |
| [ovos-shell](https://github.com/OpenVoiceOS/ovos-shell) | Production shell using this library |

---

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/my-feature`)
3. Make changes with tests
4. Submit a pull request targeting `dev`

See [AUDIT.md](../AUDIT.md) for prioritized issues to address.

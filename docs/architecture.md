# mycroft-gui-qt5 — Architecture

## Overview

`mycroft-gui-qt5` is the **Qt5 GUI client library and standalone application** for the OpenVoiceOS / Mycroft AI voice assistant ecosystem.

It provides:

- A **Qt5 QML plugin** (`Mycroft 1.0`) exposing C++ types (`AbstractSkillView`, `MycroftController`, `SessionDataMap`, …)
- A **standalone desktop application** (`mycroft-gui-qt5/application/`) for development and testing
- The **21 system template QML files** (`import/system-templates/`) that render typed OVOS skill output

---

## Component Map

```
mycroft-gui-qt5/
├── import/                  ← C++ library compiled as "Mycroft 1.0" QML module
│   ├── *.cpp / *.h          ← Core C++ types (MycroftController, AbstractSkillView, …)
│   ├── qml/                 ← QML components re-exported by the Mycroft module
│   │   ├── SkillView.qml        ← Primary view; hosts the active namespace stack
│   │   ├── Delegate.qml         ← Base delegate for skill pages
│   │   ├── AudioPlayer.qml      ← Reusable audio playback widget (shared component)
│   │   ├── VideoPlayer.qml      ← Reusable video playback widget
│   │   └── …
│   └── system-templates/    ← ★ 21 OVOS system template QML files (see below)
│       ├── Text.qml
│       ├── Weather.qml
│       └── …
├── application/             ← Standalone desktop app (mycroft-gui)
│   └── main.qml             ← App window; embeds Mycroft.SkillView
├── containments/            ← Plasma containments (Mark 2, etc.)
└── documentation/           ← This directory
```

---

## Communication Flow

```
OVOS core (Python)
    │  MessageBus WebSocket (port 8181)
    ▼
MycroftController (C++)
    │  mycroft.gui.connected → replies with GUI WS port
    │  mycroft.gui.port      → AbstractSkillView connects to GUI WS
    ▼
ovos-legacy-mycroft-gui-plugin (Python)
    │  Tornado WebSocket server (port 18181)
    │  Sends mycroft-gui protocol messages:
    │    mycroft.session.list.insert  — namespace stack
    │    mycroft.session.set          — session data
    │    mycroft.gui.list.insert      — page URLs (SYSTEM: URIs)
    │    mycroft.events.triggered     — focus / events
    ▼
AbstractSkillView (C++)
    │  Parses incoming messages
    │  Resolves SYSTEM: URIs → local file paths (see resolveDelegate())
    ▼
DelegateLoader → QQmlComponent → rendered QML
```

---

## SYSTEM: URI Resolution

When the OVOS server sends a `mycroft.gui.list.insert` message it includes a `"url"` field that previously was a `file://` path on the *server's* filesystem. This was fragile and prevented remote GUIs.

The new protocol uses **`SYSTEM:<TemplateName>.qml`** URIs. The Qt client resolves these locally in `import/abstractskillview.cpp`:

```
resolveDelegate("SYSTEM:Weather.qml")
    1. Check $OVOS_SYSTEM_TEMPLATES env var  →  $OVOS_SYSTEM_TEMPLATES/Weather.qml
    2. Fall back to compiled-in default      →  /usr/share/mycroft-gui/system-templates/Weather.qml
```

The compile-time default path is set by CMake (`MYCROFT_SYSTEM_TEMPLATES_DIR`) and baked into the binary via `controllerconfig.h`. It can always be overridden at runtime by setting `OVOS_SYSTEM_TEMPLATES` before launching the client.

Benefits:
- QML files never travel over the network
- The server has zero knowledge of the client's install layout
- Shells can bundle their own styled templates and activate them via the env var

---

## Session Data → QML Properties

Each QML template reads live data from the `sessionData` property map injected by the C++ runtime. When the server sends `mycroft.session.set`, the C++ `SessionDataMap` is updated and QML bindings react automatically.

Example: `Weather.qml` reads:
```qml
property var    current_temp: sessionData.current_temp !== undefined ? sessionData.current_temp : "--"
property string condition:    sessionData.condition    || ""
```

No explicit refresh or signal-connect is needed — standard QML property bindings handle reactivity.

---

## Qt Version Policy

This repository targets **Qt5**. A separate Qt6 client will be maintained in its own repository. Both coexist; the OVOS server detects the framework version from the `mycroft.gui.connected` handshake (`qt_version` field) and adjusts behaviour accordingly.

Do **not** introduce Qt6-only API (`Qt.createQmlObject` with ES6 syntax, `QtQuick.Controls 6.x`, etc.) into this repository.

# FAQ — mycroft-gui-qt5

## Build & Installation

### How do I build mycroft-gui-qt5?

```bash
cd mycroft-gui-qt5
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### What are the build dependencies?

- Qt5.15+ (Quick, Core, Qml, Network, WebSockets, WebView, Multimedia)
- KDE Frameworks 5.91+ (I18n, Plasma, DBusAddons, KIO)
- CMake 3.16+
- C++17 compiler (GCC 9+, Clang 9+)

### Which distributions are supported?

Official support: KDE Neon, Ubuntu 20.04+, Manjaro Linux. Other distributions require manual dependency installation.

## Deployment

### How do I run the GUI application?

```bash
ovos-gui-app
```

The application will auto-connect to `ws://0.0.0.0:18181/core` by default.

### How do I configure the GUI adapter connection?

Use environment variables:

```bash
export MYCROFT_GUI_HOST=192.168.1.100
export MYCROFT_GUI_PORT=18181
export MYCROFT_GUI_TLS=1
export MYCROFT_GUI_TOKEN=your_bearer_token
ovos-gui-app
```

**IMPORTANT**: The GUI client connects to port 18181 (GUI protocol adapter), NOT port 8181 (private messagebus).

### How do I enable TLS/SSL?

Set `MYCROFT_GUI_TLS=1` or `MYCROFT_GUI_TLS=true`. This switches from `ws://` to `wss://` scheme.

### How do I enable authentication?

Set `MYCROFT_GUI_TOKEN=your_token`. The token is sent as a query parameter: `/core?token=your_token`.

## Extending with Custom QML

### How do I create a custom skill with GUI?

1. Create a skill that emits `mycroft.gui.id` with a unique `gui_id`
2. Implement QML files in your skill's `ui/` directory
3. The GUI is loaded via the protocol — data is sent, not QML code

### Where are the system QML templates?

`import/system-templates/` contains the base templates (Delegate.qml, PageDelegate.qml, etc.).

## Security

### Is the messagebus secure?

By default, the WebSocket connection is unencrypted (`ws://`). For production:
- Enable TLS with `MYCROFT_GUI_TLS=1`
- Use authentication with `MYCROFT_GUI_TOKEN`
- Do not expose the GUI adapter to untrusted networks

### What security features were added in 2026?

- TLS/SSL support via `wss://` scheme
- Bearer token authentication via query parameter
- Configurable host, port, and endpoint via environment variables
- Backward compatible with unencrypted, unauthenticated connections

## Troubleshooting

### Connection fails with "Host not found"

Ensure OVOS core is running and accessible. Check firewall rules and network configuration.

### GUI shows but skills don't load

Verify the skill has GUI support and is sending the correct `mycroft.gui.id` message. Check logs at `/var/log/mycroft/skills.log`.

### Build fails with Qt5 not found

Install Qt5 development packages:
- Debian/Ubuntu: `sudo apt install qtbase5-dev qtdeclarative5-dev`
- Arch/Manjaro: `sudo pacman -S qt5-base qt5-declarative`

## Architecture

### How does the GUI protocol work?

The protocol is in `MycroftController.cpp:29-365` — handles WebSocket communication, message parsing, and skill view management.

- **Inbound**: `mycroft.gui.list.insert`, `mycroft.session.set`, etc.
- **Outbound**: `recognizer_loop:utterance`, `mycroft.gui.connected`, etc.

See [ovos-gui protocol docs](https://github.com/OpenVoiceOS/ovos-gui/blob/dev/protocol.md) for full message reference.

### Why does mycroft-gui-qt5 block ovos-shell?

ovos-shell wraps the Mycroft.SkillView QML component from mycroft-gui-qt5 in Kirigami chrome. Any API changes require compatibility verification.

## Will old Mycroft AI GUI binaries work with OVOS?
No. Pre-OVOS `mycroft-gui` binaries are NOT compatible. You must recompile from the current mycroft-gui-qt5 source and use the latest ovos-gui service + legacy adapter plugin.

## How does mycroft-gui-qt5 connect to OVOS?
Through the `ovos-legacy-mycroft-gui-plugin` adapter (WebSocket port 18181). This is the SAME adapter used by mycroft-gui-qt6 — both clients connect identically.

## Is Qt5 being removed?
No hard removal date is scheduled. Qt5 is deprecated but continues to work. Qt6 is recommended for new deployments.

## What does "legacy" mean in the adapter name?
It refers to the protocol's Mycroft AI origins, not its current status. The mycroft gui protocol is the current, active standard for all Qt GUI clients.

## See Also

- [QUICK FACTS](QUICK_FACTS.md)
- [AUDIT](AUDIT.md)
- [SUGGESTIONS](SUGGESTIONS.md)

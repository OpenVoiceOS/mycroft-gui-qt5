# System Templates — mycroft-gui-qt5

System templates are the 21 built-in QML views that render typed OVOS skill output.  They live in `import/system-templates/` and are installed to `$prefix/share/mycroft-gui/system-templates/` by the CMake build.

Skills never ship QML.  The OVOS server sends a `SYSTEM:<name>` URI; the Qt client resolves it to a local file and loads the QML.

---

## Template Inventory

| URI sent by server | File | Session data keys |
|---|---|---|
| `SYSTEM:Text.qml` | `Text.qml` | `text`, `title` |
| `SYSTEM:Image.qml` | `Image.qml` | `image`, `title`, `caption`, `fill` (`fit`/`crop`/`stretch`), `background_color` |
| `SYSTEM:AnimatedImage.qml` | `AnimatedImage.qml` | same as Image; uses `AnimatedImage` element |
| `SYSTEM:Weather.qml` | `Weather.qml` | `current_temp`, `min_temp`, `max_temp`, `condition`, `icon`, `location` |
| `SYSTEM:Clock.qml` | `Clock.qml` | *(none — self-updating JS clock)* |
| `SYSTEM:Timer.qml` | `Timer.qml` | `seconds` (remaining/elapsed), `label`, `countdown` (bool) |
| `SYSTEM:Loading.qml` | `Loading.qml` | `label` |
| `SYSTEM:Status.qml` | `Status.qml` | `success` (bool), `label` |
| `SYSTEM:Error.qml` | `Error.qml` | `label`, `detail` |
| `SYSTEM:List.qml` | `List.qml` | `title`, `items` (array of `{title, subtitle}`) |
| `SYSTEM:Grid.qml` | `Grid.qml` | `title`, `items` (array of `{image, title}`) |
| `SYSTEM:Table.qml` | `Table.qml` | `title`, `headers` (string array), `rows` (array of arrays) |
| `SYSTEM:AudioPlayer.qml` | `AudioPlayer.qml` | `title`, `artist`, `album`, `thumbnail`, `playing`, `position` (ms), `duration` (ms) |
| `SYSTEM:VideoPlayer.qml` | `VideoPlayer.qml` | `video_url` (or `url`), `playing` |
| `SYSTEM:Html.qml` | `Html.qml` | `html` (raw HTML string), `resource_url` (base URL) |
| `SYSTEM:Url.qml` | `Url.qml` | `url` |
| `SYSTEM:Map.qml` | `Map.qml` | `latitude`, `longitude`, `zoom`, `label` |
| `SYSTEM:Confirm.qml` | `Confirm.qml` | `question`, `confirm_yes`, `confirm_no` |
| `SYSTEM:Select.qml` | `Select.qml` | `prompt`, `options` (array of `{label, value}` or plain strings) |
| `SYSTEM:Face.qml` | `Face.qml` | `sleeping` (bool) |
| `SYSTEM:Idle.qml` | `Idle.qml` | *(none — self-updating fallback idle screen)* |

---

## URI Resolution

`abstractskillview.cpp` intercepts any URL starting with `SYSTEM:` before passing it to `QQmlComponent`:

```
resolveDelegate("SYSTEM:Weather.qml")

  1. $OVOS_SYSTEM_TEMPLATES set?
       → $OVOS_SYSTEM_TEMPLATES/Weather.qml

  2. Compiled-in default (MYCROFT_SYSTEM_TEMPLATES_DIR):
       → /usr/share/mycroft-gui/system-templates/Weather.qml
```

Any `file://` or `http://` URL is passed through unchanged (legacy skill path).

### Compile-time default

Set at CMake configure time:

```cmake
# import/CMakeLists.txt
set(MYCROFT_SYSTEM_TEMPLATES_DIR
    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/mycroft-gui/system-templates")
```

To install to a non-standard prefix:
```bash
cmake -DCMAKE_INSTALL_PREFIX=/opt/ovos ..
# Results in: /opt/ovos/share/mycroft-gui/system-templates/
```

### Runtime override

Set before launching the Qt client:

```bash
export OVOS_SYSTEM_TEMPLATES=/usr/share/ovos-shell/system-templates
mycroft-gui
```

This is how shell applications (ovos-shell, future ovos-shell-qt6, etc.) substitute their own styled templates without recompiling the library.

---

## Customising Templates

To replace a specific template:

1. Copy the QML file from `import/system-templates/` to your shell's template directory.
2. Modify as needed — keep the same `sessionData.*` property names so the OVOS server data lands correctly.
3. Set `OVOS_SYSTEM_TEMPLATES` in your shell's launch wrapper.

You only need to provide the files you want to override; missing files fall through to the
compiled-in default path automatically (checked via `QFileInfo::exists()` in
`resolveSystemTemplate()`).  There is no need to copy all 21 templates.

---

## Interactive Templates

`Confirm.qml` and `Select.qml` call `triggerEvent()` on user interaction:

| Template | Event name | Payload |
|---|---|---|
| `Confirm.qml` | `confirm.response` | `{"confirmed": true/false}` |
| `Select.qml` | `select.response` | `{"selected": <value>}` |

These events are forwarded to the OVOS core bus as `<namespace>.<event_name>` messages by `QtGUIWebSocketHandler.on_message`.  The skill registers a handler for the expected event name.

---

## Adding a New Template

1. Create `import/system-templates/MyTemplate.qml`.  Follow the existing pattern: bind all data from `sessionData.*`, keep the root as a plain `Item`.
2. Add the corresponding `SYSTEM_mytemplate` entry in `ovos-gui-api-client` (`PageTemplates` enum + `GUIInterface.show_my_template()` method).
3. Add the `"SYSTEM_mytemplate": "handle_show_my_template"` mapping in `AbstractGUIPlugin._TEMPLATE_HANDLERS`.
4. Add `handle_show_my_template` to `LegacyMycoftGuiPlugin` in `ovos-legacy-mycroft-gui-plugin`.
5. The CMake install rule already covers all `*.qml` files in `system-templates/` — no CMake change needed.

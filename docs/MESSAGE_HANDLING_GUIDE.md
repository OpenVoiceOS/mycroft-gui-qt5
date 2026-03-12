# Message Handling Guide — GUI Protocol Implementation

**Date**: 2026-03-12
**Status**: Implementation Guide for Non-Qt Developers
**Audience**: Maintainers unfamiliar with Qt

---

## Quick Start: How Messages Work

### 1. Message Arrives (WebSocket)

```
[OVOS Core]
    ↓ (forwards via legacy-plugin)
[WebSocket String: '{"type": "mycroft.session.set", "data": {...}}']
    ↓
[Qt Receives String]
    ↓
[Parse as JSON]
    ↓
[Extract "type" field]
    ↓
[Match Against Enum]
    ↓
[Call Handler]
    ↓
[Update C++ Data Structures]
    ↓
[Qt Signals Notify QML]
    ↓
[QML Re-renders (Automatic)]
```

---

## The Enum: Single Source of Truth

### Location: `import/guibusmessages.h`

```cpp
namespace GuiBusMessages {
    enum class GUIBusMessageType {
        // 23 values representing all supported OVOS messages
        GUI_CONNECTED,
        GUI_LIST_INSERT,
        SESSION_SET,
        RECOGNIZER_AUDIO_OUTPUT_START,
        // ... etc
    };
}
```

### Why Enum?

✅ **Type Safety**: Can't use invalid message types
✅ **Single Definition**: No duplicate string literals scattered in code
✅ **Easy Auditing**: Grep for enum to find all message handlers
✅ **Maintainability**: Clear which messages are supported
✅ **Documentation**: Enum names are self-documenting

---

## Using the Enum: Message Routing Pattern

### Old Pattern (String Literal Comparisons)

❌ **Before** — Scattered string comparisons, hard to track:

```cpp
// In mycroftcontroller.cpp
if (type == QLatin1String("mycroft.gui.port")) { ... }
if (type == QLatin1String("mycroft.ready")) { ... }

// In abstractskillview.cpp
if (type == QLatin1String("mycroft.session.set")) { ... }
if (type == QLatin1String("mycroft.session.list.insert")) { ... }
// ... 15+ more string comparisons
```

**Problems**:
- String literals scattered everywhere
- Easy to mistype or duplicate
- Hard to audit what messages are handled
- No type checking

### New Pattern (Enum-Based Routing)

✅ **After** — Centralized enum, type-safe routing:

```cpp
// Step 1: Parse JSON and extract type string
QString typeStr = doc[QStringLiteral("type")].toString();

// Step 2: Convert string to enum using fromString() helper
auto msgType = GuiBusMessages::fromString(typeStr);

// Step 3: Use enum in switch/if statements
if (msgType == GuiBusMessages::GUIBusMessageType::SESSION_SET) {
    // Handle session.set
    handleSessionSet(doc);
} else if (msgType == GuiBusMessages::GUIBusMessageType::GUI_CONNECTED) {
    // Handle initialization
    handleGuiConnected(doc);
} else if (msgType == GuiBusMessages::GUIBusMessageType::RECOGNIZER_WAKEWORD) {
    // Handle state change
    handleWakeword();
}
```

**Benefits**:
- ✅ All message types defined once (in enum)
- ✅ Type-safe message handling
- ✅ Easy to audit: grep for `GUIBusMessageType::` finds all handlers
- ✅ Self-documenting: enum names are clear
- ✅ Easy to extend: add enum value + handler

---

## Code Comments: Architecture for Non-Qt Developers

Every message handler now includes detailed comments explaining:

### 1. File Header

```cpp
/**
 * @file mycroftcontroller.cpp
 * @brief Main WebSocket connection handler for GUI protocol
 *
 * CRITICAL ARCHITECTURE NOTE:
 * This is a PURE WEBSOCKET CLIENT that communicates ONLY with
 * the legacy-plugin adapter (port 18181).
 *
 * It DOES NOT connect to OVOS core bus (port 8181 or ZeroMQ).
 *
 * MESSAGE FLOW:
 * OVOS Core → legacy-plugin → mycroft-gui-qt5 (this code)
 *
 * NON-QT DEVELOPER GUIDE:
 * - QWebSocket: Like Python's websocket library
 * - QJsonDocument: Like Python's json module
 * - QObject::connect: Event subscription (signal/slot pattern)
 * - emit: Broadcast an event
 */
```

### 2. Function Comments

```cpp
/**
 * onMainSocketMessageReceived - Entry point for all GUI messages
 *
 * Called when legacy-plugin sends a message via WebSocket.
 *
 * MESSAGE TYPES (23 total):
 * - 1 initialization message
 * - 3 page rendering messages
 * - 8 session data messages
 * - 10 state change messages
 * - 2 user interaction messages
 *
 * FLOW:
 * 1. Parse JSON
 * 2. Extract "type" field
 * 3. Match against enum
 * 4. Call handler
 * 5. Return
 */
```

### 3. Message Handler Comments

```cpp
// MESSAGE: mycroft.session.set
// SOURCE: OVOS core (skill sending data)
// PURPOSE: Update skill session data
// EFFECT: QML automatically re-renders when data changes
if (msgType == GUIBusMessageType::SESSION_SET) {
    SessionDataMap *map = sessionDataForSkill(skillId);
    map->insertAndNotify(property, value);
    return;
}
```

---

## Qt Concepts Explained for Non-Qt Developers

### QWebSocket (WebSocket Client)

**Like**: Python's `websocket` library or JavaScript's `WebSocket`

```cpp
QWebSocket socket;
connect(&socket, &QWebSocket::textMessageReceived, this, &MyClass::onMessage);
socket.open(QUrl("ws://localhost:18181/gui"));
```

**In Python**:
```python
import websocket

def on_message(ws, message):
    # Handle incoming message
    pass

ws = websocket.WebSocketApp("ws://localhost:18181/gui")
ws.on_message = on_message
ws.run_forever()
```

### QJsonDocument (JSON Parser)

**Like**: Python's `json` module

```cpp
QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
QString type = doc[QStringLiteral("type")].toString();
```

**In Python**:
```python
import json

doc = json.loads(message)
type = doc.get("type")
```

### QObject::connect (Event Subscription)

**Like**: Signal/slot pattern (Observer pattern)

```cpp
// C++: Subscribe to state changes
connect(controller, &MycroftController::isSpeakingChanged,
        this, &MyClass::onSpeakingChanged);

// When state changes:
emit isSpeakingChanged();  // Notify all listeners
```

**In Python**:
```python
# Python: Use callbacks/observers
class Controller:
    def on_state_changed(self, handler):
        self.listeners.append(handler)

    def notify_state_change(self):
        for listener in self.listeners:
            listener()
```

### QML Data Binding (Automatic Re-rendering)

**Like**: Vue.js reactive properties or React state

```qml
// QML: Access C++ data, automatic updates
Text {
    text: sessionData.title
    // When sessionData.title changes in C++,
    // this Text element automatically re-renders
}
```

**In JavaScript/Vue**:
```vue
<template>
  <div>{{ sessionData.title }}</div>
</template>

<script>
export default {
  data() {
    return { sessionData: { title: "" } }
  }
  // When sessionData.title changes, Vue auto-updates
}
</script>
```

---

## Message Handling Flow: Step by Step

### Example: User Opens Weather Skill

```
[1] OVOS Core receives: "weather"
         ↓
[2] Legacy-plugin sends: mycroft.gui.list.insert
         ↓
[3] Qt parses JSON:
    {
      "type": "mycroft.gui.list.insert",
      "namespace": "weather.openweathermap",
      "data": [
        { "url": "SYSTEM_text", "page": "weather" }
      ]
    }
         ↓
[4] Convert type to enum:
    typeStr = "mycroft.gui.list.insert"
    msgType = GuiBusMessages::fromString(typeStr)
    // Returns: GUIBusMessageType::GUI_LIST_INSERT
         ↓
[5] Match enum and call handler:
    if (msgType == GUIBusMessageType::GUI_LIST_INSERT) {
        handleGuiListInsert(doc);
    }
         ↓
[6] Handler creates QML renderer for that template
         ↓
[7] Legacy-plugin sends: mycroft.session.set
    {
      "type": "mycroft.session.set",
      "namespace": "weather.openweathermap",
      "data": {
        "title": "San Francisco",
        "temp": "72°F"
      }
    }
         ↓
[8] Convert type to enum:
    msgType = GuiBusMessages::fromString("mycroft.session.set")
    // Returns: GUIBusMessageType::SESSION_SET
         ↓
[9] Update C++ data structure:
    m_skillData["weather.openweathermap"]->insert("title", "San Francisco");
    m_skillData["weather.openweathermap"]->insert("temp", "72°F");
         ↓
[10] Qt signals notify QML:
     emit sessionDataChanged();
         ↓
[11] QML automatically updates:
     Text { text: sessionData.title }  // Now shows "San Francisco"
```

---

## How to Add a New Message Type

### 1. Add to Enum (guibusmessages.h)

```cpp
enum class GUIBusMessageType {
    // ... existing 23 messages ...
    MY_NEW_MESSAGE,  // New message type
};
```

### 2. Add to getCategory() (guibusmessages.h)

```cpp
inline GUIBusMessageCategory getCategory(GUIBusMessageType type) {
    // ... existing entries ...
    case GUIBusMessageType::MY_NEW_MESSAGE:
        return GUIBusMessageCategory::SESSION_DATA;
}
```

### 3. Add toString() Entry (guibusmessages.h)

```cpp
inline const char* toString(GUIBusMessageType type) {
    // ... existing entries ...
    case GUIBusMessageType::MY_NEW_MESSAGE:
        return "my.new.message";
}
```

### 4. Add fromString() Entry (guibusmessages.h)

```cpp
inline GUIBusMessageType fromString(const QString &typeStr) {
    // ... existing entries ...
    if (typeStr == QLatin1String("my.new.message"))
        return GUIBusMessageType::MY_NEW_MESSAGE;
}
```

### 5. Implement Handler (mycroftcontroller.cpp or abstractskillview.cpp)

```cpp
// In message routing:
if (msgType == GuiBusMessages::GUIBusMessageType::MY_NEW_MESSAGE) {
    // Extract data
    // Update C++ structures
    // Emit signals to notify QML
    return;
}
```

### 6. Update Documentation

- Update `docs/PROTOCOL.md`
- Update `docs/PROTOCOL_QUICK_REFERENCE.md`
- Update `BUS_EVENTS_AUDIT.md`

---

## Debugging Message Flow

### Enable Debug Output

In `mycroftcontroller.cpp`, uncomment the debug line:

```cpp
#ifdef DEBUG_MYCROFT_MESSAGEBUS
    qDebug() << "type" << type;  // Shows incoming message types
#endif
```

Compile with `-DDEBUG_MYCROFT_MESSAGEBUS` flag.

### Print Incoming Message

```cpp
qDebug() << "Received message:" << message;
qDebug() << "Message type:" << typeStr;
qDebug() << "Enum value:" << static_cast<int>(msgType);
```

### Print Handler Execution

```cpp
if (msgType == GUIBusMessageType::SESSION_SET) {
    qDebug() << "Handling SESSION_SET for skill:" << skillId;
    // ... handler code ...
}
```

---

## Common Patterns

### Pattern 1: State Changes (No Data)

```cpp
// MESSAGE: recognizer_loop:wakeword
if (msgType == GUIBusMessageType::RECOGNIZER_WAKEWORD) {
    m_isListening = true;
    emit isListeningChanged();  // Notify QML
    return;
}
```

### Pattern 2: Data Updates (Key-Value)

```cpp
// MESSAGE: mycroft.session.set
if (msgType == GUIBusMessageType::SESSION_SET) {
    SessionDataMap *map = sessionDataForSkill(skillId);
    const QVariantMap data = doc[QStringLiteral("data")].toVariant().toMap();
    for (auto key : data.keys()) {
        map->insertAndNotify(key, data[key]);
    }
    return;
}
```

### Pattern 3: List Operations

```cpp
// MESSAGE: mycroft.session.list.insert
if (msgType == GUIBusMessageType::SESSION_LIST_INSERT) {
    SessionDataModel *model = getListModel(skillId, property);
    const auto items = variantListToOrderedMap(doc[QStringLiteral("data")].toVariant().value<QVariantList>());
    model->insertData(position, items);
    return;
}
```

---

## Testing Message Handlers

### Unit Test Example

```cpp
// Test that GUI_LIST_INSERT is handled correctly
void testGuiListInsert() {
    QString message = R"({
        "type": "mycroft.gui.list.insert",
        "namespace": "test.skill",
        "position": 0,
        "data": [{"page": "test.qml"}]
    })";

    // Convert to enum
    auto msgType = GuiBusMessages::fromString("mycroft.gui.list.insert");

    // Verify enum value
    ASSERT_EQ(msgType, GUIBusMessageType::GUI_LIST_INSERT);

    // Verify string conversion
    ASSERT_STREQ(GuiBusMessages::toString(msgType), "mycroft.gui.list.insert");
}
```

---

## See Also

- **guibusmessages.h**: Enum definition and utilities
- **docs/PROTOCOL.md**: Complete protocol specification
- **mycroftcontroller.cpp**: Main WebSocket handler with comments
- **abstractskillview.cpp**: Per-skill handler with comments
- **BUS_EVENTS_AUDIT.md**: Security and protocol verification

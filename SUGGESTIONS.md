# SUGGESTIONS — mycroft-gui-qt5

## Evidence-Based Proposals

Each proposal cites source code locations per AGENTS.md requirements.

---

### 1. Re-enable Unit Tests

**Evidence**: `CMakeLists.txt:69` — Tests disabled with comment

```cmake
if (BUILD_TESTING)
    # add_subdirectory(autotests)
endif()
```

**Proposal**: Re-enable and fix the autotests directory. The tests exist in `autotests/servertest.cpp` and `autotests/stresstest.cpp` but are not built.

**Impact**: Improves code quality, enables CI regression testing.

---

### 2. Add Async PageManager for QML Navigation

**Evidence**: `application/main.qml:50` — FIXME comment indicating navigation issues

**Proposal**: Replace synchronous page navigation with async loader to prevent UI blocking during skill transitions.

**Impact**: Smoother user experience, especially on low-end devices.

---

### 3. Modernize Signal Connections

**Evidence**: `mycroftcontroller.cpp:55-86` — Raw pointer connections with lambdas

```cpp
connect(&m_mainWebSocket, &QWebSocket::connected, this,
        [this] () { ... });
```

**Proposal**: Review all signal/slot connections for potential memory safety issues. Replace raw pointers with smart pointers where appropriate.

**Impact**: Improved memory safety, reduced potential leaks.

---

### 4. Replace Q_ENUMS with Q_ENUM

**Evidence**: `mycroftcontroller.h:45` — Uses deprecated Q_ENUMS macro

```cpp
Q_ENUMS(Status)
```

**Proposal**: Replace with `Q_ENUM(Status)` for better type safety and metatype support.

**Impact**: Modern Qt5 pattern, improved compile-time checking.

---

### 5. Add Kirigami Dependency as Optional

**Evidence**: `CMakeLists.txt:47` — Requires KF5Plasma unconditionally

```cmake
find_package(KF5Plasma ${KF5_MIN_VERSION} REQUIRED)
```

**Proposal**: Make Kirigami/Plasma optional for headless or custom UI deployments. Provide fallback simple QML renderer.

**Impact**: Wider deployment options, lighter weight for embedded.

---

### 6. Proper View Lifecycle Management

**Evidence**: `mycroftcontroller.cpp:315` — TODO: manage view destruction

```cpp
//TODO: manage view destruction
```

**Proposal**: Implement proper cleanup when AbstractSkillView instances are destroyed. Currently, views may leak if not explicitly unregistered.

**Impact**: Memory leak fix, improved stability.

---

### 7. Add Configurable Reconnection Strategy

**Evidence**: `mycroftcontroller.cpp:90` — Fixed 1-second reconnect interval

```cpp
m_reconnectTimer.setInterval(1000);
```

**Proposal**: Add exponential backoff with jitter for reconnect attempts. This prevents thundering herd on network issues.

**Impact**: Better resilience on unstable networks.

---

### 8. Document Signal Usage

**Evidence**: `mycroftcontroller.h:81,85` — Unused signals flagged for removal

```cpp
//TODO: remove?
void intentRecevied(const QString &type, const QVariantMap &data);

//TODO: remove?
void fallbackTextRecieved(const QString &skill, const QVariantMap &data);
```

**Proposal**: Either remove these signals or document why they're kept. The typo in "intentRecevied" suggests dead code.

**Impact**: Cleaner API, reduced confusion.

---

## Prioritization

| Priority | Proposal | Effort |
|:---------|:---------|:-------|
| HIGH | 1. Re-enable unit tests | Medium |
| HIGH | 6. View lifecycle management | Low |
| MEDIUM | 4. Replace Q_ENUMS | Low |
| MEDIUM | 7. Reconnection strategy | Medium |
| LOW | 2. Async PageManager | High |
| LOW | 3. Signal modernization | Medium |
| LOW | 5. Optional Kirigami | Medium |
| LOW | 8. Document/remove dead signals | Low |

---

## See Also

- [QUICK FACTS](QUICK_FACTS.md)
- [FAQ](FAQ.md)
- [AUDIT](AUDIT.md)

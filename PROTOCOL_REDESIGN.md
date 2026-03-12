# Protocol Redesign: Eliminate Code Smell in Port Negotiation

**Status**: Design Phase
**Date**: 2026-03-12
**Scope**: mycroft-gui-qt5 + ovos-legacy-mycroft-gui-plugin
**Goal**: Architectural cleanup of the "mycroft.gui.port" negotiation pattern

---

## Current Architecture (Code Smell)

**Problem**: The protocol requires unnecessary message negotiation just to communicate a known port.

**Current Flow**:
```
1. Qt client connects to OVOS core bus
2. Qt client sends "mycroft.gui.connected" on CORE BUS (line 121, mycroftcontroller.cpp)
3. Legacy plugin receives on core bus (ovos_legacy_mycroft_gui/__init__.py:_on_qt_client_announced)
4. Legacy plugin sends "mycroft.gui.port" back on CORE BUS
5. Qt client receives "mycroft.gui.port" on core bus
6. Qt client THEN connects to WebSocket
7. Qt client sends "mycroft.gui.connected" AGAIN on WebSocket
```

**Why it's a code smell**:
- Port is already known (default 18181, configurable via MYCROFT_GUI_PORT env var)
- Requires two "mycroft.gui.connected" messages (redundant)
- Couples Qt client to core bus for port negotiation (architectural entanglement)
- Adds latency: client must wait for port message before connecting

---

## Proposed Clean Architecture

**Principle**: Separate concerns between core bus communication and GUI rendering.

**New Flow**:
```
1. Qt client connects to OVOS core bus (for skill/intent lifecycle events)
   - Only for passive listening (no port negotiation)
   - Can listen to: skill lifecycle, system events, preferences

2. Qt client DIRECTLY connects to WebSocket at KNOWN port (18181)
   - No negotiation needed
   - Can use environment variable OVOS_GUI_PORT (default: 18181)

3. Qt client sends "mycroft.gui.connected" on WebSocket for identification
   - Tells legacy plugin: "I'm here, sync my state"
   - Framework version, site_id info included

4. Legacy plugin WebSocket handler receives and synchronizes:
   - Current active skill stack
   - All session data for visible skills
   - Ready for rendering

5. All further GUI communication on WebSocket only
```

**Architectural Separation**:
```
┌─────────────────────────────────────────────────────────────┐
│ OVOS Core                                                   │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ Message Bus                                             │ │
│ │ • Skill lifecycle (mycroft.intent.recognized, etc.)    │ │
│ │ • System events (mycroft.ready, mycroft.stop, etc.)    │ │
│ │ • NO GUI port negotiation                              │ │
│ └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
                            ↕ (MessageBus)

┌─────────────────────────────────────────────────────────────┐
│ Legacy Plugin (ovos-legacy-mycroft-gui-plugin)             │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ Tornado WebSocket Server (port 18181)                  │ │
│ │ • Receives "mycroft.gui.connected" from Qt clients    │ │
│ │ • Sends: pages, session data, events                  │ │
│ │ • No port negotiation messages                         │ │
│ └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
                            ↕ (WebSocket, port 18181)

┌─────────────────────────────────────────────────────────────┐
│ Qt GUI Client (mycroft-gui-qt5)                            │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ WebSocket Connection (to port 18181 directly)          │ │
│ │ • No port negotiation needed                           │ │
│ │ • Immediate rendering on "mycroft.gui.connected"     │ │
│ └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

---

## Implementation Plan

### Phase 1: Qt Client (mycroftcontroller.cpp)

**Remove**:
- Line 121-123: `sendRequest("mycroft.gui.connected")` on core bus (first send)
- Line 161-162: Re-announce timer that sends to core bus
- Line 507: Any other core bus "mycroft.gui.connected" send
- Related: `m_reannounceGuiTimer` (no longer needed)

**Keep**:
- WebSocket connection logic (lines 140-150)
- Port configuration from environment variable (MYCROFT_GUI_PORT)
- WebSocket message handlers

**Result**: Qt client connects directly to WebSocket on startup, no core bus port negotiation.

### Phase 2: Legacy Plugin WebSocket Handler (websocket.py)

**Already Done**: Lines 201-222 already handle "mycroft.gui.connected" on WebSocket!
- Detects framework version
- Sets site_id
- Forwards to core bus (good for skill lifecycle)

**No Changes Needed**: The WebSocket handler is already correctly designed.

### Phase 3: Legacy Plugin __init__.py

**Remove**:
- Lines 135-148: `_on_qt_client_announced()` method
- Line 116: `self.bus.on("mycroft.gui.connected", ...)` registration
- Comment on lines 125-126 about Qt client port negotiation

**Result**: Plugin no longer sends "mycroft.gui.port" message.

---

## Backward Compatibility Strategy

**Breaking Change**: Yes, this requires client + plugin update together.

**Migration Path**:
1. Update plugin FIRST (can accept both old and new clients)
   - Ignore "mycroft.gui.connected" on core bus (don't crash)
   - Handle "mycroft.gui.connected" on WebSocket (primary flow)

2. Update Qt client (now uses new protocol only)
   - Don't send on core bus
   - Connect directly to WebSocket

3. Remove backward compat code from plugin (next major version)

---

## Benefits

✅ **Architectural Clarity**:
- Clear separation: Core bus for skill lifecycle, WebSocket for GUI

✅ **Eliminates Code Smell**:
- No more port negotiation messages
- No redundant "mycroft.gui.connected" sends
- No waiting for port assignment

✅ **Performance**:
- Faster connection (parallel instead of sequential)
- Fewer round-trip messages
- Immediate rendering on connect

✅ **Maintainability**:
- Simpler state machine (no "waiting for port" state)
- Fewer edge cases (timeouts, retries)
- Cleaner request/response patterns

---

## Files Affected

| Component | File | Changes |
|-----------|------|---------|
| Qt Client | `mycroftcontroller.cpp` | Remove core bus "mycroft.gui.connected" sends |
| Qt Client | `mycroftcontroller.h` | Remove `m_reannounceGuiTimer` member |
| Plugin | `ovos_legacy_mycroft_gui/__init__.py` | Remove `_on_qt_client_announced` handler |
| Plugin | `ovos_legacy_mycroft_gui/__init__.py` | Remove bus.on() registration |

---

## Implementation Checklist

- [ ] Phase 1: Update Qt client
  - [ ] Remove sendRequest("mycroft.gui.connected") calls (line 121, 161, 507)
  - [ ] Remove m_reannounceGuiTimer (lines 153-165)
  - [ ] Verify WebSocket connects directly to port 18181
  - [ ] Test: Qt client connects on startup without port message

- [ ] Phase 2: Legacy plugin backward compat (optional)
  - [ ] Detect and ignore "mycroft.gui.connected" on core bus (don't error)
  - [ ] Log warning if old protocol detected

- [ ] Phase 3: Remove backward compat (future major version)
  - [ ] Remove `_on_qt_client_announced` handler
  - [ ] Remove bus.on() registration
  - [ ] Clean up comments

- [ ] Testing:
  - [ ] Qt client starts → connects to WebSocket directly
  - [ ] Legacy plugin receives "mycroft.gui.connected" on WebSocket
  - [ ] Skill pages render without port negotiation delay
  - [ ] Session data syncs correctly

- [ ] Documentation:
  - [ ] Update PROTOCOL.md with new flow
  - [ ] Update PROTOCOL_QUICK_REFERENCE.md (remove "mycroft.gui.port")
  - [ ] Add migration notes to AUDIT.md

---

## Testing Strategy

**Unit Test**: Verify message routing in cleaned code

**Integration Test**:
- Start Qt client with legacy plugin
- Verify WebSocket connection on port 18181
- Send "mycroft.gui.connected" on WebSocket
- Verify state synchronization without port message

**System Test**:
- Deploy together
- Start skill
- Verify rendering works

---

## Timeline

**Immediate** (this session):
1. Implement Phase 1 in Qt client
2. Document protocol change

**Follow-up** (future):
1. Implement Phase 2/3 in legacy plugin
2. Release together (same version)
3. Document migration for downstream projects

---

## Questions for Review

1. Should we keep core bus "mycroft.gui.connected" for other purposes (e.g., client registration)?
   → **No**: WebSocket connection is sufficient for identification

2. What if port configuration changes?
   → **Handle**: Support OVOS_GUI_PORT env var (already in code)

3. Do other GUI clients use this protocol?
   → **pyhtmx-gui-client**: Uses different protocol (FastAPI), not affected
   → **ovos-shell**: Depends on mycroft-gui-qt5, will work with new protocol


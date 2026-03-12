/*
 * Copyright 2018 by Marco Martin <mart@kde.org>
 * Copyright 2018 David Edmundson <davidedmundson@kde.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

/**
 * @file mycroftcontroller.cpp
 * @brief Main WebSocket connection handler for GUI protocol
 *
 * CRITICAL ARCHITECTURE NOTE:
 * ===========================
 * This file implements a PURE WEBSOCKET CLIENT that communicates ONLY with the
 * legacy-plugin adapter (ovos-legacy-mycroft-gui-plugin) running on port 18181.
 *
 * It DOES NOT and MUST NOT connect to the OVOS core MessageBus (port 8181 or ZeroMQ).
 * All communication flows through WebSocket JSON messages only.
 *
 * MESSAGE FLOW:
 * =============
 * OVOS Core Bus → legacy-plugin (translator) → mycroft-gui-qt5 (this code)
 *
 * The legacy-plugin filters the OVOS Message Bus to send only 23 whitelisted messages
 * to this GUI client. These are defined in guibusmessages.h::GUIBusMessageType enum.
 *
 * MAIN RESPONSIBILITIES:
 * ======================
 * 1. Maintain WebSocket connection to legacy-plugin
 * 2. Parse incoming JSON messages
 * 3. Route messages to appropriate handlers
 * 4. Track connection state (listening, speaking, ready)
 * 5. Send user interaction events back to core
 *
 * NON-QT DEVELOPER GUIDE:
 * =======================
 * - QWebSocket: Qt's native WebSocket client (like Python's websocket library)
 * - QJsonDocument: Qt's JSON parser (like Python's json module)
 * - QObject::connect: Qt's signal/slot mechanism (event subscription pattern)
 * - emit: Broadcast an event to all connected handlers
 * - Q_ASSERT: Runtime assertion (like Python's assert)
 *
 * See also:
 * - guibusmessages.h: enum of all 23 supported message types
 * - docs/PROTOCOL.md: complete message specification
 * - docs/PROTOCOL_QUICK_REFERENCE.md: message lookup table
 */

#include "mycroftcontroller.h"
#include "guibusmessages.h"
#include "globalsettings.h"
#include "abstractdelegate.h"
#include "activeskillsmodel.h"
#include "abstractskillview.h"
#include "controllerconfig.h"

#include <QtGlobal>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <QProcess>
#include <QQmlPropertyMap>
#include <QStandardItemModel>
#include <QQmlEngine>
#include <QQmlContext>
#include <QUuid>
#include <QWebSocket>
#include <QMediaPlayer>
#include <QNetworkConfigurationManager>

MycroftController *MycroftController::instance()
{
    static MycroftController* s_self = nullptr;
    if (!s_self) {
        s_self = new MycroftController;
    }
    return s_self;
}


MycroftController::MycroftController(QObject *parent)
    : QObject(parent),
      m_appSettingObj(new GlobalSettings)
{
    m_qt_version_context = QStringLiteral("5");

    m_useTls = qgetenv("MYCROFT_GUI_TLS").toInt() == 1 || 
               qgetenv("MYCROFT_GUI_TLS").toLower() == "true";
    m_authToken = QString::fromUtf8(qgetenv("MYCROFT_GUI_TOKEN"));

    connect(&m_mainWebSocket, &QWebSocket::connected, this,
            [this] () {
                m_reconnectTimer.stop();
                emit socketStatusChanged();
            });
    connect(&m_mainWebSocket, &QWebSocket::disconnected, this, &MycroftController::closed);
    connect(&m_mainWebSocket, &QWebSocket::stateChanged, this,
            [this] (QAbstractSocket::SocketState state) {
                emit socketStatusChanged();
                if (state == QAbstractSocket::ConnectedState) {
                    qWarning() << "Main Socket connected, trying to connect gui";
                    #if QT_VERSION >= 0x060000
                        m_qt_version_context = QStringLiteral("6");
                    #else
                        m_qt_version_context = QStringLiteral("5");
                    #endif

                    for (const auto &guiId : m_views.keys()) {
                        sendRequest(QStringLiteral("mycroft.gui.connected"),
                                    QVariantMap({{QStringLiteral("gui_id"), guiId}}),
                                    QVariantMap({{QStringLiteral("qt_version"), m_qt_version_context}}));
                    }
                    m_reannounceGuiTimer.start();

                    sendRequest(QStringLiteral("mycroft.skills.all_loaded"), QVariantMap());
                } else {
                    if (m_serverReady) {
                        m_serverReady = false;
                        emit serverReadyChanged();
                    }
                }
            });

    connect(&m_mainWebSocket, &QWebSocket::textMessageReceived, this, &MycroftController::onMainSocketMessageReceived);

    m_reconnectTimer.setInterval(1000);
    connect(&m_reconnectTimer, &QTimer::timeout, this, [this]() {
        QString host = QString::fromUtf8(qgetenv("MYCROFT_GUI_HOST")).isEmpty() 
            ? m_appSettingObj->webSocketAddress() 
            : QString::fromUtf8(qgetenv("MYCROFT_GUI_HOST"));
        int port = qgetenv("MYCROFT_GUI_PORT").toInt();
        if (port == 0) {
            port = 18181;
        }
        QString scheme = m_useTls ? QStringLiteral("wss") : QStringLiteral("ws");
        QString path = m_authToken.isEmpty() ? QStringLiteral("/gui") : QStringLiteral("/gui?token=") + m_authToken;
        QString socket = QStringLiteral("%1://%2:%3%4").arg(scheme).arg(host).arg(port).arg(path);
        m_mainWebSocket.open(QUrl(socket));
    });

    m_reannounceGuiTimer.setInterval(10000);
    connect(&m_reannounceGuiTimer, &QTimer::timeout, this, [this]() {
        if (m_mainWebSocket.state() != QAbstractSocket::ConnectedState) {
            return;
        }
        for (const auto &guiId : m_views.keys()) {
            if (m_views[guiId]->status() != Open) {
                qWarning()<<"Retrying to announce gui";
                sendRequest(QStringLiteral("mycroft.gui.connected"),
                            QVariantMap({{QStringLiteral("gui_id"), guiId}}), QVariantMap({{QStringLiteral("qt_version"), m_qt_version_context}}));
            }
        }
    });
}


void MycroftController::start()
{
    QString host = QString::fromUtf8(qgetenv("MYCROFT_GUI_HOST")).isEmpty() 
        ? m_appSettingObj->webSocketAddress() 
        : QString::fromUtf8(qgetenv("MYCROFT_GUI_HOST"));
    int port = qgetenv("MYCROFT_GUI_PORT").toInt();
    if (port == 0) {
        port = 18181;
    }
    QString scheme = m_useTls ? QStringLiteral("wss") : QStringLiteral("ws");
    QString path = m_authToken.isEmpty() ? QStringLiteral("/gui") : QStringLiteral("/gui?token=") + m_authToken;
    QString socket = QStringLiteral("%1://%2:%3%4").arg(scheme).arg(host).arg(port).arg(path);
    m_mainWebSocket.open(QUrl(socket));
    connect(&m_mainWebSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, [this] (const QAbstractSocket::SocketError &error) {
        //qDebug() << error;

        if (error != QAbstractSocket::HostNotFoundError && error != QAbstractSocket::ConnectionRefusedError) {
            qWarning() << "Mycroft is running but the connection failed for some reason. Kill Mycroft manually.";

            return;
        }

        m_reconnectTimer.start();
        emit socketStatusChanged();
    });
    emit socketStatusChanged();
}

void MycroftController::disconnectSocket()
{
    qDebug() << "in reconnect";
    m_mainWebSocket.close();
    m_reconnectTimer.stop();
    emit socketStatusChanged();
}

void MycroftController::reconnect()
{
    qDebug() << "in reconnect";
    m_mainWebSocket.close();
    m_reconnectTimer.start();
    emit socketStatusChanged();
}

/**
 * onMainSocketMessageReceived - Main entry point for all incoming GUI protocol messages
 *
 * This function is called every time the legacy-plugin sends a message via WebSocket.
 * It parses the JSON and routes to the appropriate handler based on message type.
 *
 * MESSAGE TYPES (23 total, defined in guibusmessages.h):
 * ======================================================
 * All messages from OVOS core are filtered by legacy-plugin to these 23 types:
 * - 1 initialization message (GUI_CONNECTED)
 * - 3 page rendering messages (GUI_LIST_*)
 * - 8 session data messages (SESSION_*)
 * - 10 state change messages (RECOGNIZER_*, STOP_*, etc.)
 * - 2 user interaction messages (EVENTS_TRIGGERED, RECOGNIZER_UTTERANCE)
 *
 * FLOW:
 * 1. Parse JSON from WebSocket string
 * 2. Extract "type" field
 * 3. Match against enum values from guibusmessages.h
 * 4. Call appropriate handler
 * 5. Return (don't process further)
 *
 * NOTE: Messages NOT matched here are silently ignored (not errors).
 * This allows the protocol to be extended without breaking older clients.
 */
void MycroftController::onMainSocketMessageReceived(const QString &message)
{
    // Parse the incoming WebSocket message as JSON
    // This is like: json.loads(message) in Python
    auto doc = QJsonDocument::fromJson(message.toUtf8());

    // Check if JSON parsing succeeded
    if (doc.isEmpty()) {
        qWarning() << "Empty or invalid JSON message arrived on the main socket:" << message;
        return;
    }

    // Extract the "type" field from the JSON root object
    // This tells us which kind of message this is
    // Example: "mycroft.gui.connected", "mycroft.session.set", etc.
    auto typeStr = doc[QStringLiteral("type")].toString();

    // Validate that type field exists
    if (typeStr.isEmpty()) {
        qWarning() << "Empty type in the JSON message on the main socket";
        return;
    }

#ifdef DEBUG_MYCROFT_MESSAGEBUS
    // Optional debug output (only if DEBUG_MYCROFT_MESSAGEBUS is defined at compile time)
    qDebug() << "type" << typeStr;
#endif

    // ========================================
    // CONVERT STRING TYPE TO ENUM
    // ========================================
    // Use the centralized enum from guibusmessages.h for type-safe routing.
    // This gives us compile-time safety and a single source of truth for supported message types.
    // This handles all OVOS bus messages that are whitelisted by the legacy-plugin.
    auto msgType = GuiBusMessages::fromString(typeStr);

    // NOTE: PROTOCOL DEBT: "mycroft.gui.port" message
    // ================================================
    // The legacy protocol includes a separate "mycroft.gui.port" message that assigns
    // ports to per-skill WebSocket connections. This is a code smell and should be
    // redesigned to send port information in the initial connection response instead.
    // For now, unknown messages (including "mycroft.gui.port") are silently ignored.
    // The legacy-plugin will need protocol updates to eliminate this separate negotiation.

    // ========================================
    // MESSAGE HANDLER ROUTING
    // ========================================
    // Below we check the message type and call the appropriate handler.
    // This is essentially a switch statement on message type.
    //
    // Each handler:
    // 1. Validates the message has required fields
    // 2. Extracts data from JSON
    // 3. Performs the action (update state, render, etc.)
    // 4. Returns without processing further messages
    //
    // See guibusmessages.h for the complete enum of all supported types.

    // ========================================
    // STATE CHANGE MESSAGES (10 types)
    // ========================================
    // These messages update the client's understanding of core state.
    // They DON'T render anything, just update internal state variables.

    // MESSAGE: complete_intent_failure
    // SOURCE: OVOS core (intent matching failure)
    // PURPOSE: Indicate that all skills failed to match the user's intent
    // EFFECT: Set listening state to false, emit "not understood" event
    if (msgType == GuiBusMessages::GUIBusMessageType::INTENT_FAILURE) {
        m_isListening = false;
        emit isListeningChanged();
        emit notUnderstood();
    }

    // MESSAGE: recognizer_loop:audio_output_start
    // SOURCE: OVOS core audio service
    // PURPOSE: Core/skill is now speaking (TTS output started)
    // EFFECT: Set m_isSpeaking=true, notify listeners
    if (msgType == GuiBusMessages::GUIBusMessageType::RECOGNIZER_AUDIO_OUTPUT_START) {
        m_isSpeaking = true;
        emit isSpeakingChanged();  // Notify QML UI that speaking state changed
        return;
    }

    // MESSAGE: recognizer_loop:audio_output_end
    // SOURCE: OVOS core audio service
    // PURPOSE: Core/skill finished speaking
    // EFFECT: Set m_isSpeaking=false, notify listeners
    if (msgType == GuiBusMessages::GUIBusMessageType::RECOGNIZER_AUDIO_OUTPUT_END) {
        m_isSpeaking = false;
        emit isSpeakingChanged();  // Notify QML UI
        return;
    }

    // MESSAGE: recognizer_loop:wakeword
    // SOURCE: OVOS core wakeword detector
    // PURPOSE: Wakeword detected (user said the wake word)
    // EFFECT: Set m_isListening=true (client is now listening for speech)
    if (msgType == GuiBusMessages::GUIBusMessageType::RECOGNIZER_WAKEWORD) {
        m_isListening = true;
        emit isListeningChanged();  // Notify QML UI
        return;
    }

    // MESSAGE: recognizer_loop:record_begin
    // SOURCE: OVOS core audio recorder
    // PURPOSE: Audio recording started (user is speaking)
    // EFFECT: Set m_isListening=true (if not already set)
    if (msgType == GuiBusMessages::GUIBusMessageType::RECOGNIZER_RECORD_BEGIN && !m_isListening) {
        m_isListening = true;
        emit isListeningChanged();  // Notify QML UI
        return;
    }

    // MESSAGE: recognizer_loop:record_end
    // SOURCE: OVOS core audio recorder
    // PURPOSE: Audio recording ended (user stopped speaking)
    // EFFECT: Set m_isListening=false
    if (msgType == GuiBusMessages::GUIBusMessageType::RECOGNIZER_RECORD_END) {
        m_isListening = false;
        emit isListeningChanged();  // Notify QML UI
        return;
    }

    // MESSAGE: mycroft.speech.recognition.unknown
    // SOURCE: OVOS core speech recognition
    // PURPOSE: Recognized audio but it's not understandable (gibberish, background noise, etc.)
    // EFFECT: Emit "not understood" event
    if (msgType == GuiBusMessages::GUIBusMessageType::SPEECH_RECOGNITION_UNKNOWN) {
        emit notUnderstood();
        return;
    }

    // MESSAGE: mycroft.stop.handled / mycroft.stop
    // SOURCE: OVOS core skill manager
    // PURPOSE: Skill was stopped (by user interrupt or completion)
    // EFFECT: Emit "stopped" event
    if (msgType == GuiBusMessages::GUIBusMessageType::STOP_HANDLED) {
        emit stopped();
        return;
    }


    // ========================================
    // NAMESPACE LIFECYCLE MESSAGE
    // ========================================

    // MESSAGE: gui.clear.namespace
    // SOURCE: OVOS core / legacy-plugin
    // PURPOSE: Clear all data for a skill namespace
    // WHEN: Skill is removed from active stack or unloaded
    // EFFECT: Forward clear request to all connected views
    // NOTE: Views clean up their session data when they receive this message
    if (msgType == GuiBusMessages::GUIBusMessageType::CLEAR_NAMESPACE) {
        const QString namespace_ = doc[QStringLiteral("data")][QStringLiteral("namespace")].toString();
        if (!namespace_.isEmpty()) {
            // Forward the clear request to all connected skill views
            // Each view will check if this is its namespace and clear appropriately
            for (auto view : m_views) {
                view->sendMessage(message);
            }
        }
        return;
    }

    // ========================================
    // CORE LIFECYCLE STATE MESSAGES
    // ========================================

    // MESSAGE: mycroft.skills.all_loaded.response
    // SOURCE: OVOS core skill manager
    // PURPOSE: All skills have been loaded and initialized
    // EFFECT: Set m_serverReady flag (core is ready to receive events)
    if (msgType == GuiBusMessages::GUIBusMessageType::SKILLS_LOADED_RESPONSE) {
        if (doc[QStringLiteral("data")][QStringLiteral("status")].toBool() == true) {
            m_serverReady = true;
            emit serverReadyChanged();
        }
    } else if (msgType == GuiBusMessages::GUIBusMessageType::READY) {
        m_serverReady = true;
        emit serverReadyChanged();
    }

    if (msgType == GuiBusMessages::GUIBusMessageType::SCREEN_CLOSE_IDLE_EVENT) {
        QString skill_idle_event_id = doc[QStringLiteral("data")][QStringLiteral("skill_idle_event_id")].toString();
        emit skillTimeoutReceived(skill_idle_event_id);
    }

    // ========================================
    // CUSTOM/UNKNOWN MESSAGE TYPES
    // ========================================
    // The protocol supports extension: unknown messages are not errors.
    // This allows future versions of OVOS to send new message types
    // without breaking older GUI clients.
    //
    // Check if it's a custom skill-specific utterance event.
    // These are not in the standard whitelist but follow the pattern:
    // "skill.namespace:event_name" with utterance data.
    if (msgType == static_cast<GuiBusMessages::GUIBusMessageType>(-1)) {
        // Message type is unknown (not in the enum whitelist)
        // Check if it's a skill-specific utterance event
        if (typeStr.contains(QLatin1Char(':')) && !doc[QStringLiteral("data")][QStringLiteral("utterance")].toString().isEmpty()) {
            const QString skill = typeStr.split(QLatin1Char(':')).first();
            if (skill.contains(QLatin1Char('.'))) {
                qDebug() << "Current skill:" << skill;
                emit utteranceManagedBySkill(skill);
            }
        }
    }
}

void MycroftController::sendRequest(const QString &type, const QVariantMap &data, const QVariantMap &context)
{
    if (m_mainWebSocket.state() != QAbstractSocket::ConnectedState) {
        qWarning() << "mycroft connection not open!";
        return;
    }

    QJsonObject root;
    root[QStringLiteral("type")] = type;
    root[QStringLiteral("data")] = QJsonObject::fromVariantMap(data);

    // Ensure context has {"session": {"session_id": "default"}}
    QJsonObject contextJson = QJsonObject::fromVariantMap(context);
    if (!contextJson.contains(QStringLiteral("session"))) {
        QJsonObject session;
        session[QStringLiteral("session_id")] = QStringLiteral("default");
        contextJson[QStringLiteral("session")] = session;
    }

    root[QStringLiteral("context")] = contextJson;

    QJsonDocument doc(root);
    m_mainWebSocket.sendTextMessage(QString::fromUtf8(doc.toJson()));
}

void MycroftController::sendBinary(const QString &type, const QJsonObject &data, const QVariantMap &context)
{
    if (m_mainWebSocket.state() != QAbstractSocket::ConnectedState) {
        qWarning() << "mycroft connection not open!";
        return;
    }
    QJsonObject socketObject;
    socketObject[QStringLiteral("type")] = type;
    socketObject[QStringLiteral("data")] = data;
    socketObject[QStringLiteral("context")] = QJsonObject::fromVariantMap(context);

    QJsonDocument doc;
    doc.setObject(socketObject);
    QByteArray docbin = doc.toJson(QJsonDocument::Compact);
    m_mainWebSocket.sendBinaryMessage(docbin);
}

void MycroftController::sendText(const QString &message)
{
    sendRequest(QStringLiteral("recognizer_loop:utterance"), QVariantMap({{QStringLiteral("utterances"), QStringList({message})}}), QVariantMap({{QStringLiteral("source"), QStringLiteral("mycroft-gui")}, {QStringLiteral("destination"), QStringLiteral("skills")}, {QStringLiteral("qt_version"), m_qt_version_context}}));
}

void MycroftController::registerView(AbstractSkillView *view)
{
    Q_ASSERT(!view->id().isEmpty());
    Q_ASSERT(!m_views.contains(view->id()));
    m_views[view->id()] = view;
    // Connect view destruction to deregisterView
    connect(view, &QObject::destroyed, this, [this, view]() {
        deregisterView(view);
    });
    if (m_mainWebSocket.state() == QAbstractSocket::ConnectedState) {
        sendRequest(QStringLiteral("mycroft.gui.connected"),
                    QVariantMap({{QStringLiteral("gui_id"), view->id()}}), QVariantMap({{QStringLiteral("qt_version"), m_qt_version_context}}));
    }
}

void MycroftController::deregisterView(AbstractSkillView *view)
{
    if (!view) {
        return;
    }
    const QString viewId = view->id();
    if (m_views.contains(viewId)) {
        m_views.remove(viewId);
    }
}

MycroftController::Status MycroftController::status() const
{
    if (m_reconnectTimer.isActive()) {
        return Connecting;
    }

    switch(m_mainWebSocket.state())
    {
    case QAbstractSocket::ConnectingState:
    case QAbstractSocket::BoundState:
    case QAbstractSocket::HostLookupState:
        return Connecting;
    case QAbstractSocket::UnconnectedState:
        return Closed;
    case QAbstractSocket::ConnectedState:
        return Open;
    case QAbstractSocket::ClosingState:
        return Closing;
    default:
        return Connecting;
    }
}

bool MycroftController::isSpeaking() const
{
    return m_isSpeaking;
}

bool MycroftController::isListening() const
{
    return m_isListening;
}

bool MycroftController::serverReady() const
{
    return m_serverReady;
}

bool MycroftController::useTls() const
{
    return m_useTls;
}

QString MycroftController::authToken() const
{
    return m_authToken;
}

#include "moc_mycroftcontroller.cpp"

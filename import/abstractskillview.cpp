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
 * @file abstractskillview.cpp
 * @brief Per-skill GUI view manager - handles session data and QML rendering
 *
 * CRITICAL ARCHITECTURE:
 * ======================
 * This file manages a SINGLE SKILL'S GUI rendering:
 * - One AbstractSkillView per active skill
 * - Receives skill data updates via WebSocket
 * - Renders QML templates with that data
 * - Sends user interaction events back to core
 *
 * WEBSOCKET HIERARCHY:
 * ====================
 * There are TWO WebSocket connections:
 *
 * 1. MAIN CONNECTION (MycroftController)
 *    - Connected to legacy-plugin (port 18181)
 *    - Receives: initialization, state changes, page list updates
 *    - Sends: user interactions
 *    - Always open (singleton)
 *
 * 2. PER-SKILL CONNECTIONS (AbstractSkillView, one per skill)
 *    - Connected to per-skill port (assigned via mycroft.gui.port message)
 *    - Receives: session data updates for THIS SKILL only
 *    - Sends: nothing (data only, one-way flow)
 *    - Multiple instances (one per active skill)
 *
 * This separation allows:
 * - Multiple skills rendering in parallel (each on its own port)
 * - Skill data is isolated (skill A doesn't see skill B's data)
 * - Better scalability (data flows only to views that need it)
 *
 * SESSION DATA MODEL:
 * ===================
 * Each skill has a "session" = dictionary of key-value data + lists
 *
 * Structure:
 *   skill_id (e.g., "weather.openweathermap")
 *     ├── property_1: "value"           (string, number, bool)
 *     ├── property_2: "value"
 *     └── list_property: [
 *           { item1: "data1", item2: "data2" },
 *           { item1: "data1", item2: "data2" }
 *         ]
 *
 * Messages (8 types):
 * - SESSION_SET: update or create key-value
 * - SESSION_DELETE: remove key
 * - SESSION_LIST_INSERT: add items to list
 * - SESSION_LIST_REMOVE: delete items from list
 * - SESSION_LIST_MOVE: reorder list items
 * - SESSION_LIST_UPDATE: update existing list items
 * - CLEAR_NAMESPACE: delete entire skill's session
 *
 * QML ACCESS:
 * ===========
 * QML templates access session data via "sessionData" object:
 *
 *   Text { text: sessionData.title }
 *   ListView { model: sessionDataModel("my_list") }
 *
 * When session data changes, QML automatically re-renders (reactive binding).
 *
 * NON-QT DEVELOPER GUIDE:
 * =======================
 * - QQmlContext: How to inject C++ data into QML (like passing a dict to JavaScript)
 * - SessionDataModel: C++ model that provides list data to QML
 * - SessionDataMap: C++ dict (QHash) that provides key-value data to QML
 * - m_skillData: All session data for this skill (keyed by property name)
 *
 * See also:
 * - MycroftController: Main WebSocket connection handler
 * - docs/PROTOCOL.md: Complete message specification
 * - guibusmessages.h: Enum of all 23 supported message types
 */

#include "abstractskillview.h"
#include "guibusmessages.h"
#include "activeskillsmodel.h"
#include "abstractdelegate.h"
#include "sessiondatamap.h"
#include "sessiondatamodel.h"
#include "delegatesmodel.h"
#include "controllerconfig.h"

#include <QWebSocket>
#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTranslator>
#include <QFileInfo>

// ---------------------------------------------------------------------------
// SYSTEM: URI scheme
//
// The OVOS server sends "SYSTEM:<TemplateName>.qml" instead of a file:// URI.
// This keeps QML resources client-side: the server never needs the Qt install.
//
// Resolution order:
//   1. $OVOS_SYSTEM_TEMPLATES/<TemplateName>.qml   (runtime override)
//   2. MYCROFT_SYSTEM_TEMPLATES_DIR/<TemplateName>.qml  (compiled-in default)
//
// Shells ship their own system-templates directory and set OVOS_SYSTEM_TEMPLATES
// in their launch script to activate their themed versions.
// ---------------------------------------------------------------------------
static QUrl resolveSystemTemplate(const QString &templateName)
{
    // Shell override directory — checked first; only files present here are
    // overridden.  Missing files fall through to the compiled-in default so
    // shells only need to ship the templates they actually customise.
    const QString envDir = qEnvironmentVariable("OVOS_SYSTEM_TEMPLATES");
    if (!envDir.isEmpty()) {
        const QString envPath = envDir + QLatin1Char('/') + templateName;
        if (QFileInfo::exists(envPath)) {
            return QUrl::fromLocalFile(envPath);
        }
    }
    return QUrl::fromLocalFile(
        QStringLiteral(MYCROFT_SYSTEM_TEMPLATES_DIR) + QLatin1Char('/') + templateName
    );
}

static QUrl resolveDelegate(const QString &urlString)
{
    static const QString systemPrefix = QStringLiteral("SYSTEM:");
    if (urlString.startsWith(systemPrefix)) {
        return resolveSystemTemplate(urlString.mid(systemPrefix.length()));
    }
    return QUrl::fromUserInput(urlString);
}

AbstractSkillView::AbstractSkillView(QQuickItem *parent)
    : QQuickItem(parent),
      m_id(QUuid::createUuid().toString()),
      m_controller(MycroftController::instance())
{
    m_activeSkillsModel = new ActiveSkillsModel(this);

    m_guiWebSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
    m_controller->registerView(this);

    connect(m_guiWebSocket, &QWebSocket::connected, this,
            [this] () {
                m_reconnectTimer.stop();
                emit statusChanged();
            });

    connect(m_guiWebSocket, &QWebSocket::disconnected, this, &AbstractSkillView::closed);

    connect(m_guiWebSocket, &QWebSocket::disconnected, this, [this]() {
        m_activeSkillsModel->removeRows(0, m_activeSkillsModel->rowCount());
        // Clear all session data when socket disconnects
        for (auto it = m_skillData.begin(); it != m_skillData.end(); ++it) {
            it.value()->deleteLater();
        }
        m_skillData.clear();
    });

    connect(m_guiWebSocket, &QWebSocket::stateChanged, this,
            [this] (QAbstractSocket::SocketState state) {
                emit statusChanged();
            });

    connect(m_guiWebSocket, &QWebSocket::textMessageReceived, this, &AbstractSkillView::onGuiSocketMessageReceived);

    connect(m_guiWebSocket, &QWebSocket::stateChanged, this,
            [this](QAbstractSocket::SocketState socketState) {
                //TODO: when the connection closes, all session data and guis should be destroyed
                //qWarning()<<"GUI SOCKET STATE:"<<socketState;
                //Try to reconnect if our connection died but the main server connection is still alive
                if (socketState == QAbstractSocket::UnconnectedState && m_url.isValid() && m_controller->status() == MycroftController::Open) {
                    m_reconnectTimer.start();
                }
            });

    connect(m_guiWebSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this,
            [this](QAbstractSocket::SocketError error) {
                qWarning() << "Gui socket Connection Error:" << error;
                m_reconnectTimer.start();
            });


    connect(m_controller, &MycroftController::socketStatusChanged, this,
            [this]() {
                if (m_controller->status() != MycroftController::Open) {
                    m_guiWebSocket->close();
                    //don't assume the url will be still valid
                    m_url = QUrl();
                }
            });

    // Reconnect timer
    m_reconnectTimer.setInterval(1000);
    connect(&m_reconnectTimer, &QTimer::timeout, this, [this]() {
        m_guiWebSocket->close();
        m_guiWebSocket->open(m_url);
    });

    // Trim components cache timer
    m_trimComponentsTimer.setInterval(100);
    m_trimComponentsTimer.setSingleShot(true);
    connect(&m_trimComponentsTimer, &QTimer::timeout, this, [this]() {
        QQmlEngine *engine = qmlEngine(this);
        if (engine) {
            engine->clearComponentCache();
        }
    });

    connect(m_controller, &MycroftController::utteranceManagedBySkill, this,
        [this](const QString &skillId) {
            m_activeSkillsModel->checkGuiActivation(skillId);
        });
}

AbstractSkillView::~AbstractSkillView()
{
}


QUrl AbstractSkillView::url() const
{
    return m_url;
}

void AbstractSkillView::setUrl(const QUrl &url)
{
    if (m_url == url) {
        return;
    }

    m_url = url;

    //don't connect if the controller is offline
    if (m_controller->status() == MycroftController::Open) {
        m_guiWebSocket->close();
        m_guiWebSocket->open(url);
    }
}

QString AbstractSkillView::id() const
{
    return m_id;
}

void AbstractSkillView::triggerEvent(const QString &skillId, const QString &eventName, const QVariantMap &parameters)
{
    if (m_guiWebSocket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "Error: Mycroft gui connection not open!";
        return;
    }
    QJsonObject root;

    root[QStringLiteral("type")] = QStringLiteral("mycroft.events.triggered");
    root[QStringLiteral("namespace")] = skillId;
    root[QStringLiteral("event_name")] = eventName;
    root[QStringLiteral("parameters")] = QJsonObject::fromVariantMap(parameters);

    QJsonDocument doc(root);
    m_guiWebSocket->sendTextMessage(QString::fromUtf8(doc.toJson()));
}

void AbstractSkillView::writeProperties(const QString &skillId, const QVariantMap &data)
{
    if (m_guiWebSocket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "Error: Mycroft gui connection not open!";
        return;
    }
    QJsonObject root;

    root[QStringLiteral("type")] = QStringLiteral("mycroft.session.set");
    root[QStringLiteral("namespace")] = skillId;
    root[QStringLiteral("data")] = QJsonObject::fromVariantMap(data);

    QJsonDocument doc(root);
    m_guiWebSocket->sendTextMessage(QString::fromUtf8(doc.toJson()));
}

void AbstractSkillView::deleteProperty(const QString &skillId, const QString &property)
{
    if (m_guiWebSocket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "Error: Mycroft gui connection not open!";
        return;
    }
    QJsonObject root;

    root[QStringLiteral("type")] = QStringLiteral("mycroft.session.delete");
    root[QStringLiteral("namespace")] = skillId;
    root[QStringLiteral("property")] = property;

    QJsonDocument doc(root);
    m_guiWebSocket->sendTextMessage(QString::fromUtf8(doc.toJson()));
}

MycroftController::Status AbstractSkillView::status() const
{
    if (m_reconnectTimer.isActive()) {
        return MycroftController::Connecting;
    }

    switch(m_guiWebSocket->state())
    {
    case QAbstractSocket::ConnectingState:
    case QAbstractSocket::BoundState:
    case QAbstractSocket::HostLookupState:
        return MycroftController::Connecting;
    case QAbstractSocket::UnconnectedState:
        return MycroftController::Closed;
    case QAbstractSocket::ConnectedState:
        return MycroftController::Open;
    case QAbstractSocket::ClosingState:
        return MycroftController::Closing;
    default:
        return MycroftController::Connecting;
    }
}

ActiveSkillsModel *AbstractSkillView::activeSkills() const
{
    return m_activeSkillsModel;
}

SessionDataMap *AbstractSkillView::sessionDataForSkill(const QString &skillId)
{
    SessionDataMap *map = nullptr;

    if (m_skillData.contains(skillId)) {
        map = m_skillData[skillId];
    } else if (m_activeSkillsModel->skillIndex(skillId).isValid()) {
        map = new SessionDataMap(skillId, this);
        m_skillData[skillId] = map;
    }

    return map;
}

QList<QVariantMap> variantListToOrderedMap(const QVariantList &data)
{
    QList<QVariantMap> ordMap;

    QStringList roleNames;

    for (const auto &item : data) {
        if (!item.canConvert<QVariantMap>()) {
            qWarning() << "Error: Array data structure corrupted: " << data;
            return ordMap;
        }
        const auto &map = item.value<QVariantMap>();
        if (roleNames.isEmpty()) {
            roleNames = map.keys();
        } else if (roleNames != map.keys()) {
            qWarning() << "WARNING: Item with a wrong set of roles encountered, some roles will be inaccessible from QML, expected: " << roleNames << "Encountered: " << map.keys();
        }
        ordMap << map;
    }

    return ordMap;
}

QStringList jsonModelToStringList(const QString &key, const QJsonValue &data)
{
    QStringList items;

    if (!data.isArray()) {
        qWarning() << "Error: Model data is not an Array" << data;
        return items;
    }

    const auto &array = data.toArray();
    for (const auto &item : array) {
        if (!item.isObject()) {
            qWarning() << "Error: Array data structure currupted: " << data;
            items.clear();
            return items;
        }
        const auto &obj = item.toObject();
        const auto &value = obj.value(key);
        if (!value.isString()) {
            qWarning() << "Error: item in model not a string" << value;
        }
        items << value.toString();
    }

    return items;
}

/**
 * onGuiSocketMessageReceived - Per-skill session data message handler
 *
 * This function is called when THIS SKILL receives session data updates
 * from the per-skill WebSocket connection.
 *
 * MESSAGE TYPES HANDLED (8 types):
 * ===============================
 * All session data and list management messages:
 * - mycroft.session.set: update key-value data
 * - mycroft.session.delete: remove key
 * - mycroft.session.list.insert: add list items
 * - mycroft.session.list.remove: delete list items
 * - mycroft.session.list.move: reorder list items
 * - mycroft.session.list.update: update list item values
 * - gui.clear.namespace: clear entire skill session
 * - (Plus page rendering: mycroft.gui.list.*)
 *
 * EXECUTION FLOW:
 * ===============
 * 1. Parse JSON from WebSocket
 * 2. Extract "type" field (message kind)
 * 3. Match against enum values (guibusmessages.h)
 * 4. Call appropriate handler to update C++ data structures
 * 5. Qt signals automatically notify QML when data changes
 * 6. QML re-renders automatically (reactive binding)
 *
 * THREAD SAFETY:
 * ==============
 * WebSocket callbacks may arrive from any thread.
 * All Qt code MUST run on the main GUI thread.
 * This is handled automatically by Qt's signal/slot mechanism.
 */
void AbstractSkillView::onGuiSocketMessageReceived(const QString &message)
{
    // Parse the incoming WebSocket message as JSON
    // This is equivalent to json.loads(message) in Python
    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);

    // Check if JSON parsing succeeded
    if (doc.isEmpty()) {
        qWarning() << "Empty or invalid JSON message arrived on the gui socket:" << message << "Error:" << parseError.errorString();
        return;
    }

    // Extract the "type" field from the JSON root object
    // This tells us what kind of message this is
    auto typeStr = doc[QStringLiteral("type")].toString();

    // Validate that type field exists
    if (typeStr.isEmpty()) {
        qWarning() << "Empty type in the JSON message on the gui socket";
        return;
    }

    //qDebug() << "gui message type" << typeStr;

    // ========================================
    // CONVERT STRING TYPE TO ENUM
    // ========================================
    // Use the centralized enum from guibusmessages.h for type-safe routing.
    auto msgType = GuiBusMessages::fromString(typeStr);

    // ========================================
    // SESSION DATA MESSAGE HANDLERS (8 types)
    // ========================================
    // Below we update the skill's session data based on message type.
    // Session data is exposed to QML via the "sessionData" object.
    //
    // When C++ updates m_skillData, Qt signals notify QML,
    // which automatically re-renders (reactive binding).

    // MESSAGE: mycroft.session.set
    // PURPOSE: Set or update a key-value property in skill session
    // EXAMPLE: { "type": "mycroft.session.set", "namespace": "weather.openweathermap", "data": { "title": "San Francisco", "temp": 72 } }
    // EFFECT: QML can now use {{ sessionData.title }} and {{ sessionData.temp }}
    if (msgType == GuiBusMessages::GUIBusMessageType::SESSION_SET) {
        const QString skillId = doc[QStringLiteral("namespace")].toString();
        const QVariantMap data = doc[QStringLiteral("data")].toVariant().toMap();

        if (skillId.isEmpty()) {
            qWarning() << "Empty skill_id in mycroft.session.set";
            return;
        }
        if (!m_activeSkillsModel->skillIndex(skillId).isValid()) {
            qWarning() << "Invalid skill_id in mycroft.session.set:" << skillId;
            return;
        }
        if (data.isEmpty()) {
            qWarning() << "Empty data in mycroft.session.set";
            return;
        }

        //we already checked, assume *map is valid
        SessionDataMap *map = sessionDataForSkill(skillId);
        if (!map) {
            return;
        }
        QVariantMap::const_iterator i;
        for (i = data.constBegin(); i != data.constEnd(); ++i) {
            //insert it as a model
            QList<QVariantMap> list = variantListToOrderedMap(i.value().value<QVariantList>());
            SessionDataModel *dm = map->value(i.key()).value<SessionDataModel *>();

            if (!list.isEmpty()) {
                if (!dm) {
                    dm = new SessionDataModel(map);
                    map->insertAndNotify(i.key(), QVariant::fromValue(dm));
                } else {
                    dm->clear();
                }
                dm->insertData(0, list);

            //insert it as is.
            } else {
                if (dm) {
                    dm->deleteLater();
                }
                map->insertAndNotify(i.key(), i.value());
            }
            //qDebug() << "             " << i.key() << " = " << i.value();
        }

    // The SkillData was removed by the server
    } else if (msgType == GuiBusMessages::GUIBusMessageType::SESSION_DELETE) {
        const QString skillId = doc[QStringLiteral("namespace")].toString();
        const QString property = doc[QStringLiteral("property")].toString();
        if (skillId.isEmpty()) {
            qWarning() << "No skill_id provided in mycroft.session.delete";
            return;
        }
        if (!m_activeSkillsModel->skillIndex(skillId).isValid()) {
            qWarning() << "Invalid skill_id in mycroft.session.delete:" << skillId;
            return;
        }
        if (property.isEmpty()) {
            qWarning() << "No property provided in mycroft.session.delete";
            return;
        }

        SessionDataMap *map = sessionDataForSkill(skillId);
        SessionDataModel *dm = map->value(property).value<SessionDataModel *>();
        map->clearAndNotify(property);
        //a model will need to be manually deleted
        if (dm) {
            dm->deleteLater();
        }
//END SKILLDATA


//BEGIN ACTIVESKILLS
    // Insert new active skill
    } else if (msgType == GuiBusMessages::GUIBusMessageType::SESSION_LIST_INSERT && doc[QStringLiteral("namespace")].toString() == QLatin1String("mycroft.system.active_skills")) {
        const int position = doc[QStringLiteral("position")].toInt();

        if (position < 0 || position > m_activeSkillsModel->rowCount()) {
            qWarning() << "Error: Invalid position in mycroft.session.list.insert of mycroft.system.active_skills";
            return;
        }

        const QStringList skillList = jsonModelToStringList(QStringLiteral("skill_id"), doc[QStringLiteral("data")]);

        if (skillList.isEmpty()) {
            qWarning() << "Error: no valid skills received in mycroft.session.list.insert of mycroft.system.active_skills";
            return;
        }

        m_activeSkillsModel->insertSkills(position, skillList);


    // Active skill removed
    } else if (msgType == GuiBusMessages::GUIBusMessageType::SESSION_LIST_REMOVE && doc[QStringLiteral("namespace")].toString() == QLatin1String("mycroft.system.active_skills")) {
        const int position = doc[QStringLiteral("position")].toInt();
        const int itemsNumber = doc[QStringLiteral("items_number")].toInt();

        if (position < 0 || position > m_activeSkillsModel->rowCount() - 1) {
            qWarning() << "Error: Invalid position in mycroft.session.list.remove of mycroft.system.active_skills";
            return;
        }
        if (itemsNumber < 0 || itemsNumber > m_activeSkillsModel->rowCount() - position) {
            qWarning() << "Error: Invalid items_number in mycroft.session.list.remove of mycroft.system.active_skills";
            return;
        }

        for (int i = 0; i < itemsNumber; ++i) {

            const QString skillId = m_activeSkillsModel->data(m_activeSkillsModel->index(position+i, 0)).toString();

            if (m_translatorsForSkill.contains(skillId)) {
                QTranslator *translator = m_translatorsForSkill[skillId];
                QCoreApplication::removeTranslator(translator);
                m_translatorsForSkill.remove(skillId);
                delete translator;
            }
            //TODO: do this after an animation
            {
                auto i = m_skillData.find(skillId);
                if (i != m_skillData.end()) {
                    i.value()->deleteLater();
                    m_skillData.erase(i);
                }
            }
        }
        m_activeSkillsModel->removeRows(position, itemsNumber);

    // Active skill moved
    } else if (msgType == GuiBusMessages::GUIBusMessageType::SESSION_LIST_MOVE && doc[QStringLiteral("namespace")].toString() == QLatin1String("mycroft.system.active_skills")) {
        const int from = doc[QStringLiteral("from")].toInt();
        const int to = doc[QStringLiteral("to")].toInt();
        const int itemsNumber = doc[QStringLiteral("items_number")].toInt();

        if (from < 0 || from > m_activeSkillsModel->rowCount() - 1) {
            qWarning() << "Error: Invalid from position in mycroft.session.list.move of mycroft.system.active_skills";
            return;
        }
        if (to < 0 || to > m_activeSkillsModel->rowCount() - 1) {
            qWarning() << "Error: Invalid to position in mycroft.session.list.move of mycroft.system.active_skills";
            return;
        }
        if (itemsNumber <= 0 || itemsNumber > m_activeSkillsModel->rowCount() - from) {
            qWarning() << "Error: Invalid items_number in mycroft.session.list.move of mycroft.system.active_skills";
            return;
        }

        m_activeSkillsModel->moveRows(QModelIndex(), from, itemsNumber, QModelIndex(), to);
//END ACTIVESKILLS


//BEGIN GUI MODEL
    // Insert new new gui delegates
    } else if (msgType == GuiBusMessages::GUIBusMessageType::GUI_LIST_INSERT) {
        const QString skillId = doc[QStringLiteral("namespace")].toString();
        if (skillId.isEmpty()) {
            qWarning() << "No skill_id provided in mycroft.gui.list.insert";
            return;
        }

        const int position = doc[QStringLiteral("position")].toInt();

        DelegatesModel *delegatesModel = m_activeSkillsModel->delegatesModelForSkill(skillId);

        if (!delegatesModel) {
            qWarning() << "Error: no delegates model for skill" << skillId;
            return;
        }
        if (position < 0 || position > delegatesModel->rowCount()) {
            qWarning() << "Error: Invalid position in mycroft.gui.list.insert";
            return;
        }

        const QStringList delegateUrls = jsonModelToStringList(QStringLiteral("url"), doc[QStringLiteral("data")]);

        if (delegateUrls.isEmpty()) {
            qWarning() << "Error: no valid skills received in mycroft.gui.list.insert";
            return;
        }

        qWarning() << "Arrived mycroft.gui.list.insert, delegateUrls are" << delegateUrls;

        QList <DelegateLoader *> delegateLoaders;
        for (const auto &urlString : delegateUrls) {
            const QUrl delegateUrl = resolveDelegate(urlString);

            if (!delegateUrl.isValid()) {
                continue;
            }

            DelegateLoader *loader = new DelegateLoader(this);
            loader->init(skillId, delegateUrl);

            qWarning() << "Created a new DelegateLoader" << loader << "which will load" << delegateUrl << "for the skill" << skillId;

            if (!m_translatorsForSkill.contains(skillId)) {
                QTranslator *translator = new QTranslator(this);
                if (translator->load(QLocale(), skillId, QLatin1String("_"), loader->translationsUrl().path())) {
                    QCoreApplication::installTranslator(translator);
                    m_translatorsForSkill[skillId] = translator;
                } else {
                    translator->deleteLater();
                }
            }

            connect(loader, &QObject::destroyed, &m_trimComponentsTimer, QOverload<>::of(&QTimer::start));

            delegateLoaders << loader;
        }

        if (delegateLoaders.count() > 0) {
            delegatesModel->insertDelegateLoaders(position, delegateLoaders);
            //give the focus to the first
            delegateLoaders.first()->setFocus(true);
        }


    // Gui delegates removed
    } else if (msgType == GuiBusMessages::GUIBusMessageType::GUI_LIST_REMOVE) {
        const QString skillId = doc[QStringLiteral("namespace")].toString();
        if (skillId.isEmpty()) {
            qWarning() << "No skill_id provided in mycroft.gui.list.remove";
            return;
        }

        const int position = doc[QStringLiteral("position")].toInt();
        const int itemsNumber = doc[QStringLiteral("items_number")].toInt();

        //TODO: try with lifecycle managed by the view?
        DelegatesModel *delegatesModel = m_activeSkillsModel->delegatesModelForSkill(skillId);
        if (!delegatesModel) {
            qWarning() << "Error: no delegates model for skill" << skillId;
            return;
        }

        if (position < 0 || position > delegatesModel->rowCount() - 1) {
            qWarning() << "Error: Invalid position in mycroft.gui.list.remove";
            return;
        }

        if (itemsNumber < 0 || itemsNumber > delegatesModel->rowCount()) {
            qWarning() << "Error: Invalid items_number in mycroft.gui.list.remove";
            return;
        }

        delegatesModel->removeRows(position, itemsNumber);

    // Gui delegates moved
    } else if (msgType == GuiBusMessages::GUIBusMessageType::GUI_LIST_MOVE) {

        const QString skillId = doc[QStringLiteral("namespace")].toString();
        if (skillId.isEmpty()) {
            qWarning() << "No skill_id provided in mycroft.gui.list.move";
            return;
        }

        const int from = doc[QStringLiteral("from")].toInt();
        const int to = doc[QStringLiteral("to")].toInt();
        const int itemsNumber = doc[QStringLiteral("items_number")].toInt();

        DelegatesModel *delegatesModel = m_activeSkillsModel->delegatesModelForSkill(skillId);

        if (!delegatesModel) {
            qWarning() << "Error: no delegates model for skill" << skillId;
            return;
        }

        if (from < 0 || from > delegatesModel->rowCount() - 1) {
            qWarning() << "Error: Invalid from position in mycroft.gui.list.move";
            return;
        }
        if (to < 0 || to > delegatesModel->rowCount() - 1) {
            qWarning() << "Error: Invalid to position in mycroft.gui.list.move";
            return;
        }
        if (itemsNumber <= 0 || itemsNumber > delegatesModel->rowCount() - from) {
            qWarning() << "Error: Invalid items_number in mycroft.gui.list.move";
            return;
        }
        delegatesModel->moveRows(QModelIndex(), from, itemsNumber, QModelIndex(), to);
//END GUI MODELS


//TODO: manage nested models?
// NOTE: Currently supports one level of nesting (skill → properties → lists)
// Full nested model support would require recursive model creation for data items containing lists
// and proper lifecycle management for multi-level hierarchies. Deferred for future enhancement.
//BEGIN DATA MODELS
    // Insert new items in an existing list, or creates one under "property"
    // NOTE: Generic data lists (skill custom lists) - different from active_skills list
    } else if (msgType == GuiBusMessages::GUIBusMessageType::SESSION_LIST_INSERT && doc[QStringLiteral("namespace")].toString() != QLatin1String("mycroft.system.active_skills")) {
        const QString skillId = doc[QStringLiteral("namespace")].toString();
        if (skillId.isEmpty()) {
            qWarning() << "No skill_id provided in mycroft.session.list.insert";
            return;
        }
        const QString &property = doc[QStringLiteral("property")].toString();
        if (property.isEmpty()) {
            qWarning() << "Error: Invalid or empty \"property\" in mycroft.session.list.insert";
            return;
        }

        SessionDataMap *map = sessionDataForSkill(skillId);
        SessionDataModel *dm = map->value(property).value<SessionDataModel *>();

        if (!dm) {
            dm = new SessionDataModel(map);
            map->insertAndNotify(property, QVariant::fromValue(dm));
        }

        const int position = doc[QStringLiteral("position")].toInt();

        if (position < 0 || position > dm->rowCount()) {
            qWarning() << "Error: Invalid position in mycroft.session.list.insert";
            return;
        }

        QList<QVariantMap> list = variantListToOrderedMap(doc[QStringLiteral("data")].toVariant().value<QVariantList>());

        if (list.isEmpty()) {
            qWarning() << "Error: invalid data in mycroft.session.list.insert:" << doc[QStringLiteral("data")];
            return;
        }

        dm->insertData(position, list);

    // Updates the value of items in an existing list, Error if under "property" no list exists
    } else if (msgType == GuiBusMessages::GUIBusMessageType::SESSION_LIST_UPDATE) {
        const QString skillId = doc[QStringLiteral("namespace")].toString();
        if (skillId.isEmpty()) {
            qWarning() << "No skill_id provided in mycroft.session.list.update";
            return;
        }
        const QString &property = doc[QStringLiteral("property")].toString();
        if (property.isEmpty()) {
            qWarning() << "Error: Invalid or empty \"property\" in mycroft.session.list.update";
            return;
        }

        SessionDataMap *map = sessionDataForSkill(skillId);
        SessionDataModel *dm = map->value(property).value<SessionDataModel *>();

        if (!dm) {
            qWarning() << "Error: no list model existing under property" << property << "in mycroft.session.list.update";
            return;
        }

        const int position = doc[QStringLiteral("position")].toInt();

        if (position < 0 || position > m_activeSkillsModel->rowCount()) {
            qWarning() << "Error: Invalid position in mycroft.session.list.update";
            return;
        }

        QList<QVariantMap> list = variantListToOrderedMap(doc[QStringLiteral("data")].toVariant().value<QVariantList>());

        if (list.isEmpty()) {
            qWarning() << "Error: invalid data in mycroft.session.list.insert:" << doc[QStringLiteral("data")];
            return;
        }

        dm->updateData(position, list);

    // Moves items within an existing list, Error if under "property" no list exists
    // NOTE: Generic data lists (skill custom lists) - different from active_skills list
    } else if (msgType == GuiBusMessages::GUIBusMessageType::SESSION_LIST_MOVE && doc[QStringLiteral("namespace")].toString() != QLatin1String("mycroft.system.active_skills")) {
        const QString skillId = doc[QStringLiteral("namespace")].toString();
        if (skillId.isEmpty()) {
            qWarning() << "No skill_id provided in mycroft.session.list.update";
            return;
        }
        const QString &property = doc[QStringLiteral("property")].toString();
        if (property.isEmpty()) {
            qWarning() << "Error: Invalid or empty \"property\" in mycroft.session.list.move";
            return;
        }

        SessionDataMap *map = sessionDataForSkill(skillId);
        SessionDataModel *dm = map->value(property).value<SessionDataModel *>();

        if (!dm) {
            qWarning() << "Error: no list model existing under property" << property << "in mycroft.session.list.move";
            return;
        }

        const int from = doc[QStringLiteral("from")].toInt();
        const int to = doc[QStringLiteral("to")].toInt();
        const int itemsNumber = doc[QStringLiteral("items_number")].toInt();

        if (from < 0 || from > dm->rowCount() - 1) {
            qWarning() << "Error: Invalid from position in mycroft.session.list.move";
            return;
        }
        if (to < 0 || to > dm->rowCount()) {
            qWarning() << "Error: Invalid to position in mycroft.session.list.move";
            return;
        }
        if (itemsNumber <= 0 || itemsNumber > dm->rowCount() - from) {
            qWarning() << "Error: Invalid items_number in mycroft.session.list.move";
            return;
        }
        dm->moveRows(QModelIndex(), from, itemsNumber, QModelIndex(), to);

    // Removes items from an existing list, Error if under "property" no list exists
    // NOTE: Generic data lists (skill custom lists) - different from active_skills list
    } else if (msgType == GuiBusMessages::GUIBusMessageType::SESSION_LIST_REMOVE && doc[QStringLiteral("namespace")].toString() != QLatin1String("mycroft.system.active_skills")) {
        const QString skillId = doc[QStringLiteral("namespace")].toString();
        if (skillId.isEmpty()) {
            qWarning() << "No skill_id provided in mycroft.session.list.update";
            return;
        }
        const QString &property = doc[QStringLiteral("property")].toString();
        if (property.isEmpty()) {
            qWarning() << "Error: Invalid or empty \"property\" in mycroft.session.list.move";
            return;
        }

        SessionDataMap *map = sessionDataForSkill(skillId);
        SessionDataModel *dm = map->value(property).value<SessionDataModel *>();

        if (!dm) {
            qWarning() << "Error: no list model existing under property" << property << "in mycroft.session.list.move";
            return;
        }

        const int position = doc[QStringLiteral("position")].toInt();
        const int itemsNumber = doc[QStringLiteral("items_number")].toInt();

        if (position < 0 || position > dm->rowCount() - 1) {
            qWarning() << "Error: Invalid position in mycroft.session.list.remove of mycroft.system.active_skills";
            return;
        }
        if (itemsNumber < 0 || itemsNumber > dm->rowCount() - position) {
            qWarning() << "Error: Invalid items_number in mycroft.session.list.remove of mycroft.system.active_skills";
            return;
        }

        dm->removeRows(position, itemsNumber);
//END DATA MODELS


//BEGIN EVENTS
    // Action triggered from the server
    } else if (msgType == GuiBusMessages::GUIBusMessageType::EVENTS_TRIGGERED) {
        const QString skillOrSystem = doc[QStringLiteral("namespace")].toString();

        if (skillOrSystem.isEmpty()) {
            qWarning() << "No namespace provided for mycroft.events.triggered";
            return;
        }
        // NOTE: Check is intentionally disabled to allow events from skills without active GUI
        // This prevents orphaned events and supports skills that emit events but don't render UI
        // Original check would have been:
        // if (skillOrSystem != QLatin1String("system") && !m_activeSkillsModel->skillIndex(skillOrSystem).isValid()) {
        //     qWarning() << "Invalid skill id passed as namespace for mycroft.events.triggered:" << skillOrSystem;
        //     return;
        // }

        const QString eventName = doc[QStringLiteral("event_name")].toString();
        if (eventName.isEmpty()) {
            qWarning() << "No namespace provided for mycroft.events.triggered";
            return;
        }

        // data can also be empty
        const QVariantMap data = doc[QStringLiteral("data")].toVariant().toMap();

        QList<AbstractDelegate *> delegates;

        if (skillOrSystem == QLatin1String("system")) {
            for (auto *delegatesModel : activeSkills()->delegatesModels()) {
                delegates << delegatesModel->delegates();
            }
        } else {
            DelegatesModel *delegatesModel = activeSkills()->delegatesModelForSkill(skillOrSystem);
            if (delegatesModel) {
                delegates << delegatesModel->delegates();
            }
        }

        // page_gained_focus is special: interests only one single delegate
        if (eventName == QStringLiteral("page_gained_focus")) {
            int pos = data.value(QStringLiteral("number")).toInt();
            if (pos >= 0 && pos < delegates.count()) {
                AbstractDelegate *delegate = delegates[pos];
                delegate->forceActiveFocus((Qt::FocusReason)ServerEventFocusReason);
                emit delegate->guiEvent(eventName, data);
            }
        } else if (eventName == QStringLiteral("mycroft.gui.close.screen")) {
            emit activeSkillClosed();
        } else {
            for (auto *delegate : delegates) {
                emit delegate->guiEvent(eventName, data);
            }
        }
    } else {
        qWarning() << "Unrecognized operation" << type;
    }
//END EVENTS
}

#include "moc_abstractskillview.cpp"

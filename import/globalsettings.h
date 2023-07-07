/*
 * Copyright 2018 by Aditya Mehra <aix.m@outlook.com>
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

#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include <QSettings>
#include <QCoreApplication>
#include <QDebug>

#define SettingPropertyKey(type, name, setOption, signalName, settingKey, defaultValue) \
    inline type name() const { return m_settings.value(settingKey, defaultValue).value<type>(); } \
    inline void setOption (const type &value) { m_settings.setValue(settingKey, value); emit signalName(); qDebug() << "emitted"; }

class QSettings;
class GlobalSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString webSocketAddress READ webSocketAddress WRITE setWebSocketAddress NOTIFY webSocketChanged)
    Q_PROPERTY(bool autoConnect READ autoConnect WRITE setAutoConnect NOTIFY autoConnectChanged)
    Q_PROPERTY(bool useEntryNameSpaceAnimation READ useEntryNameSpaceAnimation WRITE setUseEntryNameSpaceAnimation NOTIFY useEntryNameSpaceAnimationChanged)
    Q_PROPERTY(bool useExitNameSpaceAnimation READ useExitNameSpaceAnimation WRITE setUseExitNameSpaceAnimation NOTIFY useExitNameSpaceAnimationChanged)
    Q_PROPERTY(bool useFocusAnimation READ useFocusAnimation WRITE setUseFocusAnimation NOTIFY useFocusAnimationChanged)
    Q_PROPERTY(bool useDelegateAnimation READ useDelegateAnimation WRITE setUseDelegateAnimation NOTIFY useDelegateAnimationChanged)

public:
    explicit GlobalSettings(QObject *parent=0);

    SettingPropertyKey(QString, webSocketAddress, setWebSocketAddress, webSocketChanged, QStringLiteral("webSocketAddress"), QStringLiteral("ws://0.0.0.0"))

    bool autoConnect() const;
    void setAutoConnect(bool autoconnect);
    bool useEntryNameSpaceAnimation() const;
    void setUseEntryNameSpaceAnimation(bool useEntryNameSpaceAnimation);
    bool useExitNameSpaceAnimation() const;
    void setUseExitNameSpaceAnimation(bool useExitNameSpaceAnimation);
    bool useFocusAnimation() const;
    void setUseFocusAnimation(bool useFocusAnimation);
    bool useDelegateAnimation() const;
    void setUseDelegateAnimation(bool useDelegateAnimation);

Q_SIGNALS:
    void webSocketChanged();
    void autoConnectChanged();
    void useEntryNameSpaceAnimationChanged();
    void useExitNameSpaceAnimationChanged();
    void useFocusAnimationChanged();
    void useDelegateAnimationChanged();

private:
    QSettings m_settings;
};

#endif // GLOBALSETTINGS_H

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

#include <QDebug>
#include <QFile>
#include "globalsettings.h"
#include "controllerconfig.h"

GlobalSettings::GlobalSettings(QObject *parent) :
    QObject(parent)
{
}

bool GlobalSettings::autoConnect() const
{
    return m_settings.value(QStringLiteral("autoConnect"), true).toBool();
}

void GlobalSettings::setAutoConnect(bool autoConnect)
{
    if (GlobalSettings::autoConnect() == autoConnect) {
        return;
    }

    m_settings.setValue(QStringLiteral("autoConnect"), autoConnect);
    emit autoConnectChanged();
}

bool GlobalSettings::usesRemoteTTS() const
{
    return m_settings.value(QStringLiteral("usesRemoteTTS"), false).toBool();
}

void GlobalSettings::setUsesRemoteTTS(bool usesRemoteTTS)
{
    if (GlobalSettings::usesRemoteTTS() == usesRemoteTTS) {
        return;
    }

    m_settings.setValue(QStringLiteral("usesRemoteTTS"), usesRemoteTTS);
    emit usesRemoteTTSChanged();
}

bool GlobalSettings::displayRemoteConfig() const
{
return m_settings.value(QStringLiteral("displayRemoteConfig"), true).toBool();
}

void GlobalSettings::setDisplayRemoteConfig(bool displayRemoteConfig)
{
    if (GlobalSettings::displayRemoteConfig() == displayRemoteConfig) {
        return;
    }

    m_settings.setValue(QStringLiteral("displayRemoteConfig"), displayRemoteConfig);
    emit displayRemoteConfigChanged();
}

bool GlobalSettings::usePTTClient() const
{
    return m_settings.value(QStringLiteral("usePTTClient"), false).toBool();
}

void GlobalSettings::setUsePTTClient(bool usePTTClient)
{
    if (GlobalSettings::usePTTClient() == usePTTClient) {
        return;
    }

    m_settings.setValue(QStringLiteral("usePTTClient"), usePTTClient);
    emit usePTTClientChanged();
}

bool GlobalSettings::useHivemindProtocol() const
{
    return m_settings.value(QStringLiteral("useHivemindProtocol"), false).toBool();
}

void GlobalSettings::setUseHivemindProtocol(bool useHivemindProtocol)
{
    if (GlobalSettings::useHivemindProtocol() == useHivemindProtocol) {
        return;
    }

    m_settings.setValue(QStringLiteral("useHivemindProtocol"), useHivemindProtocol);
    emit useHivemindProtocolChanged();
}

bool GlobalSettings::useEntryNameSpaceAnimation() const
{
    return m_settings.value(QStringLiteral("useEntryNameSpaceAnimation"), true).toBool();
}

void GlobalSettings::setUseEntryNameSpaceAnimation(bool useEntryNameSpaceAnimation)
{
    if (GlobalSettings::useEntryNameSpaceAnimation() == useEntryNameSpaceAnimation) {
        return;
    }

    m_settings.setValue(QStringLiteral("useEntryNameSpaceAnimation"), useEntryNameSpaceAnimation);
    emit useEntryNameSpaceAnimationChanged();
}

bool GlobalSettings::useExitNameSpaceAnimation() const
{
    return m_settings.value(QStringLiteral("useExitNameSpaceAnimation"), true).toBool();
}

void GlobalSettings::setUseExitNameSpaceAnimation(bool useExitNameSpaceAnimation)
{
    if (GlobalSettings::useExitNameSpaceAnimation() == useExitNameSpaceAnimation) {
        return;
    }

    m_settings.setValue(QStringLiteral("useExitNameSpaceAnimation"), useExitNameSpaceAnimation);
    emit useExitNameSpaceAnimationChanged();
}

bool GlobalSettings::useFocusAnimation() const
{
    return m_settings.value(QStringLiteral("useFocusAnimation"), true).toBool();
}

void GlobalSettings::setUseFocusAnimation(bool useFocusAnimation)
{
    if (GlobalSettings::useFocusAnimation() == useFocusAnimation) {
        return;
    }

    m_settings.setValue(QStringLiteral("useFocusAnimation"), useFocusAnimation);
    emit useFocusAnimationChanged();
}

bool GlobalSettings::useDelegateAnimation() const
{
    return m_settings.value(QStringLiteral("useDelegateAnimation"), true).toBool();
}

void GlobalSettings::setUseDelegateAnimation(bool useDelegateAnimation)
{
    if (GlobalSettings::useDelegateAnimation() == useDelegateAnimation) {
        return;
    }

    m_settings.setValue(QStringLiteral("useDelegateAnimation"), useDelegateAnimation);
    emit useDelegateAnimationChanged();
}
/*
 * Copyright 2023 by Aditya Mehra <aix.m@outlook.com>
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

#include coloroverlay.h
#include <QtQuick/QQuickPaintedItem>
#include <QImage>

ColorOverlay::ColorOverlay(QQuickItem *parent)
    : QQuickPaintedItem(parent), m_source(nullptr), m_strength(1.0)
{
}

QQuickItem *ColorOverlay::source() const
{
    return m_source;
}

void ColorOverlay::setSource(QQuickItem *source)
{
    if (m_source != source) {
        m_source = source;
        update();
        emit sourceChanged();
    }
}

void ColorOverlay::paint(QPainter *painter)
{
    if (!m_source)
        return;

    QImage sourceImage(m_source->width(), m_source->height(), QImage::Format_ARGB32_Premultiplied);
    QPainter sourcePainter(&sourceImage);
    m_source->paint(&sourcePainter);

    QImage colorOverlayImage(m_source->width(), m_source->height(), QImage::Format_ARGB32_Premultiplied);
    colorOverlayImage.fill(m_color);

    QPainter colorOverlayPainter(&colorOverlayImage);
    colorOverlayPainter.setCompositionMode(QPainter::CompositionMode_Multiply);
    colorOverlayPainter.drawImage(0, 0, sourceImage);

    painter->drawImage(0, 0, colorOverlayImage);
    painter->setOpacity(m_strength);
}
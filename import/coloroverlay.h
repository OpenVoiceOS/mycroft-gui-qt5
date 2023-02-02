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

#include <QtQuick/QQuickPaintedItem>
#include <QImage>
#include <QPainter>

class ColorOverlay : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QColor color MEMBER m_color NOTIFY colorChanged)
    Q_PROPERTY(qreal strength MEMBER m_strength NOTIFY strengthChanged)
    Q_PROPERTY(QQuickItem *source READ source WRITE setSource NOTIFY sourceChanged)

public:
    ColorOverlay(QQuickItem *parent = nullptr);

    QQuickItem *source() const;
    void setSource(QQuickItem *source);

    void paint(QPainter *painter) override;

signals:
    void sourceChanged();
    void colorChanged();
    void strengthChanged();

private:
    QQuickItem *m_source;
    QColor m_color;
    qreal m_strength;
};
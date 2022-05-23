/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2017 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef QNAVITQUICK_H
#define QNAVITQUICK_H
class QNavitQuick;
#include <QColor>
#include <QtQuick/QQuickPaintedItem>

#include "graphics_qt5.h"

class QNavitQuick : public QQuickPaintedItem {
    Q_OBJECT
public:
    void paint(QPainter* painter);
    QNavitQuick(QQuickItem* parent = 0);


    Q_INVOKABLE void setGraphicContext(GraphicsPriv* gp);

protected:
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);
    virtual void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry);
    virtual void mouseEvent(int pressed, QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void wheelEvent(QWheelEvent* event);

private:
    struct graphics_priv* graphics_priv;
};

#endif

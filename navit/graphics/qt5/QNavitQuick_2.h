/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2019 Navit Team
 * Copyright (C) 2019 Viktor Verebelyi <me@viktorgino.me>
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

#ifndef QNAVITQUICK2_H
#define QNAVITQUICK2_H

#include <QColor>
#include <QtQuick/QQuickPaintedItem>
#include <QVariant>
#include <QVariantMap>
#include <QtMath>

extern "C" {
#include "config.h"
#include "item.h" /* needs to be first, as attr.h depends on it */
#include "navit.h"
}

#include "graphics_qt5.h"
#include "navitinstance.h"
#include "navithelper.h"

class QNavitQuick_2 : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(int  pitch           MEMBER m_pitch          WRITE setPitch          NOTIFY propertiesChanged)
    Q_PROPERTY(bool autoZoom        MEMBER m_autoZoom       WRITE setAutozoom       NOTIFY propertiesChanged)
    Q_PROPERTY(bool followVehicle   MEMBER m_followVehicle  WRITE setFollowVehicle  NOTIFY propertiesChanged)
    Q_PROPERTY(bool tracking        MEMBER m_tracking       WRITE setTracking       NOTIFY propertiesChanged)
    Q_PROPERTY(int  orientation     MEMBER m_orientation    WRITE setOrientation    NOTIFY propertiesChanged)
    Q_PROPERTY(long zoomLevel       READ   getZoomLevel                             NOTIFY zoomLevelChanged)
    Q_PROPERTY(NavitInstance *navit READ   navitInstance    WRITE setNavitInstance)

public:
    void paint(QPainter* painter);
    QNavitQuick_2(QQuickItem* parent = 0);

    Q_INVOKABLE void zoomIn(int zoomLevel);
    Q_INVOKABLE void zoomOut(int zoomLevel);
    Q_INVOKABLE void zoomInToPoint(int zoomLevel, int x, int y);
    Q_INVOKABLE void zoomOutFromPoint(int zoomLevel, int x, int y);
    Q_INVOKABLE void zoomToRoute();
    Q_INVOKABLE void mapMove(int originX, int originY, int destinationX, int destinationY);
    Q_INVOKABLE void addBookmark(QString label, int x, int y);
    Q_INVOKABLE QVariantMap positionToCoordinates(int x, int y);
    Q_INVOKABLE QString getAddress(int x, int y);
    Q_INVOKABLE void centerOnPosition();

    void setPitch(int pitch);
    void setAutozoom(bool autoZoom);
    void setFollowVehicle(bool followVehicle, int followTime = 0);
    void setTracking(bool tracking);
    void setOrientation(int orientation);
    NavitInstance *navitInstance();
    void setNavitInstance(NavitInstance *navit);
    int getZoomLevel() {
        return m_zoomLevel;
    }

    static void attributeCallbackHandler(QNavitQuick_2 *navitGraph, navit *this_, attr *attr);
    void attributeCallback(attr *attr);
protected:
    virtual void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void wheelEvent(QWheelEvent* event);

private:
    struct graphics_priv* graphics_priv;
    int m_pitch;
    bool m_autoZoom;
    bool m_followVehicle;
    bool m_tracking;
    int m_orientation;
    void setNavitNumProperty(enum attr_type type, int value);
    int getNavitNumProperty(enum attr_type type);
    void paintOverlays(QPainter* painter, struct graphics_priv* gp, QPaintEvent* event);
    NavitInstance *m_navitInstance;
    long m_zoomLevel = 0;
    int m_moveX;
    int m_moveY;
    int m_originX;
    int m_originY;

    void updateZoomLevel();
signals:
    void leftButtonClicked(QPoint position);
    void rightButtonClicked(QPoint position);
    void positionChanged(QMouseEvent* mouse);
    void pressAndHold(QMouseEvent* mouse);
    void propertiesChanged();
    void zoomLevelChanged();
};

#endif

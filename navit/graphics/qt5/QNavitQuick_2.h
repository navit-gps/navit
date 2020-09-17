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
    Q_PROPERTY(bool northing        MEMBER m_northing       WRITE setNorthing       NOTIFY propertiesChanged)
    Q_PROPERTY(int  pitch           MEMBER m_pitch          WRITE setPitch          NOTIFY propertiesChanged)
    Q_PROPERTY(bool autoZoom        MEMBER m_autoZoom       WRITE setAutozoom       NOTIFY propertiesChanged)
    Q_PROPERTY(bool followVehicle   MEMBER m_followVehicle  WRITE setFollowVehicle  NOTIFY propertiesChanged)
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

    void setNorthing(bool northing);
    void setPitch(int pitch);
    void setAutozoom(bool autoZoom);
    void setFollowVehicle(bool followVehicle);
    void setOrientation(int orientation);
    NavitInstance *navitInstance();
    void setNavitInstance(NavitInstance *navit);
    int getZoomLevel() {
        return m_zoomLevel;
    }
protected:
    virtual void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry);

private:
    struct graphics_priv* graphics_priv;
    bool m_northing;
    int m_pitch;
    bool m_autoZoom;
    bool m_followVehicle;
    int m_orientation;
    void setNavitNumProperty(enum attr_type type, int value);
    int getNavitNumProperty(enum attr_type type);
    NavitInstance *m_navitInstance;
    long m_zoomLevel = 0;

    void updateZoomLevel();
signals:
    void mouseLeftButtonClicked();
    void mouseRightButtonClicked();
    void pressAndHold(QMouseEvent* mouse);
    void propertiesChanged();
    void zoomLevelChanged();
};

#endif

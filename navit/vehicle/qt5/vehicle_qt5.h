/** @file vehicle_null.c
 * @brief null uses dbus signals
 *
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
 *
 * @Author Tim Niemeyer <reddog@mastersword.de>
 * @date 2008-2009
 */
// style with: clang-format -style=WebKit -i *

#ifndef __vehicle_qt5_h
#define __vehicle_qt5_h

#include <QGeoPositionInfoSource>
#include <QGeoSatelliteInfoSource>
#include <QObject>
#include <QStringList>
extern "C" {
#include "item.h" /* needs to be on to as attr.h depends on it */

#include "attr.h"
#include "callback.h"
#include "coord.h"
}

class QNavitGeoReceiver;
struct vehicle_priv {
    struct callback_list* cbl;
    struct coord_geo geo;
    double speed;
    double direction;
    double height;
    double radius;
    int fix_type;
    time_t fix_time;
    char fixiso8601[128];
    int sats;
    int sats_used;
    int have_coords;
    struct attr** attrs;

    QGeoPositionInfoSource* source;
    QGeoSatelliteInfoSource* satellites;
    QNavitGeoReceiver* receiver;
};

class QNavitGeoReceiver : public QObject {
    Q_OBJECT
public:
    QNavitGeoReceiver(QObject* parent, struct vehicle_priv* c);
public slots:
    void positionUpdated(const QGeoPositionInfo& info);
    void satellitesInUseUpdated(const QList<QGeoSatelliteInfo>& satellites);
    void satellitesInViewUpdated(const QList<QGeoSatelliteInfo>& satellites);

private:
    struct vehicle_priv* priv;
};
#endif

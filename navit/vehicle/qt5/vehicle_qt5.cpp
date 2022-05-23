/*
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

extern "C" {
#include "config.h"
#include "item.h" /* needs to be first as attr.h depends on it */

#include "attr.h"
#include "coord.h"
#include "debug.h"
#include "plugin.h"
#include "vehicle.h"
}
#include <glib.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "vehicle_qt5.h"
// #include "vehicle_qt5.moc"
#include <QDateTime>

/**
 * @defgroup vehicle-qt5 Vehicle QT5
 * @ingroup vehicle-plugins
 * @brief The Vehicle to gain position data from qt5
 *
 * @{
 */

QNavitGeoReceiver::QNavitGeoReceiver(QObject* parent, struct vehicle_priv* c)
    : QObject(parent) {
    priv = c;
    if (priv->source != NULL) {
        connect(priv->source, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(positionUpdated(QGeoPositionInfo)));
    }
    if (priv->satellites != NULL) {
        connect(priv->satellites, SIGNAL(satellitesInUseUpdated(const QList<QGeoSatelliteInfo>&)), this,
                SLOT(satellitesInUseUpdated(const QList<QGeoSatelliteInfo>&)));
        connect(priv->satellites, SIGNAL(satellitesInViewUpdated(const QList<QGeoSatelliteInfo>&)), this,
                SLOT(satellitesInViewUpdated(const QList<QGeoSatelliteInfo>&)));
    }
}
void QNavitGeoReceiver::satellitesInUseUpdated(const QList<QGeoSatelliteInfo>& sats) {
    dbg(lvl_debug, "Sats in use: %d", sats.count());
    priv->sats_used = sats.count();
    callback_list_call_attr_0(priv->cbl, attr_position_sats_used);
}

void QNavitGeoReceiver::satellitesInViewUpdated(const QList<QGeoSatelliteInfo>& sats) {
    dbg(lvl_debug, "Sats in view: %d", sats.count());
    priv->sats = sats.count();
    callback_list_call_attr_0(priv->cbl, attr_position_qual);
}

void QNavitGeoReceiver::positionUpdated(const QGeoPositionInfo& info) {
    /* ignore stale view */
    if (info.coordinate().isValid()) {
        if (info.timestamp().toUTC().secsTo(QDateTime::currentDateTimeUtc()) > 20) {
            dbg(lvl_debug, "Ignoring old FIX");
            return;
        }
    }

    if (info.hasAttribute(QGeoPositionInfo::HorizontalAccuracy)) {
        dbg(lvl_debug, "Horizontal acc (%f)", info.attribute(QGeoPositionInfo::HorizontalAccuracy));
        priv->radius = info.attribute(QGeoPositionInfo::HorizontalAccuracy);
        callback_list_call_attr_0(priv->cbl, attr_position_radius);
    }
    if (info.hasAttribute(QGeoPositionInfo::GroundSpeed)) {
        dbg(lvl_debug, "Got ground speed (%f)", info.attribute(QGeoPositionInfo::GroundSpeed));
        priv->speed = info.attribute(QGeoPositionInfo::GroundSpeed) * 3.6;
        callback_list_call_attr_0(priv->cbl, attr_position_speed);
    }
    if (info.hasAttribute(QGeoPositionInfo::Direction)) {
        dbg(lvl_debug, "Direction (%f)", info.attribute(QGeoPositionInfo::Direction));
        priv->direction = info.attribute(QGeoPositionInfo::Direction);
        callback_list_call_attr_0(priv->cbl, attr_position_direction);
    }

    switch (info.coordinate().type()) {
    case QGeoCoordinate::Coordinate3D:
        priv->fix_type = 2;
        break;
    case QGeoCoordinate::Coordinate2D:
        priv->fix_type = 1;
        break;
    case QGeoCoordinate::InvalidCoordinate:
        priv->fix_type = 0;
        break;
    }

    if (info.coordinate().isValid()) {
        dbg(lvl_debug, "Got valid coordinate (lat %f, lon %f)", info.coordinate().latitude(), info.coordinate().longitude());
        priv->geo.lat = info.coordinate().latitude();
        priv->geo.lng = info.coordinate().longitude();
        if (info.coordinate().type() == QGeoCoordinate::Coordinate3D) {
            dbg(lvl_debug, "Got valid altitude (alt %f)", info.coordinate().altitude());
            priv->height = info.coordinate().altitude();
        }
        // dbg(lvl_debug, "Time %s", info.timestamp().toUTC().toString().toLatin1().data());
        priv->fix_time = info.timestamp().toUTC().toTime_t();
        callback_list_call_attr_0(priv->cbl, attr_position_coord_geo);
        if (priv->have_coords != attr_position_valid_valid) {
            priv->have_coords = attr_position_valid_valid;
            callback_list_call_attr_0(priv->cbl, attr_position_valid);
        }
    } else {
        dbg(lvl_debug, "Got invalid coordinate");
        callback_list_call_attr_0(priv->cbl, attr_position_coord_geo);
        if (priv->have_coords != attr_position_valid_invalid) {
            priv->have_coords = attr_position_valid_invalid;
            callback_list_call_attr_0(priv->cbl, attr_position_valid);
        }
    }
}

/**
 * @brief Free the null_vehicle
 *
 * @param priv
 * @returns nothing
 */
static void vehicle_qt5_destroy(struct vehicle_priv* priv) {
    dbg(lvl_debug, "enter");
    if (priv->receiver != NULL)
        delete priv->receiver;
    if (priv->source != NULL)
        delete priv->source;
    g_free(priv);
}

/**
 * @brief Provide the outside with information
 *
 * @param priv
 * @param type TODO: What can this be?
 * @param attr
 * @returns true/false
 */
static int vehicle_qt5_position_attr_get(struct vehicle_priv* priv,
        enum attr_type type, struct attr* attr) {
    struct attr* active = NULL;
    dbg(lvl_debug, "enter %s", attr_to_name(type));
    switch (type) {
    case attr_position_valid:
        attr->u.num = priv->have_coords;
        break;
    case attr_position_fix_type:
        attr->u.num = priv->fix_type;
        break;
    case attr_position_height:
        attr->u.numd = &priv->height;
        break;
    case attr_position_speed:
        attr->u.numd = &priv->speed;
        break;
    case attr_position_direction:
        attr->u.numd = &priv->direction;
        break;
    case attr_position_radius:
        attr->u.numd = &priv->radius;
        break;
    case attr_position_qual:
        attr->u.num = priv->sats;
        break;
    case attr_position_sats_used:
        attr->u.num = priv->sats_used;
        break;
    case attr_position_coord_geo:
        attr->u.coord_geo = &priv->geo;
        if (priv->have_coords != attr_position_valid_valid)
            return 0;
        break;
    case attr_position_time_iso8601:
        if (priv->fix_time) {
            struct tm tm;
            if (gmtime_r(&priv->fix_time, &tm)) {
                strftime(priv->fixiso8601, sizeof(priv->fixiso8601),
                         "%Y-%m-%dT%TZ", &tm);
                attr->u.str = priv->fixiso8601;
            } else {
                priv->fix_time = 0;
                return 0;
            }
            // dbg(lvl_debug,"Fix Time: %s", priv->fixiso8601);
        } else {
            // dbg(lvl_debug,"Fix Time: 0");
            return 0;
        }
        break;

    case attr_active:
        active = attr_search(priv->attrs, attr_active);
        if (active != NULL) {
            attr->u.num = active->u.num;
            return 1;
        } else
            return 0;
        break;

    default:
        return 0;
    }
    dbg(lvl_debug, "ok");
    attr->type = type;
    return 1;
}

static int vehicle_qt5_set_attr(struct vehicle_priv* priv, struct attr* attr) {
    switch (attr->type) {
    case attr_position_speed:
        priv->speed = *attr->u.numd;
        break;
    case attr_position_direction:
        priv->direction = *attr->u.numd;
        break;
    case attr_position_coord_geo:
        priv->geo = *attr->u.coord_geo;
        if (priv->have_coords != attr_position_valid_valid) {
            priv->have_coords = attr_position_valid_valid;
            callback_list_call_attr_0(priv->cbl, attr_position_valid);
        }
        break;
    default:
        break;
    }
    callback_list_call_attr_0(priv->cbl, attr->type);
    return 1;
}

struct vehicle_methods vehicle_null_methods = {
    vehicle_qt5_destroy,
    vehicle_qt5_position_attr_get,
    vehicle_qt5_set_attr,
};

/**
 * @brief Create null_vehicle
 *
 * @param meth
 * @param cbl
 * @param attrs
 * @returns vehicle_priv
 */
static struct vehicle_priv* vehicle_qt5_new_qt5(struct vehicle_methods* meth,
        struct callback_list* cbl,
        struct attr** attrs) {
    struct vehicle_priv* ret;

    dbg(lvl_debug, "enter");
    ret = g_new0(struct vehicle_priv, 1);
    ret->cbl = cbl;
    *meth = vehicle_null_methods;
    ret->attrs = attrs;
    ret->source = QGeoPositionInfoSource::createDefaultSource(NULL);
    ret->satellites = QGeoSatelliteInfoSource::createDefaultSource(NULL);
    if (ret->source == NULL) {
        dbg(lvl_error, "Got NO QGeoPositionInfoSource");
    } else {
        dbg(lvl_debug, "Using %s", ret->source->sourceName().toLatin1().data());
        ret->receiver = new QNavitGeoReceiver(NULL, ret);
        if (ret->satellites != NULL) {
            ret->satellites->setUpdateInterval(1000);
            ret->satellites->startUpdates();
        }
        ret->source->setUpdateInterval(500);
        ret->source->startUpdates();
    }
    dbg(lvl_debug, "return");
    return ret;
}

/**
 * @brief register vehicle_null
 *
 * @returns nothing
 */
void plugin_init(void) {
    dbg(lvl_debug, "enter");
    plugin_register_category_vehicle("qt5", vehicle_qt5_new_qt5);
}

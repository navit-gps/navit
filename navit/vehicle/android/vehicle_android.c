/*
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
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
 */

#include <config.h>
#include <string.h>
#include <glib.h>
#include <math.h>
#include <time.h>
#include "debug.h"
#include "callback.h"
#include "plugin.h"
#include "coord.h"
#include "item.h"
#include "android.h"
#include "vehicle.h"

/**
 * @defgroup vehicle-android Vehicle Android
 * @ingroup vehicle-plugins
 * @brief The Vehicle to gain position data from android.
 * @author Tim Niemeyer <reddog@mastersword.de>
 * @date 2008-2009
 *
 */

struct vehicle_priv {
    struct callback_list *cbl;
    struct coord_geo geo;      /**< The last known position of the vehicle **/
    double speed;              /**< Speed in km/h **/
    double direction;          /**< Bearing in degrees **/
    double height;             /**< Elevation in meters **/
    double radius;             /**< Position accuracy in meters **/
    int fix_type;              /**< Type of last fix (1 = valid, 0 = invalid) **/
    time_t fix_time;           /**< Timestamp of last fix (not used) **/
    char fixiso8601[128];      /**< Timestamp of last fix in ISO 8601 format **/
    int sats;                  /**< Number of satellites in view **/
    int sats_used;             /**< Number of satellites used in fix **/
    int valid;                 /**< Whether the vehicle coordinates in {@code geo} are valid **/
    struct attr ** attrs;
    struct callback *pcb;      /**< The callback function for position updates **/
    struct callback *scb;      /**< The callback function for status updates **/
    struct callback *fcb;      /**< The callback function for fix status updates **/
    jclass NavitVehicleClass;  /**< The {@code NavitVehicle} class **/
    jobject NavitVehicle;      /**< An instance of {@code NavitVehicle} **/
    jclass LocationClass;      /**< Android's {@code Location} class **/
    jmethodID Location_getLatitude, Location_getLongitude, Location_getSpeed, Location_getBearing, Location_getAltitude,
              Location_getTime, Location_getAccuracy;
};

/**
 * @brief Free the android_vehicle
 *
 * @param priv vehicle_priv structure for the vehicle
 * @returns nothing
 */
static void vehicle_android_destroy(struct vehicle_priv *priv) {
    dbg(lvl_debug,"enter");
    g_free(priv);
}

/**
 * @brief Retrieves a vehicle attribute.
 *
 * @param priv vehicle_priv structure for the vehicle
 * @param type The attribute type to retrieve
 * @param attr Points to an attr structure that will receive the attribute data
 * @returns True for success, false for failure
 */
static int vehicle_android_position_attr_get(struct vehicle_priv *priv,
        enum attr_type type, struct attr *attr) {
    dbg(lvl_debug,"enter %s",attr_to_name(type));
    switch (type) {
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
        if (priv->valid == attr_position_valid_invalid)
            return 0;
        break;
    case attr_position_time_iso8601:
        attr->u.str=priv->fixiso8601;
        break;
    case attr_position_valid:
        attr->u.num = priv->valid;
        break;
    default:
        return 0;
    }
    dbg(lvl_debug,"ok");
    attr->type = type;
    return 1;
}

struct vehicle_methods vehicle_android_methods = {
    vehicle_android_destroy,
    vehicle_android_position_attr_get,
};

/**
 * @brief Called when a new position has been reported
 *
 * This function is called by {@code NavitLocationListener} upon receiving a new {@code Location}.
 *
 * @param v The {@code struct_vehicle_priv} for the vehicle
 * @param location A {@code Location} object describing the new position
 */
static void vehicle_android_position_callback(struct vehicle_priv *v, jobject location) {
    time_t tnow;
    struct tm *tm;
    dbg(lvl_debug,"enter");

    v->geo.lat = (*jnienv)->CallDoubleMethod(jnienv, location, v->Location_getLatitude);
    v->geo.lng = (*jnienv)->CallDoubleMethod(jnienv, location, v->Location_getLongitude);
    v->speed = (*jnienv)->CallFloatMethod(jnienv, location, v->Location_getSpeed)*3.6;
    v->direction = (*jnienv)->CallFloatMethod(jnienv, location, v->Location_getBearing);
    v->height = (*jnienv)->CallDoubleMethod(jnienv, location, v->Location_getAltitude);
    v->radius = (*jnienv)->CallFloatMethod(jnienv, location, v->Location_getAccuracy);
    tnow=(*jnienv)->CallLongMethod(jnienv, location, v->Location_getTime)/1000;
    tm = gmtime(&tnow);
    strftime(v->fixiso8601, sizeof(v->fixiso8601), "%Y-%m-%dT%TZ", tm);
    dbg(lvl_debug,"lat %f lon %f time %s",v->geo.lat,v->geo.lng,v->fixiso8601);
    if (v->valid != attr_position_valid_valid) {
        v->valid = attr_position_valid_valid;
        callback_list_call_attr_0(v->cbl, attr_position_valid);
    }
    callback_list_call_attr_0(v->cbl, attr_position_coord_geo);
}

/**
 * @brief Called when a new GPS status has been reported
 *
 * This function is called by {@code NavitLocationListener} upon receiving a new {@code GpsStatus}.
 *
 * Note that {@code sats_used} should not be used to determine whether the vehicle's position is valid:
 * some devices report non-zero numbers even when they do not have a fix. Position validity should be
 * determined in {@code vehicle_android_fix_callback} (an invalid fix type means we have lost the fix)
 * and {@code vehicle_android_position_callback} (receiving a position means we have a fix).
 *
 * @param v The {@code struct_vehicle_priv} for the vehicle
 * @param sats_in_view The number of satellites in view
 * @param sats_used The number of satellites currently used to determine the position
 */
static void vehicle_android_status_callback(struct vehicle_priv *v, int sats_in_view, int sats_used) {
    if (v->sats != sats_in_view) {
        v->sats = sats_in_view;
        callback_list_call_attr_0(v->cbl, attr_position_qual);
    }
    if (v->sats_used != sats_used) {
        v->sats_used = sats_used;
        callback_list_call_attr_0(v->cbl, attr_position_sats_used);
    }
}

/**
 * @brief Called when a change in GPS fix status has been reported
 *
 * This function is called by {@code NavitLocationListener} upon receiving a new {@code android.location.GPS_FIX_CHANGE} broadcast.
 *
 * @param v The {@code struct_vehicle_priv} for the vehicle
 * @param fix_type The fix type (1 = valid, 0 = invalid)
 */
static void vehicle_android_fix_callback(struct vehicle_priv *v, int fix_type) {
    if (v->fix_type != fix_type) {
        v->fix_type = fix_type;
        callback_list_call_attr_0(v->cbl, attr_position_fix_type);
        if (!fix_type && (v->valid == attr_position_valid_valid)) {
            v->valid = attr_position_valid_extrapolated_time;
            callback_list_call_attr_0(v->cbl, attr_position_valid);
        }
    }
}

/**
 * @brief Initializes an Android vehicle
 *
 * @return True on success, false on failure
 */
static int vehicle_android_init(struct vehicle_priv *ret) {
    jmethodID cid;

    if (!android_find_class_global("android/location/Location", &ret->LocationClass))
        return 0;
    if (!android_find_method(ret->LocationClass, "getLatitude", "()D", &ret->Location_getLatitude))
        return 0;
    if (!android_find_method(ret->LocationClass, "getLongitude", "()D", &ret->Location_getLongitude))
        return 0;
    if (!android_find_method(ret->LocationClass, "getSpeed", "()F", &ret->Location_getSpeed))
        return 0;
    if (!android_find_method(ret->LocationClass, "getBearing", "()F", &ret->Location_getBearing))
        return 0;
    if (!android_find_method(ret->LocationClass, "getAltitude", "()D", &ret->Location_getAltitude))
        return 0;
    if (!android_find_method(ret->LocationClass, "getTime", "()J", &ret->Location_getTime))
        return 0;
    if (!android_find_method(ret->LocationClass, "getAccuracy", "()F", &ret->Location_getAccuracy))
        return 0;
    if (!android_find_class_global("org/navitproject/navit/NavitVehicle", &ret->NavitVehicleClass))
        return 0;
    dbg(lvl_debug,"at 3");
    cid = (*jnienv)->GetMethodID(jnienv, ret->NavitVehicleClass, "<init>", "(Landroid/content/Context;JJJ)V");
    if (cid == NULL) {
        dbg(lvl_error,"no method found");
        return 0; /* exception thrown */
    }
    ret->NavitVehicle=(*jnienv)->NewObject(jnienv, ret->NavitVehicleClass, cid, android_activity,
                                           (jlong) ret->pcb, (jlong) ret->scb, (jlong) ret->fcb);
    dbg(lvl_debug,"result=%p",ret->NavitVehicle);
    if (!ret->NavitVehicle)
        return 0;
    if (ret->NavitVehicle)
        ret->NavitVehicle = (*jnienv)->NewGlobalRef(jnienv, ret->NavitVehicle);

    return 1;
}

/**
 * @brief Create android_vehicle
 *
 * @param meth
 * @param cbl
 * @param attrs
 * @returns vehicle_priv
 */
static struct vehicle_priv *vehicle_android_new_android(struct vehicle_methods *meth,
        struct callback_list *cbl,
        struct attr **attrs) {
    struct vehicle_priv *ret;

    dbg(lvl_debug, "enter");
    ret = g_new0(struct vehicle_priv, 1);
    ret->cbl = cbl;
    ret->pcb = callback_new_1(callback_cast(vehicle_android_position_callback), ret);
    ret->scb = callback_new_1(callback_cast(vehicle_android_status_callback), ret);
    ret->fcb = callback_new_1(callback_cast(vehicle_android_fix_callback), ret);
    ret->valid = attr_position_valid_invalid;
    ret->sats = 0;
    ret->sats_used = 0;
    *meth = vehicle_android_methods;
    vehicle_android_init(ret);
    dbg(lvl_debug, "return");
    return ret;
}

/**
 * @brief register vehicle_android
 *
 * @returns nothing
 */
void plugin_init(void) {
    dbg(lvl_debug, "enter");
    plugin_register_category_vehicle("android", vehicle_android_new_android);
}

/** @} */

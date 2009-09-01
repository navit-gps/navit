/**
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
 */

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <time.h>
#include "config.h"
#include "debug.h"
#include "coord.h"
#include "item.h"
#include "log.h"
#include "callback.h"
#include "plugin.h"
#include "vehicle.h"
#include "util.h"

struct vehicle {
	struct vehicle_priv *priv;
	struct vehicle_methods meth;
	struct callback_list *cbl;
	struct log *nmea_log, *gpx_log;
	struct attr **attrs;
};

static void
vehicle_log_nmea(struct vehicle *this_, struct log *log)
{
	struct attr pos_attr;
	if (!this_->meth.position_attr_get)
		return;
	if (!this_->meth.position_attr_get(this_->priv, attr_position_nmea, &pos_attr))
		return;
	log_write(log, pos_attr.u.str, strlen(pos_attr.u.str));
}

static void
vehicle_log_gpx(struct vehicle *this_, struct log *log)
{
	struct attr attr,*attrp;
	enum attr_type *attr_types;
	char *logstr;
	char *extensions="\t<extensions>\n";

	if (!this_->meth.position_attr_get)
		return;
	if (log_get_attr(log, attr_attr_types, &attr, NULL))
		attr_types=attr.u.attr_types;
	else
		attr_types=NULL;
	if (!this_->meth.position_attr_get(this_->priv, attr_position_coord_geo, &attr))
		return;
	logstr=g_strdup_printf("<trkpt lat=\"%f\" lon=\"%f\">\n",attr.u.coord_geo->lat,attr.u.coord_geo->lng);
	if (attr_types && attr_types_contains_default(attr_types, attr_position_time_iso8601, 0)) {
		if (this_->meth.position_attr_get(this_->priv, attr_position_time_iso8601, &attr)) {
			logstr=g_strconcat_printf(logstr,"\t<time>%s</time>\n",attr.u.str);
		} else {
			char *timep = current_to_iso8601();
			logstr=g_strconcat_printf(logstr,"\t<time>%s</time>\n",timep);
			g_free(timep);
		}
	}
	if (attr_types_contains_default(attr_types, attr_position_direction,0) && this_->meth.position_attr_get(this_->priv, attr_position_direction, &attr))
		logstr=g_strconcat_printf(logstr,"\t<course>%.1f</course>\n",*attr.u.numd);
	if (attr_types_contains_default(attr_types, attr_position_speed, 0) && this_->meth.position_attr_get(this_->priv, attr_position_speed, &attr))
		logstr=g_strconcat_printf(logstr,"\t<speed>%.2f</speed>\n",*attr.u.numd);
	if (attr_types_contains_default(attr_types, attr_profilename, 0) && (attrp=attr_search(this_->attrs, NULL, attr_profilename))) {
		logstr=g_strconcat_printf(logstr,"%s\t\t<navit:profilename>%s</navit:profilename>\n",extensions,attrp->u.str);
		extensions="";
	}
	if (attr_types_contains_default(attr_types, attr_position_radius, 0) && this_->meth.position_attr_get(this_->priv, attr_position_radius, &attr)) {
		logstr=g_strconcat_printf(logstr,"%s\t\t<navit:radius>%.2f</navit:radius>\n",extensions,*attr.u.numd);
		extensions="";
	}
	if (!strcmp(extensions,"")) {
		logstr=g_strconcat_printf(logstr,"\t</extensions>\n");
	}
	logstr=g_strconcat_printf(logstr,"</trkpt>\n");
	callback_list_call_attr_1(this_->cbl, attr_log_gpx, &logstr);
	log_write(log, logstr, strlen(logstr));
	g_free(logstr);
}

static void
vehicle_log_textfile(struct vehicle *this_, struct log *log)
{
	struct attr pos_attr;
	char *logstr;
	if (!this_->meth.position_attr_get)
		return;
	if (!this_->meth.position_attr_get(this_->priv, attr_position_coord_geo, &pos_attr))
		return;
	logstr=g_strdup_printf("%f %f type=trackpoint\n", pos_attr.u.coord_geo->lng, pos_attr.u.coord_geo->lat);
	callback_list_call_attr_1(this_->cbl, attr_log_textfile, &logstr);
	log_write(log, logstr, strlen(logstr));
}

static int
vehicle_add_log(struct vehicle *this_, struct log *log)
{
	struct callback *cb;
	struct attr type_attr;
	if (!log_get_attr(log, attr_type, &type_attr, NULL))
                return 1;

	if (!strcmp(type_attr.u.str, "nmea")) {
		cb=callback_new_2(callback_cast(vehicle_log_nmea), this_, log);
	} else if (!strcmp(type_attr.u.str, "gpx")) {
		char *header = "<?xml version='1.0' encoding='UTF-8'?>\n"
			"<gpx version='1.1' creator='Navit http://navit.sourceforge.net'\n"
			"     xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'\n"
			"     xmlns:navit='http://www.navit-project.org/schema/navit'\n"
			"     xmlns='http://www.topografix.com/GPX/1/1'\n"
			"     xsi:schemaLocation='http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd'>\n"
			"<trk>\n"
			"<trkseg>\n";
		char *trailer = "</trkseg>\n</trk>\n</gpx>\n";
		log_set_header(log, header, strlen(header));
		log_set_trailer(log, trailer, strlen(trailer));
		cb=callback_new_2(callback_cast(vehicle_log_gpx), this_, log);
	} else if (!strcmp(type_attr.u.str, "textfile")) {
		char *header = "type=track\n";
		log_set_header(log, header, strlen(header));
		cb=callback_new_2(callback_cast(vehicle_log_textfile), this_, log);
	} else
		return 1;
	callback_list_add(this_->cbl, cb);
	return 0;
}

struct vehicle *
vehicle_new(struct attr *parent, struct attr **attrs)
{
	struct vehicle *this_;
	struct attr *source;
	struct vehicle_priv *(*vehicletype_new) (struct vehicle_methods *
						 meth,
						 struct callback_list *
						 cbl,
						 struct attr ** attrs);
	char *type, *colon;

	dbg(1, "enter\n");
	source = attr_search(attrs, NULL, attr_source);
	if (!source) {
		dbg(0, "no source\n");
		return NULL;
	}

	type = g_strdup(source->u.str);
	colon = strchr(type, ':');
	if (colon)
		*colon = '\0';
	dbg(1, "source='%s' type='%s'\n", source->u.str, type);

	vehicletype_new = plugin_get_vehicle_type(type);
	if (!vehicletype_new) {
		dbg(0, "invalid type '%s'\n", type);
		return NULL;
	}
	this_ = g_new0(struct vehicle, 1);
	this_->cbl = callback_list_new();
	this_->priv = vehicletype_new(&this_->meth, this_->cbl, attrs);
	if (!this_->priv) {
		dbg(0, "vehicletype_new failed\n");
		callback_list_destroy(this_->cbl);
		g_free(this_);
		return NULL;
	}
	this_->attrs=attr_list_dup(attrs);
	dbg(1, "leave\n");

	return this_;
}

int
vehicle_get_attr(struct vehicle *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter)
{
	int ret;
	if (this_->meth.position_attr_get) {
		ret=this_->meth.position_attr_get(this_->priv, type, attr);
		if (ret)
			return ret;
	}
	return attr_generic_get_attr(this_->attrs, NULL, type, attr, iter);
}

int
vehicle_set_attr(struct vehicle *this_, struct attr *attr,
		 struct attr **attrs)
{
	int ret=1;
	if (this_->meth.set_attr)
		ret=this_->meth.set_attr(this_->priv, attr, attrs);
	if (ret == 1 && attr->type != attr_navit)
		this_->attrs=attr_generic_set_attr(this_->attrs, attr);
	return ret != 0;
}

int
vehicle_add_attr(struct vehicle *this_, struct attr *attr)
{
	int ret=1;
	switch (attr->type) {
	case attr_callback:
		callback_list_add(this_->cbl, attr->u.callback);
		break;
	case attr_log:
		ret=vehicle_add_log(this_, attr->u.log);
		break;
	default:
		break;
	}
	if (ret)
		this_->attrs=attr_generic_add_attr(this_->attrs, attr);
	return ret;
}

int
vehicle_remove_attr(struct vehicle *this_, struct attr *attr)
{
	switch (attr->type) {
	case attr_callback:
		callback_list_remove(this_->cbl, attr->u.callback);
		break;
	default:
		return 0;
	}
	return 1;
}

void
vehicle_destroy(struct vehicle *this_)
{
	this_->meth.destroy(this_->priv);
	callback_list_destroy(this_->cbl);
	attr_list_free(this_->attrs);
	g_free(this_);
}

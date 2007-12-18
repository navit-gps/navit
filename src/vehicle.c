#include <glib.h>
#include <string.h>
#include "config.h"
#include "debug.h"
#include "coord.h"
#include "item.h"
#include "log.h"
#include "callback.h"
#include "plugin.h"
#include "vehicle.h"

struct vehicle {
	struct vehicle_priv *priv;
	struct vehicle_methods meth;
	struct callback_list *cbl;
	struct log *nmea_log, *gpx_log, *textfile_log;
};

static int
vehicle_add_log(struct vehicle *this_, struct log *log,
		struct attr **attrs)
{
	struct attr *type;
	type = attr_search(attrs, NULL, attr_type);
	if (!type)
		return 1;
	if (!strcmp(type->u.str, "nmea")) {
		this_->nmea_log = log;
	} else if (!strcmp(type->u.str, "gpx")) {
		char *header =
		    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<gpx version=\"1.0\" creator=\"Navit http://navit.sourceforge.net\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.topografix.com/GPX/1/0\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">\n<trk>\n<trkseg>\n";
		char *trailer = "</trkseg>\n</trk>\n</gpx>\n";
		this_->gpx_log = log;
		log_set_header(log, header, strlen(header));
		log_set_trailer(log, trailer, strlen(trailer));
	} else if (!strcmp(type->u.str, "textfile")) {
		char *header = "type=track\n";
		this_->textfile_log = log;
		log_set_header(log, header, strlen(header));
	} else
		return 1;
	return 0;
}

struct vehicle *
vehicle_new(struct attr **attrs)
{
	struct vehicle *this_;
	struct attr *source;
	struct vehicle_priv *(*vehicletype_new) (struct vehicle_methods *
						 meth,
						 struct callback_list *
						 cbl,
						 struct attr ** attrs);
	char *type, *colon;

	dbg(0, "enter\n");
	source = attr_search(attrs, NULL, attr_source);
	if (!source) {
		dbg(0, "no source\n");
		return NULL;
	}

	type = g_strdup(source->u.str);
	colon = index(type, ':');
	if (colon)
		*colon = '\0';
	dbg(0, "source='%s' type='%s'\n", source->u.str, type);

	vehicletype_new = plugin_get_vehicle_type(type);
	if (!vehicletype_new) {
		dbg(0, "invalid type\n");
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
	dbg(0, "leave\n");

	return this_;
}

int
vehicle_position_attr_get(struct vehicle *this_, enum attr_type type,
			  struct attr *attr)
{
	if (this_->meth.position_attr_get)
		return this_->meth.position_attr_get(this_->priv, type,
						     attr);
	return 0;
}

int
vehicle_set_attr(struct vehicle *this_, struct attr *attr,
		 struct attr **attrs)
{
	if (this_->meth.set_attr)
		return this_->meth.set_attr(this_->priv, attr, attrs);
	return 0;
}

int
vehicle_add_attr(struct vehicle *this_, struct attr *attr,
		 struct attr **attrs)
{
	switch (attr->type) {
	case attr_callback:
		callback_list_add(this_->cbl, attr->u.callback);
		break;
	case attr_log:
		return vehicle_add_log(this_, attr->u.log, attrs);
	default:
		return 0;
	}
	return 1;
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
	callback_list_destroy(this_->cbl);
	g_free(this_);
}

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

#include <string.h>
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include "config.h"
#include "main.h"
#include "navit.h"
#include "coord.h"
#include "point.h"
#include "plugin.h"
#include "debug.h"
#include "item.h"
#include "attr.h"
#include "layout.h"


static DBusConnection *connection;

static char *service_name="org.navit_project.navit";
static char *object_path="/org/navit_project/navit";

GHashTable *object_hash;
GHashTable *object_count;

static char *
object_new(char *type, void *object)
{
	int id;
	char *ret;
	dbg(0,"enter %s\n", type);
	id=(int)g_hash_table_lookup(object_count, type);
	g_hash_table_insert(object_count, type, (void *)(id+1));
	ret=g_strdup_printf("%s/%s/%d", object_path, type, id);
	g_hash_table_insert(object_hash, ret, object);
	dbg(0,"return %s\n", ret);
	return (ret);
}

static void *
object_get(const char *path)
{
	return g_hash_table_lookup(object_hash, path);
}

static void *
object_get_from_message_arg(DBusMessage *message, char *type)
{
	char *opath;
	char *prefix;
	DBusError error;
	void *ret=NULL;

	dbus_error_init(&error);
	if (!dbus_message_get_args(message, &error, DBUS_TYPE_OBJECT_PATH, &opath, DBUS_TYPE_INVALID)) {
		dbus_error_free(&error);
		dbg(0,"wrong arg type\n");
		return NULL;
	}
	prefix=g_strdup_printf("%s/%s/", object_path, type);
	if (!strncmp(prefix, opath, strlen(prefix)))
		ret=object_get(opath);
	else
		dbg(0,"wrong object type\n");
	g_free(prefix);
	return ret;
}

static void *
object_get_from_message(DBusMessage *message, char *type)
{
	const char *opath=dbus_message_get_path(message);
	char *prefix;
	void *ret=NULL;

	prefix=g_strdup_printf("%s/%s/", object_path, type);
	if (!strncmp(prefix, opath, strlen(prefix)))
		ret=object_get(opath);
	else
		dbg(0,"wrong object type\n");
	g_free(prefix);
	return ret;
}

static DBusHandlerResult
empty_reply(DBusConnection *connection, DBusMessage *message)
{
	DBusMessage *reply;

	reply = dbus_message_new_method_return(message);
	dbus_connection_send (connection, reply, NULL);
	dbus_message_unref (reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
request_main_get_navit(DBusConnection *connection, DBusMessage *message)
{
	DBusMessage *reply;
	DBusError error;
	struct iter *iter;
	struct navit *navit;
	char *opath;

	dbus_error_init(&error);

	if (!dbus_message_get_args(message, &error, DBUS_TYPE_OBJECT_PATH, &opath, DBUS_TYPE_INVALID)) {
		dbg(0,"Error parsing\n");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}
	dbg(0,"opath=%s\n", opath);
	iter=object_get(opath);
	navit=main_get_navit(iter);
	if (navit) {
		reply = dbus_message_new_method_return(message);
		opath=object_new("navit",navit);
		dbus_message_append_args(reply, DBUS_TYPE_OBJECT_PATH, &opath, DBUS_TYPE_INVALID);
		dbus_connection_send (connection, reply, NULL);
		dbus_message_unref (reply);
		return DBUS_HANDLER_RESULT_HANDLED;
	}
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusHandlerResult
request_main_iter(DBusConnection *connection, DBusMessage *message)
{
	DBusMessage *reply;
	struct iter *iter=main_iter_new();
	dbg(0,"iter=%p\n", iter);
	char *opath=object_new("main_iter",iter);
	reply = dbus_message_new_method_return(message);
	dbus_message_append_args(reply, DBUS_TYPE_OBJECT_PATH, &opath, DBUS_TYPE_INVALID);
	dbus_connection_send (connection, reply, NULL);
	dbus_message_unref (reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
request_main_iter_destroy(DBusConnection *connection, DBusMessage *message)
{
	struct iter *iter;
	
	iter=object_get_from_message_arg(message, "main_iter");
	if (! iter)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	main_iter_destroy(iter);

	return empty_reply(connection, message);
}

/**
 * Extracts a struct pcoord from a DBus message
 *
 * @param message The DBus message
 * @param iter Sort of pointer that points on that (iii)-object in the message
 * @param pc Pointer where the data should get stored
 */
static int
pcoord_get_from_message(DBusMessage *message, DBusMessageIter *iter, struct pcoord *pc)
{
	DBusMessageIter iter2;

	dbg(0,"%s\n", dbus_message_iter_get_signature(iter));
	
	dbus_message_iter_recurse(iter, &iter2);

	if (dbus_message_iter_get_arg_type(&iter2) != DBUS_TYPE_INT32)
		return 0;
	dbus_message_iter_get_basic(&iter2, &pc->pro);
	
	dbus_message_iter_next(&iter2);
	
	if (dbus_message_iter_get_arg_type(&iter2) != DBUS_TYPE_INT32)
		return 0;
	dbus_message_iter_get_basic(&iter2, &pc->x);
	
	dbus_message_iter_next(&iter2);

	if (dbus_message_iter_get_arg_type(&iter2) != DBUS_TYPE_INT32)
		return 0;
	dbus_message_iter_get_basic(&iter2, &pc->y);

	dbg(0, " pro -> %i x -> %i y -> %i\n", &pc->pro, &pc->x, &pc->y);
	
	dbus_message_iter_next(&iter2);
	
	if (dbus_message_iter_get_arg_type(&iter2) != DBUS_TYPE_INVALID)
		return 0;

	return 1;
}

/**
 * Extracts a struct point from a DBus message
 *
 * @param message The DBus message
 * @param iter Sort of pointer that points on that (ii)-object in the message
 * @param p Pointer where the data should get stored
 */
static int
point_get_from_message(DBusMessage *message, DBusMessageIter *iter, struct point *p)
{
	DBusMessageIter iter2;

	dbg(0,"%s\n", dbus_message_iter_get_signature(iter));
	
	dbus_message_iter_recurse(iter, &iter2);

	if (dbus_message_iter_get_arg_type(&iter2) != DBUS_TYPE_INT32)
		return 0;
	dbus_message_iter_get_basic(&iter2, &p->x);
	
	dbus_message_iter_next(&iter2);
	
	if (dbus_message_iter_get_arg_type(&iter2) != DBUS_TYPE_INT32)
		return 0;
	dbus_message_iter_get_basic(&iter2, &p->y);

	dbg(0, " x -> %i  y -> %i\n", p->x, p->y);
	
	dbus_message_iter_next(&iter2);

	if (dbus_message_iter_get_arg_type(&iter2) != DBUS_TYPE_INVALID)
		return 0;
	
	return 1;
}

static DBusHandlerResult
request_navit_set_center(DBusConnection *connection, DBusMessage *message)
{
	struct pcoord pc;
	struct navit *navit;
	DBusMessageIter iter;

	navit=object_get_from_message(message, "navit");
	if (! navit)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	dbus_message_iter_init(message, &iter);

	if (!pcoord_get_from_message(message, &iter, &pc))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	navit_set_center(navit, &pc);
	return empty_reply(connection, message);
}

static DBusHandlerResult
request_navit_set_center_screen(DBusConnection *connection, DBusMessage *message)
{
	struct point p;
	struct navit *navit;
	DBusMessageIter iter;

	navit=object_get_from_message(message, "navit");
	if (! navit)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	dbus_message_iter_init(message, &iter);

	if (!point_get_from_message(message, &iter, &p))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	navit_set_center_screen(navit, &p);
	return empty_reply(connection, message);
}

static DBusHandlerResult
request_navit_set_layout(DBusConnection *connection, DBusMessage *message)
{
	char *new_layout_name;
	struct navit *navit;
	struct attr attr;
	struct attr_iter *iter;

	navit=object_get_from_message(message, "navit");
	if (! navit)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	
    if (!dbus_message_get_args(message, NULL, DBUS_TYPE_STRING, &new_layout_name, DBUS_TYPE_INVALID))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	
    iter=navit_attr_iter_new();
	while(navit_get_attr(navit, attr_layout, &attr, iter)) {
		if (strcmp(attr.u.layout->name, new_layout_name) == 0) {
			navit_set_attr(navit, &attr);
		}
	}
	return empty_reply(connection, message);
}

static DBusHandlerResult
request_navit_zoom(DBusConnection *connection, DBusMessage *message)
{
	int factor;
	struct point *p = NULL;
	struct navit *navit;
	DBusMessageIter iter;

	navit = object_get_from_message(message, "navit");
	if (! navit)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	dbus_message_iter_init(message, &iter);
	dbg(0,"%s\n", dbus_message_iter_get_signature(&iter));
	
	dbus_message_iter_get_basic(&iter, &factor);
	
	if (dbus_message_iter_has_next(&iter))
	{
		dbus_message_iter_next(&iter);
		if (!point_get_from_message(message, &iter, p))
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	if (factor > 1)
		navit_zoom_in(navit, factor, p);
	else if (factor < -1)
		navit_zoom_out(navit, 0-factor, p);

	return empty_reply(connection, message);

}

static DBusHandlerResult
request_navit_resize(DBusConnection *connection, DBusMessage *message)
{
	struct navit *navit;
	int w, h;
	DBusMessageIter iter;

	navit = object_get_from_message(message, "navit");
	if (! navit)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	dbus_message_iter_init(message, &iter);
	dbg(0,"%s\n", dbus_message_iter_get_signature(&iter));
	
	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_INT32)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	dbus_message_iter_get_basic(&iter, &w);
	
	dbus_message_iter_next(&iter);
	
	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_INT32)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	dbus_message_iter_get_basic(&iter, &h);

	dbg(0, " w -> %i  h -> %i\n", w, h);
	
	navit_resize(navit, w, h);

	return empty_reply(connection, message);

}

static DBusHandlerResult
request_navit_set_position(DBusConnection *connection, DBusMessage *message)
{
	struct navit *navit;
	struct pcoord c;

	DBusMessageIter iter;

	navit = object_get_from_message(message, "navit");
	if (! navit)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	dbus_message_iter_init(message, &iter);
	pcoord_get_from_message(message, &iter, &c);
	
	navit_set_position(navit, &c);
	return empty_reply(connection, message);
}

static DBusHandlerResult
request_navit_set_destination(DBusConnection *connection, DBusMessage *message)
{
	struct navit *navit;
	struct pcoord c;
	char *description;

	DBusMessageIter iter;

	navit = object_get_from_message(message, "navit");
	if (! navit)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	dbus_message_iter_init(message, &iter);
	pcoord_get_from_message(message, &iter, &c);
	
	dbus_message_iter_next(&iter);
	dbus_message_iter_get_basic(&iter, &description);
	dbg(0, " destination -> %s\n", description);
	
	navit_set_destination(navit, &c, description);
	return empty_reply(connection, message);
}

struct dbus_method {
	char *path;
	char *method;
	char *signature;
	DBusHandlerResult(*func)(DBusConnection *connection, DBusMessage *message);
} dbus_methods[] = {
	{"",		"iter",                "",        request_main_iter},
	{"",        "iter_destroy",        "o",       request_main_iter_destroy},
	{"",        "get_navit",           "o",       request_main_get_navit},
	{".navit",	"set_center",          "(iii)",   request_navit_set_center},
	{".navit",  "set_center_screen",   "(ii)",    request_navit_set_center_screen},
	{".navit",  "set_layout",          "s",       request_navit_set_layout},
	{".navit",  "zoom",                "i(ii)",   request_navit_zoom},
	{".navit",  "zoom",                "i",       request_navit_zoom},
	{".navit",  "resize",              "ii",      request_navit_resize},
	{".navit",  "set_position",        "(iii)",   request_navit_set_position},
	{".navit",  "set_destination",     "(iii)s",  request_navit_set_destination},
};

static DBusHandlerResult
navit_handler_func(DBusConnection *connection, DBusMessage *message, void *user_data)
{
	int i;
	char *path;
	dbg(0,"type=%s interface=%s path=%s member=%s signature=%s\n", dbus_message_type_to_string(dbus_message_get_type(message)), dbus_message_get_interface(message), dbus_message_get_path(message), dbus_message_get_member(message), dbus_message_get_signature(message));
#if 0
	if (dbus_message_is_method_call (message, "org.freedesktop.DBus.Introspectable", "Introspect")) {
		DBusMessage *reply;
		gchar *idata;
		dbg(0,"Introspect\n");
		if (! strcmp(dbus_message_get_path(message), object_path)) {
			g_file_get_contents("binding/dbus/navit.introspect", &idata, NULL, NULL);
			reply = dbus_message_new_method_return(message);
			dbus_message_append_args(reply, DBUS_TYPE_STRING, &idata, DBUS_TYPE_INVALID);
			dbus_connection_send (connection, reply, NULL);
			dbus_message_unref (reply);
			g_free(idata);
			return DBUS_HANDLER_RESULT_HANDLED;
		}
	}
#endif
	for (i = 0 ; i < sizeof(dbus_methods)/sizeof(struct dbus_method) ; i++) {
		path=g_strdup_printf("%s%s", service_name, dbus_methods[i].path);
		if (dbus_message_is_method_call(message, path, dbus_methods[i].method) &&
		    dbus_message_has_signature(message, dbus_methods[i].signature)) {
			g_free(path);
			return dbus_methods[i].func(connection, message);
		}
		g_free(path);
	}
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusObjectPathVTable dbus_navit_vtable = {
	NULL,
	navit_handler_func,
	NULL
};

#if 0
DBusHandlerResult
filter(DBusConnection *connection, DBusMessage *message, void *user_data)
{
	dbg(0,"type=%s interface=%s path=%s member=%s signature=%s\n", dbus_message_type_to_string(dbus_message_get_type(message)), dbus_message_get_interface(message), dbus_message_get_path(message), dbus_message_get_member(message), dbus_message_get_signature(message));
	if (dbus_message_is_signal(message, DBUS_INTERFACE_DBUS, "NameOwnerChanged")) {
	}
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
#endif

void plugin_init(void)
{
	DBusError error;

	object_hash=g_hash_table_new(g_str_hash, g_str_equal);
	object_count=g_hash_table_new(g_str_hash, g_str_equal);
	dbg(0,"enter 1\n");
	dbus_error_init(&error);
	connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
	if (!connection) {
		dbg(0,"Failed to open connection to session message bus: %s\n", error.message);
		dbus_error_free(&error);
		return;
	}
	dbus_connection_setup_with_g_main(connection, NULL);
#if 0
	dbus_connection_add_filter(connection, filter, NULL, NULL);
	dbus_bus_add_match(connection, "type='signal',""interface='" DBUS_INTERFACE_DBUS  "'", &error);
#endif
	dbus_connection_register_fallback(connection, object_path, &dbus_navit_vtable, NULL);
	dbus_bus_request_name(connection, service_name, 0, &error);
	if (dbus_error_is_set(&error)) {
		dbg(0,"Failed to request name: %s", error.message);
		dbus_error_free (&error);
	}
}

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
#include "config_.h"
#include "navit.h"
#include "coord.h"
#include "point.h"
#include "plugin.h"
#include "debug.h"
#include "item.h"
#include "attr.h"
#include "layout.h"
#include "command.h"
#include "callback.h"
#include "graphics.h"
#include "vehicle.h"
#include "map.h"
#include "mapset.h"
#include "route.h"
#include "search.h"
#include "callback.h"
#include "gui.h"
#include "util.h"


static DBusConnection *connection;
static dbus_uint32_t dbus_serial;

static char *service_name = "org.navit_project.navit";
static char *object_path = "/org/navit_project/navit";
char *navitintrospectxml_head1 = "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
                                 "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
                                 "<node name=\"";

char *navitintrospectxml_head2 = "\">\n"
                                 "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
                                 "    <method name=\"Introspect\">\n"
                                 "      <arg direction=\"out\" type=\"s\" />\n"
                                 "    </method>\n"
                                 "  </interface>\n";



GHashTable *object_hash;
GHashTable *object_hash_rev;
GHashTable *object_count;

struct dbus_callback {
	struct callback *callback;
	char *signal;
};

static char *
object_new(char *type, void *object)
{
	int id;
	char *ret;
	dbg(1,"enter %s\n", type);
	if ((ret=g_hash_table_lookup(object_hash_rev, object)))
		return ret;
	id=GPOINTER_TO_INT(g_hash_table_lookup(object_count, type));
	g_hash_table_insert(object_count, type, GINT_TO_POINTER((id+1)));
	ret=g_strdup_printf("%s/%s/%d", object_path, type, id);
	g_hash_table_insert(object_hash, ret, object);
	g_hash_table_insert(object_hash_rev, object, ret);
	dbg(1,"return %s\n", ret);
	return (ret);
}

static void *
object_get(const char *path)
{
	return g_hash_table_lookup(object_hash, path);
}

static void 
object_destroy(const char *path, void *object)
{
	if (!path && !object)
		return;
	if (!object)
		object=g_hash_table_lookup(object_hash, path);
	if (!path)
		path=g_hash_table_lookup(object_hash_rev, object);
	g_hash_table_remove(object_hash, path);
	g_hash_table_remove(object_hash_rev, object);
}

static void *
resolve_object(const char *opath, char *type)
{
	char *prefix;
	const char *oprefix;
	void *ret=NULL;
	char *def_navit="/default_navit";
	char *def_gui="/default_gui";
	char *def_graphics="/default_graphics";
	char *def_vehicle="/default_vehicle";
	char *def_mapset="/default_mapset";
	char *def_map="/default_map";
	char *def_route="/default_route";
	struct attr attr;

	if (strncmp(opath, object_path, strlen(object_path))) {
		dbg(0,"wrong object path %s\n",opath);
		return NULL;
	}
	prefix=g_strdup_printf("%s/%s/", object_path, type);
	if (!strncmp(prefix, opath, strlen(prefix))) {
		ret=object_get(opath);
		g_free(prefix);
		return ret;
	}
	g_free(prefix);
	oprefix=opath+strlen(object_path);
	if (!strncmp(oprefix,def_navit,strlen(def_navit))) {
		oprefix+=strlen(def_navit);
		struct attr navit;
		if (!config_get_attr(config, attr_navit, &navit, NULL))
			return NULL;
		if (!oprefix[0]) {
			dbg(0,"default_navit\n");
			return navit.u.navit;
		}
		if (!strncmp(oprefix,def_graphics,strlen(def_graphics))) {
			if (navit_get_attr(navit.u.navit, attr_graphics, &attr, NULL)) {
				return attr.u.graphics;
			}
			return NULL;
		}
		if (!strncmp(oprefix,def_gui,strlen(def_gui))) {
			if (navit_get_attr(navit.u.navit, attr_gui, &attr, NULL)) {
				return attr.u.gui;
			}
			return NULL;
		}
		if (!strncmp(oprefix,def_vehicle,strlen(def_vehicle))) {
			if (navit_get_attr(navit.u.navit, attr_vehicle, &attr, NULL)) {
				return attr.u.vehicle;
			}
			return NULL;
		}
		if (!strncmp(oprefix,def_mapset,strlen(def_mapset))) {
			oprefix+=strlen(def_mapset);
			if (navit_get_attr(navit.u.navit, attr_mapset, &attr, NULL)) {
				if (!oprefix[0]) {
					return attr.u.mapset;
				}	
				if (!strncmp(oprefix,def_map,strlen(def_map))) {
					if (mapset_get_attr(attr.u.mapset, attr_map, &attr, NULL)) {
						return attr.u.map;
					}
					return NULL;
				}
			}
			return NULL;
		}
		if (!strncmp(oprefix,def_route,strlen(def_route))) {
			oprefix+=strlen(def_route);
			if (navit_get_attr(navit.u.navit, attr_route, &attr, NULL)) {
				return attr.u.route;
			}
			return NULL;
		}
	}
	return NULL;
}

static void *
object_get_from_message_arg(DBusMessageIter *iter, char *type)
{
	char *opath;

	if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_OBJECT_PATH)
		return NULL;
	dbus_message_iter_get_basic(iter, &opath);
	dbus_message_iter_next(iter);
	return resolve_object(opath, type);
}

static void *
object_get_from_message(DBusMessage *message, char *type)
{
	return resolve_object(dbus_message_get_path(message), type);
}

static enum attr_type 
attr_type_get_from_message(DBusMessageIter *iter)
{
	char *attr_type;

	if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_STRING) 
		return attr_none;
	dbus_message_iter_get_basic(iter, &attr_type);
	dbus_message_iter_next(iter);
	return attr_from_name(attr_type); 
}

static void
encode_variant_string(DBusMessageIter *iter, char *str)
{
	DBusMessageIter variant;
	dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &variant);
	dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &str);
	dbus_message_iter_close_container(iter, &variant);
}

static void
encode_dict_string_variant_string(DBusMessageIter *iter, char *key, char *value)
{
	DBusMessageIter dict;
	dbus_message_iter_open_container(iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict);
	dbus_message_iter_append_basic(&dict, DBUS_TYPE_STRING, &key);
	encode_variant_string(&dict, value);
	dbus_message_iter_close_container(iter, &dict);
}

static int
encode_attr(DBusMessageIter *iter1, struct attr *attr)
{
	char *name=attr_to_name(attr->type);
	DBusMessageIter iter2;
	dbus_message_iter_append_basic(iter1, DBUS_TYPE_STRING, &name);
	if (attr->type >= attr_type_int_begin && attr->type < attr_type_boolean_begin) {
		dbus_message_iter_open_container(iter1, DBUS_TYPE_VARIANT, DBUS_TYPE_INT32_AS_STRING, &iter2);
		dbus_message_iter_append_basic(&iter2, DBUS_TYPE_INT32, &attr->u.num);
		dbus_message_iter_close_container(iter1, &iter2);
	}
	if (attr->type >= attr_type_boolean_begin && attr->type <= attr_type_int_end) {
		dbus_message_iter_open_container(iter1, DBUS_TYPE_VARIANT, DBUS_TYPE_BOOLEAN_AS_STRING, &iter2);
		dbus_message_iter_append_basic(&iter2, DBUS_TYPE_BOOLEAN, &attr->u.num);
		dbus_message_iter_close_container(iter1, &iter2);
	}
	if (attr->type >= attr_type_string_begin && attr->type <= attr_type_string_end) {
		encode_variant_string(iter1, attr->u.str);
	}
	if (attr->type >= attr_type_object_begin && attr->type <= attr_type_object_end) {
		char *object=object_new(attr_to_name(attr->type), attr->u.data);
		dbus_message_iter_open_container(iter1, DBUS_TYPE_VARIANT, DBUS_TYPE_OBJECT_PATH_AS_STRING, &iter2);
		dbus_message_iter_append_basic(&iter2, DBUS_TYPE_OBJECT_PATH, &object);
		dbus_message_iter_close_container(iter1, &iter2);
	}
	return 1;
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
dbus_error(DBusConnection *connection, DBusMessage *message, char *error, char *msg)
{
	DBusMessage *reply;

	reply = dbus_message_new_error(message, error, msg);
	dbus_connection_send (connection, reply, NULL);
	dbus_message_unref (reply);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
dbus_error_invalid_attr_type(DBusConnection *connection, DBusMessage *message)
{
	return dbus_error(connection, message, DBUS_ERROR_INVALID_ARGS, "attribute type invalid");
}

static DBusHandlerResult
dbus_error_invalid_parameter(DBusConnection *connection, DBusMessage *message)
{
	return dbus_error(connection, message, DBUS_ERROR_INVALID_ARGS, "parameter invalid");
}

static DBusHandlerResult
dbus_error_invalid_object_path(DBusConnection *connection, DBusMessage *message)
{
	return dbus_error(connection, message, DBUS_ERROR_BAD_ADDRESS, "object path invalid");
}

static DBusHandlerResult
dbus_error_invalid_object_path_parameter(DBusConnection *connection, DBusMessage *message)
{
	return dbus_error(connection, message, DBUS_ERROR_BAD_ADDRESS, "object path parameter invalid");
}

#if 0
static void
dbus_dump_iter(char *prefix, DBusMessageIter *iter)
{
	char *prefixr,*vals;
	int arg,vali;
	DBusMessageIter iterr;
	while ((arg=dbus_message_iter_get_arg_type(iter)) != DBUS_TYPE_INVALID) {
		switch (arg) {
		case DBUS_TYPE_INT32:
            		dbus_message_iter_get_basic(iter, &vali);
			dbg(0,"%sDBUS_TYPE_INT32: %d\n",prefix,vali);
			break;
		case DBUS_TYPE_STRING:
            		dbus_message_iter_get_basic(iter, &vals);
			dbg(0,"%sDBUS_TYPE_STRING: %s\n",prefix,vals);
			break;
		case DBUS_TYPE_STRUCT:
			dbg(0,"%sDBUS_TYPE_STRUCT:\n",prefix);
			prefixr=g_strdup_printf("%s  ",prefix);
        		dbus_message_iter_recurse(iter, &iterr);
			dbus_dump_iter(prefixr, &iterr);
			g_free(prefixr);
			break;
		case DBUS_TYPE_VARIANT:
			dbg(0,"%sDBUS_TYPE_VARIANT:\n",prefix);
			prefixr=g_strdup_printf("%s  ",prefix);
        		dbus_message_iter_recurse(iter, &iterr);
			dbus_dump_iter(prefixr, &iterr);
			g_free(prefixr);
			break;
		case DBUS_TYPE_DICT_ENTRY:
			dbg(0,"%sDBUS_TYPE_DICT_ENTRY:\n",prefix);
			prefixr=g_strdup_printf("%s  ",prefix);
        		dbus_message_iter_recurse(iter, &iterr);
			dbus_dump_iter(prefixr, &iterr);
			g_free(prefixr);
			break;
		case DBUS_TYPE_ARRAY:
			dbg(0,"%sDBUS_TYPE_ARRAY:\n",prefix);
			prefixr=g_strdup_printf("%s  ",prefix);
        		dbus_message_iter_recurse(iter, &iterr);
			dbus_dump_iter(prefixr, &iterr);
			g_free(prefixr);
			break;
		default:
			dbg(0,"%c\n",arg);
		}
		dbus_message_iter_next(iter);
	}
}

static void
dbus_dump(DBusMessage *message)
{
	DBusMessageIter iter;

	dbus_message_iter_init(message, &iter);
	dbus_dump_iter("",&iter);
		
}
#endif

/**
 * Extracts a struct pcoord from a DBus message
 *
 * @param message The DBus message
 * @param iter Sort of pointer that points on that (iii)-object in the message
 * @param pc Pointer where the data should get stored
 * @returns Returns 1 when everything went right, otherwise 0
 */
static int
pcoord_get_from_message(DBusMessage *message, DBusMessageIter *iter, struct pcoord *pc)
{

    if(!strcmp(dbus_message_iter_get_signature(iter), "s")) {
        char *coordstring;

        dbus_message_iter_get_basic(iter, &coordstring);
        if(!pcoord_parse(coordstring, projection_mg, pc))
            return 0;
        
        return 1;
    } else {
        
        DBusMessageIter iter2;
        dbus_message_iter_recurse(iter, &iter2);
        if(!strcmp(dbus_message_iter_get_signature(iter), "(is)")) {
            char *coordstring;
            int projection;
            
            dbus_message_iter_get_basic(&iter2, &projection);

            dbus_message_iter_next(&iter2);
            dbus_message_iter_get_basic(&iter2, &coordstring);

            if(!pcoord_parse(coordstring, projection, pc))
                return 0;

            return 1;
        } else if(!strcmp(dbus_message_iter_get_signature(iter), "(iii)")) {
            
            dbus_message_iter_get_basic(&iter2, &pc->pro);
            
            dbus_message_iter_next(&iter2);
            dbus_message_iter_get_basic(&iter2, &pc->x);
            
            dbus_message_iter_next(&iter2);
            dbus_message_iter_get_basic(&iter2, &pc->y);

            return 1;
        }
    }
    return 0;
    
}

static void
pcoord_encode(DBusMessageIter *iter, struct pcoord *pc)
{
	DBusMessageIter iter2;
	dbus_message_iter_open_container(iter,DBUS_TYPE_STRUCT,NULL,&iter2);
	if (pc) {
		dbus_message_iter_append_basic(&iter2, DBUS_TYPE_INT32, &pc->pro);
		dbus_message_iter_append_basic(&iter2, DBUS_TYPE_INT32, &pc->x);
		dbus_message_iter_append_basic(&iter2, DBUS_TYPE_INT32, &pc->y);
	} else {
		int n=0;
		dbus_message_iter_append_basic(&iter2, DBUS_TYPE_INT32, &n);
		dbus_message_iter_append_basic(&iter2, DBUS_TYPE_INT32, &n);
		dbus_message_iter_append_basic(&iter2, DBUS_TYPE_INT32, &n);
	}
	dbus_message_iter_close_container(iter, &iter2);
}

static enum attr_type
decode_attr_type_from_iter(DBusMessageIter *iter)
{
	char *attr_type;
	enum attr_type ret;
	
	if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_STRING)
		return attr_none;
	dbus_message_iter_get_basic(iter, &attr_type);
	dbus_message_iter_next(iter);
	ret=attr_from_name(attr_type); 
	dbg(1, "attr value: 0x%x string: %s\n", ret, attr_type);
	return ret;
}

static int
decode_attr_from_iter(DBusMessageIter *iter, struct attr *attr)
{
	DBusMessageIter iterattr, iterstruct;
	int ret=1;
	double d;

	attr->type=decode_attr_type_from_iter(iter);
	if (attr->type == attr_none)
		return 0;
    
	dbus_message_iter_recurse(iter, &iterattr);
	dbus_message_iter_next(iter);
	dbg(1, "seems valid. signature: %s\n", dbus_message_iter_get_signature(&iterattr));
    
	if (attr->type >= attr_type_item_begin && attr->type <= attr_type_item_end)
		return 0;

	if (attr->type >= attr_type_int_begin && attr->type <= attr_type_boolean_begin) {
		if (dbus_message_iter_get_arg_type(&iterattr) == DBUS_TYPE_INT32) {
			dbus_message_iter_get_basic(&iterattr, &attr->u.num);
			return 1;
		}
		return 0;
	}
	if(attr->type >= attr_type_boolean_begin && attr->type <= attr_type_int_end) {
		if (dbus_message_iter_get_arg_type(&iterattr) == DBUS_TYPE_BOOLEAN) {
			dbus_message_iter_get_basic(&iterattr, &attr->u.num);
			return 1;
		}
		return 0;
        }
	if(attr->type >= attr_type_string_begin && attr->type <= attr_type_string_end) {
		if (dbus_message_iter_get_arg_type(&iterattr) == DBUS_TYPE_STRING) {
			dbus_message_iter_get_basic(&iterattr, &attr->u.str);
			return 1;
		}
		return 0;
        }
	if(attr->type >= attr_type_double_begin && attr->type <= attr_type_double_end) {
		if (dbus_message_iter_get_arg_type(&iterattr) == DBUS_TYPE_DOUBLE) {
			attr->u.numd=g_new(typeof(*attr->u.numd),1);
			dbus_message_iter_get_basic(&iterattr, attr->u.numd);
			return 1;
		}
	}
	if(attr->type >= attr_type_coord_geo_begin && attr->type <= attr_type_coord_geo_end) {
		if (dbus_message_iter_get_arg_type(&iterattr) == DBUS_TYPE_STRUCT) {
			attr->u.coord_geo=g_new(typeof(*attr->u.coord_geo),1);
			dbus_message_iter_recurse(&iterattr, &iterstruct);
			if (dbus_message_iter_get_arg_type(&iterstruct) == DBUS_TYPE_DOUBLE) {
				dbus_message_iter_get_basic(&iterstruct, &d);
				dbus_message_iter_next(&iterstruct);
				attr->u.coord_geo->lng=d;
			} else
				ret=0;
			if (dbus_message_iter_get_arg_type(&iterstruct) == DBUS_TYPE_DOUBLE) {
				dbus_message_iter_get_basic(&iterstruct, &d);
				attr->u.coord_geo->lat=d;
			} else
				ret=0;
			if (!ret) {
				g_free(attr->u.coord_geo);
				attr->u.coord_geo=NULL;
			}
			return ret;
		}
	}
	if (attr->type == attr_callback) {
		struct dbus_callback *callback=object_get_from_message_arg(&iterattr, "callback");
		if (callback) {
			attr->u.callback=callback->callback;
			return 1;
		}
	}
	return 0;
}

static int
decode_attr(DBusMessage *message, struct attr *attr)
{
	DBusMessageIter iter;

	dbus_message_iter_init(message, &iter);
	return decode_attr_from_iter(&iter, attr);
}

static void
destroy_attr(struct attr *attr)
{
	if(attr->type > attr_type_double_begin && attr->type < attr_type_double_end) {
		g_free(attr->u.numd);
	}
}

static char *
get_iter_name(char *type)
{
	return g_strdup_printf("%s_attr_iter",type);
}

static DBusHandlerResult
request_attr_iter(DBusConnection *connection, DBusMessage *message, char *type, struct attr_iter *(*func)(void))
{
	DBusMessage *reply;
	char *iter_name;
	char *opath;
	struct attr_iter *attr_iter;

	attr_iter=(*func)();
	iter_name=get_iter_name(type);
	opath=object_new(iter_name,attr_iter);
	g_free(iter_name);
	reply = dbus_message_new_method_return(message);
	dbus_message_append_args(reply, DBUS_TYPE_OBJECT_PATH, &opath, DBUS_TYPE_INVALID);
	dbus_connection_send (connection, reply, NULL);
	dbus_message_unref (reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
request_attr_iter_destroy(DBusConnection *connection, DBusMessage *message, char *type, void (*func)(struct attr_iter *))
{
	struct attr_iter *attr_iter;
	DBusMessageIter iter;
	char *iter_name;

	dbus_message_iter_init(message, &iter);
	iter_name=get_iter_name(type);
	attr_iter=object_get_from_message_arg(&iter, iter_name);
	g_free(iter_name);
	if (! attr_iter)
		return dbus_error_invalid_object_path_parameter(connection, message);
	object_destroy(NULL, attr_iter);
	func(attr_iter);

	return empty_reply(connection, message);
}

static DBusHandlerResult
request_destroy(DBusConnection *connection, DBusMessage *message, char *type, void *data, void (*func)(void *))
{
	if (!data) 
		data=object_get_from_message(message, type);
	if (!data)
		return dbus_error_invalid_object_path(connection, message);
	object_destroy(NULL, data);
	func(data);

	return empty_reply(connection, message);
}


static DBusHandlerResult
request_get_attr(DBusConnection *connection, DBusMessage *message, char *type, void *data, int (*func)(void *data, enum attr_type type, struct attr *attr, struct attr_iter *iter))
{
	DBusMessage *reply;
	DBusMessageIter iter;
	struct attr attr;
	enum attr_type attr_type;
	struct attr_iter *attr_iter;
	char *iter_name;

	if (! data)
		data = object_get_from_message(message, type);
	if (! data)
		return dbus_error_invalid_object_path(connection, message);

	dbus_message_iter_init(message, &iter);
	attr_type=attr_type_get_from_message(&iter);
	if (attr_type == attr_none)
		return dbus_error_invalid_attr_type(connection, message);
	iter_name=get_iter_name(type);	
	attr_iter=object_get_from_message_arg(&iter, iter_name);
	g_free(iter_name);
	if (func(data, attr_type, &attr, attr_iter)) {
		DBusMessageIter iter1;
		reply = dbus_message_new_method_return(message);
		dbus_message_iter_init_append(reply, &iter1);
		encode_attr(&iter1, &attr);
		dbus_connection_send (connection, reply, NULL);
		dbus_message_unref (reply);
		return DBUS_HANDLER_RESULT_HANDLED;
	}
	return empty_reply(connection, message);
	
}

static DBusHandlerResult
request_command(DBusConnection *connection, DBusMessage *message, char *type, void *data, int (*func)(void *data, enum attr_type type, struct attr *attr, struct attr_iter *iter))
{
	DBusMessageIter iter;
	struct attr attr;
	char *command;

	if (! data)
		data = object_get_from_message(message, type);
	if (! data)
		return dbus_error_invalid_object_path(connection, message);

	dbus_message_iter_init(message, &iter);
	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) 
		return dbus_error_invalid_parameter(connection, message);
	dbus_message_iter_get_basic(&iter, &command);
	dbus_message_iter_next(&iter);
	if (func(data, attr_callback_list, &attr, NULL)) {
		int valid=0;
                callback_list_call_attr_4(attr.u.callback_list, attr_command, command, NULL, NULL, &valid);
	}
	return empty_reply(connection, message);
	
}

static DBusHandlerResult
request_set_add_remove_attr(DBusConnection *connection, DBusMessage *message, char *type, void *data, int (*func)(void *data, struct attr *attr))
{
    	struct attr attr;
	int ret;

	if (! data)	
		data = object_get_from_message(message, type);
	if (! data)
		return dbus_error_invalid_object_path(connection, message);

	if (decode_attr(message, &attr)) {
		ret=(*func)(data, &attr);
		destroy_attr(&attr);
		if (ret)	
			return empty_reply(connection, message);
	}
    	return dbus_error_invalid_parameter(connection, message);
}

/* callback */


static void
dbus_callback_emit_signal(struct dbus_callback *dbus_callback)
{
	DBusMessage* msg;
	msg = dbus_message_new_signal(object_path, service_name, dbus_callback->signal);
	if (msg) {
		dbus_connection_send(connection, msg, &dbus_serial);
		dbus_connection_flush(connection);
		dbus_message_unref(msg);
	}
}

static void
request_callback_destroy_do(struct dbus_callback *data)
{
	callback_destroy(data->callback);
	g_free(data->signal);
	g_free(data);
}

static DBusHandlerResult
request_callback_destroy(DBusConnection *connection, DBusMessage *message)
{
	return request_destroy(connection, message, "search_list", NULL, (void (*)(void *)) request_callback_destroy_do);
}

static DBusHandlerResult
request_callback_new(DBusConnection *connection, DBusMessage *message)
{
	DBusMessageIter iter;
	DBusMessage *reply;
	struct dbus_callback *callback;
	char *signal,*opath;
	enum attr_type type;

	dbus_message_iter_init(message, &iter);
	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) 
		return dbus_error_invalid_parameter(connection, message);
	dbus_message_iter_get_basic(&iter, &signal);
	dbus_message_iter_next(&iter);
	callback=g_new0(struct dbus_callback, 1);
	callback->signal=g_strdup(signal);

	if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING) {
		type=attr_type_get_from_message(&iter);
		callback->callback=callback_new_attr_1(callback_cast(dbus_callback_emit_signal), type, callback);
	} else
		callback->callback=callback_new_1(callback_cast(dbus_callback_emit_signal), callback);
	opath=object_new("callback", callback);
	reply = dbus_message_new_method_return(message);
	dbus_message_append_args(reply, DBUS_TYPE_OBJECT_PATH, &opath, DBUS_TYPE_INVALID);
	dbus_connection_send (connection, reply, NULL);
	dbus_message_unref (reply);
	return DBUS_HANDLER_RESULT_HANDLED;
}

/* config */
static DBusHandlerResult
request_config_get_attr(DBusConnection *connection, DBusMessage *message)
{
	return request_get_attr(connection, message, "config", config, (int (*)(void *, enum attr_type, struct attr *, struct attr_iter *))config_get_attr);
}

static DBusHandlerResult
request_config_attr_iter(DBusConnection *connection, DBusMessage *message)
{
	return request_attr_iter(connection, message, "config", (struct attr_iter * (*)(void))config_attr_iter_new);
}

static DBusHandlerResult
request_config_attr_iter_destroy(DBusConnection *connection, DBusMessage *message)
{
	return request_attr_iter_destroy(connection, message, "config", (void (*)(struct attr_iter *))config_attr_iter_destroy);
}



/* graphics */

static DBusHandlerResult
request_graphics_get_data(DBusConnection *connection, DBusMessage *message)
{
	struct graphics *graphics;
	char *data;
	struct graphics_data_image *image;
	DBusMessage *reply;

	graphics = object_get_from_message(message, "graphics");
	if (! graphics)
		return dbus_error_invalid_object_path(connection, message);

        if (!dbus_message_get_args(message, NULL, DBUS_TYPE_STRING, &data, DBUS_TYPE_INVALID))
    		return dbus_error_invalid_parameter(connection, message);
	image=graphics_get_data(graphics, data);
	if (image) {
		DBusMessageIter iter1,iter2;
		reply = dbus_message_new_method_return(message);
#if 0
		dbus_message_append_args(reply, DBUS_TYPE_STRING, &result, DBUS_TYPE_INVALID);
#endif
		dbus_message_iter_init_append(reply, &iter1);
		dbus_message_iter_open_container(&iter1, DBUS_TYPE_ARRAY, "y", &iter2);
		if (image->data && image->size) 
			dbus_message_iter_append_fixed_array(&iter2, DBUS_TYPE_BYTE, &image->data, image->size);
		dbus_message_iter_close_container(&iter1, &iter2);
		dbus_connection_send (connection, reply, NULL);
		dbus_message_unref (reply);
		return DBUS_HANDLER_RESULT_HANDLED;
	}
	return empty_reply(connection, message);
}

/* gui */

static DBusHandlerResult
request_gui_get_attr(DBusConnection *connection, DBusMessage *message)
{
	return request_get_attr(connection, message, "gui", NULL, (int (*)(void *, enum attr_type, struct attr *, struct attr_iter *))gui_get_attr);
}

static DBusHandlerResult
request_gui_command(DBusConnection *connection, DBusMessage *message)
{
	return request_command(connection, message, "gui", NULL, (int (*)(void *, enum attr_type, struct attr *, struct attr_iter *))gui_get_attr);
}



static DBusHandlerResult
request_graphics_set_attr(DBusConnection *connection, DBusMessage *message)
{
	return request_set_add_remove_attr(connection, message, "graphics", NULL, (int (*)(void *, struct attr *))graphics_set_attr);
}

/* map */

static DBusHandlerResult
request_map_get_attr(DBusConnection *connection, DBusMessage *message)
{
	return request_get_attr(connection, message, "map", NULL, (int (*)(void *, enum attr_type, struct attr *, struct attr_iter *))map_get_attr);
}


static DBusHandlerResult
request_map_set_attr(DBusConnection *connection, DBusMessage *message)
{
	return request_set_add_remove_attr(connection, message, "map", NULL, (int (*)(void *, struct attr *))map_set_attr);
}

/* mapset */

static DBusHandlerResult
request_mapset_attr_iter(DBusConnection *connection, DBusMessage *message)
{
	return request_attr_iter(connection, message, "mapset", (struct attr_iter * (*)(void))mapset_attr_iter_new);
}

static DBusHandlerResult
request_mapset_attr_iter_destroy(DBusConnection *connection, DBusMessage *message)
{
	return request_attr_iter_destroy(connection, message, "mapset", (void (*)(struct attr_iter *))mapset_attr_iter_destroy);
}

static DBusHandlerResult
request_mapset_get_attr(DBusConnection *connection, DBusMessage *message)
{
	return request_get_attr(connection, message, "mapset", NULL, (int (*)(void *, enum attr_type, struct attr *, struct attr_iter *))mapset_get_attr);
}

/* route */

static DBusHandlerResult
request_route_get_attr(DBusConnection *connection, DBusMessage *message)
{
	return request_get_attr(connection, message, "route", NULL, (int (*)(void *, enum attr_type, struct attr *, struct attr_iter *))route_get_attr);
}


static DBusHandlerResult
request_route_set_attr(DBusConnection *connection, DBusMessage *message)
{
	return request_set_add_remove_attr(connection, message, "route", NULL, (int (*)(void *, struct attr *))route_set_attr);
}

static DBusHandlerResult
request_route_add_attr(DBusConnection *connection, DBusMessage *message)
{
	return request_set_add_remove_attr(connection, message, "route", NULL, (int (*)(void *, struct attr *))route_add_attr);
}

static DBusHandlerResult
request_route_remove_attr(DBusConnection *connection, DBusMessage *message)
{
	return request_set_add_remove_attr(connection, message, "route", NULL, (int (*)(void *, struct attr *))route_remove_attr);
}


/* navit */

static DBusHandlerResult
request_navit_draw(DBusConnection *connection, DBusMessage *message)
{
	struct navit *navit;

	navit=object_get_from_message(message, "navit");
	if (! navit)
		return dbus_error_invalid_object_path(connection, message);

	navit_draw(navit);
	
	return empty_reply(connection, message);
}


/**
 * Extracts a struct point from a DBus message
 *
 * @param message The DBus message
 * @param iter Sort of pointer that points on that (ii)-object in the message
 * @param p Pointer where the data should get stored
 * @returns Returns 1 when everything went right, otherwise 0
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

	dbg(0, " x -> %x  y -> %x\n", p->x, p->y);
	
	dbus_message_iter_next(&iter2);

	if (dbus_message_iter_get_arg_type(&iter2) != DBUS_TYPE_INVALID)
		return 0;
	
	return 1;
}

/**
 * @brief Shows up a message
 * @param connection The DBusConnection object through which \a message arrived
 * @param message The DBusMessage containing the coordinates
 * @returns An empty reply if everything went right, otherwise DBUS_HANDLER_RESULT_NOT_YET_HANDLED
 */

static DBusHandlerResult
request_navit_add_message(DBusConnection *connection, DBusMessage *message)
{
	struct navit *navit;
	char *usermessage;
    
    DBusMessageIter iter;

	navit=object_get_from_message(message, "navit");
	if (! navit)
		return dbus_error_invalid_object_path(connection, message);

	dbus_message_iter_init(message, &iter);
	dbus_message_iter_get_basic(&iter, &usermessage);
	
    navit_add_message(navit, usermessage);
	
    return empty_reply(connection, message);
}


/**
 * @brief Centers the screen on a specified position \a pc on the world
 * @param connection The DBusConnection object through which \a message arrived
 * @param message The DBusMessage containing the coordinates
 * @returns An empty reply if everything went right, otherwise DBUS_HANDLER_RESULT_NOT_YET_HANDLED
 */

static DBusHandlerResult
request_navit_set_center(DBusConnection *connection, DBusMessage *message)
{
	struct pcoord pc;
	struct navit *navit;
	DBusMessageIter iter;

	navit=object_get_from_message(message, "navit");
	if (! navit)
		return dbus_error_invalid_object_path(connection, message);

	dbus_message_iter_init(message, &iter);

	if (!pcoord_get_from_message(message, &iter, &pc))
		return dbus_error_invalid_parameter(connection, message);
    
	navit_set_center(navit, &pc, 0);
	return empty_reply(connection, message);
}

/**
 * @brief Centers the screen on a specified position \a p shown on the screen
 * @param connection The DBusConnection object through which \a message arrived
 * @param message The DBusMessage containing the x and y value
 * @returns An empty reply if everything went right, otherwise DBUS_HANDLER_RESULT_NOT_YET_HANDLED
 */
static DBusHandlerResult
request_navit_set_center_screen(DBusConnection *connection, DBusMessage *message)
{
	struct point p;
	struct navit *navit;
	DBusMessageIter iter;

	navit=object_get_from_message(message, "navit");
	if (! navit)
		return dbus_error_invalid_object_path(connection, message);

	dbus_message_iter_init(message, &iter);

	if (!point_get_from_message(message, &iter, &p))
		return dbus_error_invalid_parameter(connection, message);
	navit_set_center_screen(navit, &p, 0);
	return empty_reply(connection, message);
}

/**
 * @brief Sets the layout to \a new_layout_name extracted from \a message
 * @param connection The DBusConnection object through which \a message arrived
 * @param message The DBusMessage containing the name of the layout
 * @returns An empty reply if everything went right, otherwise DBUS_HANDLER_RESULT_NOT_YET_HANDLED
 */
static DBusHandlerResult
request_navit_set_layout(DBusConnection *connection, DBusMessage *message)
{
	char *new_layout_name;
	struct navit *navit;
	struct attr attr;
	struct attr_iter *iter;

	navit=object_get_from_message(message, "navit");
	if (! navit)
		return dbus_error_invalid_object_path(connection, message);
	
	if (!dbus_message_get_args(message, NULL, DBUS_TYPE_STRING, &new_layout_name, DBUS_TYPE_INVALID))
		return dbus_error_invalid_parameter(connection, message);
	
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
	struct point p, *pp=NULL;
	struct navit *navit;
	DBusMessageIter iter;

	navit = object_get_from_message(message, "navit");
	if (! navit)
		return dbus_error_invalid_object_path(connection, message);

	dbus_message_iter_init(message, &iter);
	dbg(0,"%s\n", dbus_message_iter_get_signature(&iter));
	
	dbus_message_iter_get_basic(&iter, &factor);
	
	if (dbus_message_iter_has_next(&iter))
	{
		dbus_message_iter_next(&iter);
		if (!point_get_from_message(message, &iter, &p))
			return dbus_error_invalid_parameter(connection, message);
		pp=&p;
	}

	if (factor > 1)
		navit_zoom_in(navit, factor, pp);
	else if (factor < -1)
		navit_zoom_out(navit, 0-factor, pp);

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
		return dbus_error_invalid_object_path(connection, message);

	dbus_message_iter_init(message, &iter);
	dbg(0,"%s\n", dbus_message_iter_get_signature(&iter));
	
	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_INT32)
		return dbus_error_invalid_parameter(connection, message);
	dbus_message_iter_get_basic(&iter, &w);
	
	dbus_message_iter_next(&iter);
	
	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_INT32)
		return dbus_error_invalid_parameter(connection, message);
	dbus_message_iter_get_basic(&iter, &h);

	dbg(0, " w -> %i  h -> %i\n", w, h);
	
	navit_handle_resize(navit, w, h);

	return empty_reply(connection, message);

}

static DBusHandlerResult
request_navit_get_attr(DBusConnection *connection, DBusMessage *message)
{
	return request_get_attr(connection, message, "navit", NULL, (int (*)(void *, enum attr_type, struct attr *, struct attr_iter *))navit_get_attr);
}

static DBusHandlerResult
request_navit_attr_iter(DBusConnection *connection, DBusMessage *message)
{
	DBusMessage *reply;
	struct attr_iter *attr_iter=navit_attr_iter_new();
	char *opath=object_new("navit_attr_iter",attr_iter);
	reply = dbus_message_new_method_return(message);
	dbus_message_append_args(reply, DBUS_TYPE_OBJECT_PATH, &opath, DBUS_TYPE_INVALID);
	dbus_connection_send (connection, reply, NULL);
	dbus_message_unref (reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
request_navit_attr_iter_destroy(DBusConnection *connection, DBusMessage *message)
{
	struct attr_iter *attr_iter;
	DBusMessageIter iter;

	dbus_message_iter_init(message, &iter);
	attr_iter=object_get_from_message_arg(&iter, "navit_attr_iter");
	if (! attr_iter)
		return dbus_error_invalid_object_path_parameter(connection, message);
	navit_attr_iter_destroy(attr_iter);

	return empty_reply(connection, message);
}


static DBusHandlerResult
request_navit_set_attr(DBusConnection *connection, DBusMessage *message)
{
	return request_set_add_remove_attr(connection, message, "navit", NULL, (int (*)(void *, struct attr *))navit_set_attr);
}

static DBusHandlerResult
request_navit_add_attr(DBusConnection *connection, DBusMessage *message)
{
	return request_set_add_remove_attr(connection, message, "navit", NULL, (int (*)(void *, struct attr *))navit_add_attr);
}

static DBusHandlerResult
request_navit_remove_attr(DBusConnection *connection, DBusMessage *message)
{
	return request_set_add_remove_attr(connection, message, "navit", NULL, (int (*)(void *, struct attr *))navit_remove_attr);
}

static DBusHandlerResult
request_navit_set_position(DBusConnection *connection, DBusMessage *message)
{
	struct pcoord pc;
	struct navit *navit;
	DBusMessageIter iter;

	navit = object_get_from_message(message, "navit");
	if (! navit)
		return dbus_error_invalid_object_path(connection, message);

	dbus_message_iter_init(message, &iter);
	if (!pcoord_get_from_message(message, &iter, &pc))
    		return dbus_error_invalid_parameter(connection, message);
	
	navit_set_position(navit, &pc);
	return empty_reply(connection, message);
}

static DBusHandlerResult
request_navit_set_destination(DBusConnection *connection, DBusMessage *message)
{
	struct pcoord pc;
	struct navit *navit;
	DBusMessageIter iter;
	char *description;

	navit = object_get_from_message(message, "navit");
	if (! navit)
		return dbus_error_invalid_object_path(connection, message);

	dbus_message_iter_init(message, &iter);
	if (!pcoord_get_from_message(message, &iter, &pc))
    		return dbus_error_invalid_parameter(connection, message);
	
	dbus_message_iter_next(&iter);
	dbus_message_iter_get_basic(&iter, &description);
	dbg(0, " destination -> %s\n", description);
	
	navit_set_destination(navit, &pc, description, 1);
	return empty_reply(connection, message);
}

static DBusHandlerResult
request_navit_evaluate(DBusConnection *connection, DBusMessage *message)
{
	struct navit *navit;
	char *command;
	char *result;
	struct attr attr;
	DBusMessage *reply;
	int error;

	navit = object_get_from_message(message, "navit");
	if (! navit)
		return dbus_error_invalid_object_path(connection, message);

	attr.type=attr_navit;
	attr.u.navit=navit;
        if (!dbus_message_get_args(message, NULL, DBUS_TYPE_STRING, &command, DBUS_TYPE_INVALID))
    		return dbus_error_invalid_parameter(connection, message);
	result=command_evaluate_to_string(&attr, command, &error);
	reply = dbus_message_new_method_return(message);
	if (error)
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &error, DBUS_TYPE_INVALID);
	else if (result)
		dbus_message_append_args(reply, DBUS_TYPE_STRING, &result, DBUS_TYPE_INVALID);
	dbus_connection_send (connection, reply, NULL);
	dbus_message_unref (reply);
	return DBUS_HANDLER_RESULT_HANDLED;
}

/* search_list */

static DBusHandlerResult
request_search_list_destroy(DBusConnection *connection, DBusMessage *message)
{
	return request_destroy(connection, message, "search_list", NULL, (void (*)(void *)) search_list_destroy);
}

static DBusHandlerResult
request_search_list_get_result(DBusConnection *connection, DBusMessage *message)
{
	struct search_list *search_list;
	struct search_list_result *result;
	DBusMessage *reply;
	DBusMessageIter iter,iter2,iter3,iter4;
	char *country="country";
	char *town="town";
	char *street="street";

	search_list = object_get_from_message(message, "search_list");
	if (! search_list)
		return dbus_error_invalid_object_path(connection, message);
	result=search_list_get_result(search_list);
	if (!result)
		return empty_reply(connection, message);
	reply = dbus_message_new_method_return(message);
	dbus_message_iter_init_append(reply, &iter);
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &result->id);
	pcoord_encode(&iter, result->c);
	dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sa{sv}}", &iter2);
	if (result->country && (result->country->car || result->country->iso2 || result->country->iso3 || result->country->name)) {
		dbus_message_iter_open_container(&iter2, DBUS_TYPE_DICT_ENTRY, NULL, &iter3);
		dbus_message_iter_append_basic(&iter3, DBUS_TYPE_STRING, &country);
		dbus_message_iter_open_container(&iter3, DBUS_TYPE_ARRAY, "{sv}", &iter4);
		if (result->country->car) 
			encode_dict_string_variant_string(&iter4, "car", result->country->car);
		if (result->country->iso2) 
			encode_dict_string_variant_string(&iter4, "iso2", result->country->iso2);
		if (result->country->iso3) 
			encode_dict_string_variant_string(&iter4, "iso3", result->country->iso3);
		if (result->country->name) 
			encode_dict_string_variant_string(&iter4, "name", result->country->name);
		dbus_message_iter_close_container(&iter3, &iter4);
		dbus_message_iter_close_container(&iter2, &iter3);
	}
	if (result->town && (result->town->common.district_name || result->town->common.town_name)) {
		dbus_message_iter_open_container(&iter2, DBUS_TYPE_DICT_ENTRY, NULL, &iter3);
		dbus_message_iter_append_basic(&iter3, DBUS_TYPE_STRING, &town);
		dbus_message_iter_open_container(&iter3, DBUS_TYPE_ARRAY, "{sv}", &iter4);
		if (result->town->common.district_name)
			encode_dict_string_variant_string(&iter4, "district", result->town->common.district_name);
		if (result->town->common.town_name)
			encode_dict_string_variant_string(&iter4, "name", result->town->common.town_name);
		dbus_message_iter_close_container(&iter3, &iter4);
		dbus_message_iter_close_container(&iter2, &iter3);
	}
	if (result->street && result->street->name) {
		dbus_message_iter_open_container(&iter2, DBUS_TYPE_DICT_ENTRY, NULL, &iter3);
		dbus_message_iter_append_basic(&iter3, DBUS_TYPE_STRING, &street);
		dbus_message_iter_open_container(&iter3, DBUS_TYPE_ARRAY, "{sv}", &iter4);
		if (result->street->name)
			encode_dict_string_variant_string(&iter4, "name", result->street->name);
		dbus_message_iter_close_container(&iter3, &iter4);
		dbus_message_iter_close_container(&iter2, &iter3);
	}
	dbus_message_iter_close_container(&iter, &iter2);
	dbus_connection_send (connection, reply, NULL);
	dbus_message_unref (reply);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
request_search_list_new(DBusConnection *connection, DBusMessage *message)
{
	DBusMessageIter iter;
	DBusMessage *reply;
	struct mapset *mapset;
	struct search_list *search_list;
	char *opath;

	dbus_message_iter_init(message, &iter);
	mapset=object_get_from_message_arg(&iter, "mapset");
	if (! mapset)
		return dbus_error_invalid_object_path_parameter(connection, message);
	search_list=search_list_new(mapset);
	opath=object_new("search_list", search_list);
	reply = dbus_message_new_method_return(message);
	dbus_message_append_args(reply, DBUS_TYPE_OBJECT_PATH, &opath, DBUS_TYPE_INVALID);
	dbus_connection_send (connection, reply, NULL);
	dbus_message_unref (reply);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
request_search_list_search(DBusConnection *connection, DBusMessage *message)
{
	DBusMessageIter iter;
	struct search_list *search_list;
	struct attr attr;
	int partial;

	search_list = object_get_from_message(message, "search_list");
	if (! search_list)
		return dbus_error_invalid_object_path(connection, message);

	dbus_message_iter_init(message, &iter);
	if (!decode_attr_from_iter(&iter, &attr))
    		return dbus_error_invalid_parameter(connection, message);
	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_INT32) 
    		return dbus_error_invalid_parameter(connection, message);
	dbus_message_iter_get_basic(&iter, &partial);
	search_list_search(search_list, &attr, partial);
	return empty_reply(connection, message);
}

static DBusHandlerResult
request_search_list_select(DBusConnection *connection, DBusMessage *message)
{
	DBusMessageIter iter;
	struct search_list *search_list;
	int id, mode;
	enum attr_type attr_type;

	search_list = object_get_from_message(message, "search_list");
	if (! search_list)
		return dbus_error_invalid_object_path(connection, message);

	dbus_message_iter_init(message, &iter);
	attr_type=decode_attr_type_from_iter(&iter);	
	if (attr_type == attr_none)
    		return dbus_error_invalid_parameter(connection, message);
	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_INT32) 
    		return dbus_error_invalid_parameter(connection, message);
	dbus_message_iter_get_basic(&iter, &id);
	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_INT32) 
    		return dbus_error_invalid_parameter(connection, message);
	dbus_message_iter_get_basic(&iter, &mode);
	search_list_select(search_list, attr_type, id, mode);
	return empty_reply(connection, message);
}

/* vehicle */

static DBusHandlerResult
request_vehicle_set_attr(DBusConnection *connection, DBusMessage *message)
{
	struct vehicle *vehicle;
    	struct attr attr;
	int ret;
	
	vehicle = object_get_from_message(message, "vehicle");
	if (! vehicle)
		return dbus_error_invalid_object_path(connection, message);
	if (decode_attr(message, &attr)) {
		ret=vehicle_set_attr(vehicle, &attr);
		destroy_attr(&attr);
		if (ret)	
			return empty_reply(connection, message);
	}
    	return dbus_error_invalid_parameter(connection, message);
}


struct dbus_method {
	char *path;
	char *method;
	char *signature;
    char *signature_name;
    char *response;
    char *response_name;
	DBusHandlerResult(*func)(DBusConnection *connection, DBusMessage *message);
} dbus_methods[] = {
	{"",        "attr_iter",           "",        "",                                        "o",  "attr_iter",  request_config_attr_iter},
	{"",        "attr_iter_destroy",   "o",       "attr_iter",                               "",   "",      request_config_attr_iter_destroy},
	{"",        "get_attr",            "s",       "attrname",                                "sv", "attrname,value",request_config_get_attr},
	{"",        "get_attr_wi",         "so",      "attrname,attr_iter",                      "sv", "attrname,value",request_config_get_attr},
	{"",	    "callback_new",	   "s",       "signalname",                              "o",  "callback",request_callback_new},
	{"",	    "callback_attr_new",   "ss",       "signalname,attribute",                   "o",  "callback",request_callback_new},
	{"",	    "search_list_new",	   "o",       "mapset",                                  "o",  "search",request_search_list_new},
	{".callback","destroy",            "",        "",                                        "",   "",      request_callback_destroy},
	{".graphics","get_data", 	   "s",	      "type",				 	 "ay",  "data", request_graphics_get_data},
	{".graphics","set_attr",           "sv",      "attribute,value",                         "",   "",      request_graphics_set_attr},
	{".gui",     "get_attr",           "s",       "attribute",                               "sv",  "attrname,value", request_gui_get_attr},
	{".gui",     "command_parameter",  "sa{sa{sv}}","command,parameter",                     "a{sa{sv}}",  "return", request_gui_command},
	{".gui",     "command",            "s",       "command",                                 "a{sa{sv}}",  "return", request_gui_command},
	{".navit",  "draw",                "",        "",                                        "",   "",      request_navit_draw},
	{".navit",  "add_message",         "s",       "message",                                 "",   "",      request_navit_add_message},
	{".navit",  "set_center_by_string","s",       "(coordinates)",                           "",   "",      request_navit_set_center},
	{".navit",  "set_center",          "(is)",    "(projection,coordinates)",                "",   "",      request_navit_set_center},
	{".navit",  "set_center",          "(iii)",   "(projection,longitude,latitude)",         "",   "",      request_navit_set_center},
	{".navit",  "set_center_screen",   "(ii)",    "(pixel_x,pixel_y)",                       "",   "",      request_navit_set_center_screen},
	{".navit",  "set_layout",          "s",       "layoutname",                              "",   "",      request_navit_set_layout},
	{".navit",  "zoom",                "i(ii)",   "factor(pixel_x,pixel_y)",                 "",   "",      request_navit_zoom},
	{".navit",  "zoom",                "i",       "factor",                                  "",   "",      request_navit_zoom},
	{".navit",  "resize",              "ii",      "upperleft,lowerright",                    "",   "",      request_navit_resize},
	{".navit",  "attr_iter",           "",        "",                                        "o",  "attr_iter",  request_navit_attr_iter},
	{".navit",  "attr_iter_destroy",   "o",       "attr_iter",                               "",   "",      request_navit_attr_iter_destroy},
	{".navit",  "get_attr",            "s",       "attribute",                               "sv",  "attrname,value", request_navit_get_attr},
	{".navit",  "get_attr_wi",         "so",      "attribute,attr_iter",                     "sv",  "attrname,value", request_navit_get_attr},
	{".navit",  "set_attr",            "sv",      "attribute,value",                         "",   "",      request_navit_set_attr},
	{".navit",  "add_attr",            "sv",      "attribute,value",                         "",   "",      request_navit_add_attr},
	{".navit",  "remove_attr",         "sv",      "attribute,value",                         "",   "",      request_navit_remove_attr},
	{".navit",  "set_position",        "s",       "(coordinates)",                           "",   "",      request_navit_set_position},
	{".navit",  "set_position",        "(is)",    "(projection,coordinated)",                "",   "",      request_navit_set_position},
	{".navit",  "set_position",        "(iii)",   "(projection,longitude,latitude)",         "",   "",      request_navit_set_position},
	{".navit",  "set_destination",     "ss",      "coordinates,comment",                     "",   "",      request_navit_set_destination},
	{".navit",  "set_destination",     "(is)s",   "(projection,coordinates)comment",         "",   "",      request_navit_set_destination},
	{".navit",  "set_destination",     "(iii)s",  "(projection,longitude,latitude)comment",  "",   "",      request_navit_set_destination},
	{".navit",  "evaluate", 	   "s",	      "command",				 "s",  "",      request_navit_evaluate},
	{".map",    "get_attr",            "s",       "attribute",                               "sv",  "attrname,value", request_map_get_attr},
	{".map",    "set_attr",            "sv",      "attribute,value",                         "",   "",      request_map_set_attr},
	{".mapset", "attr_iter",           "",        "",                                        "o",  "attr_iter",  request_mapset_attr_iter},
	{".mapset", "attr_iter_destroy",   "o",       "attr_iter",                               "",   "",      request_mapset_attr_iter_destroy},
	{".mapset", "get_attr",            "s",       "attribute",                               "sv",  "attrname,value", request_mapset_get_attr},
	{".mapset", "get_attr_wi",         "so",      "attribute,attr_iter",                     "sv",  "attrname,value", request_mapset_get_attr},
	{".route",    "get_attr",          "s",       "attribute",                               "sv",  "attrname,value", request_route_get_attr},
	{".route",    "set_attr",          "sv",      "attribute,value",                         "",    "",  request_route_set_attr},
	{".route",    "add_attr",          "sv",      "attribute,value",                         "",    "",  request_route_add_attr},
	{".route",    "remove_attr",       "sv",      "attribute,value",                         "",    "",  request_route_remove_attr},
	{".search_list","destroy",         "",        "",                                        "",   "",      request_search_list_destroy},
	{".search_list","get_result",      "",        "",                                        "i(iii)a{sa{sv}}",   "id,coord,dict",      request_search_list_get_result},
	{".search_list","search",          "svi",     "attribute,value,partial",                 "",   "",      request_search_list_search},
	{".search_list","select",          "sii",     "attribute_type,id,mode",                  "",   "",      request_search_list_select},
	{".vehicle","set_attr",            "sv",      "attribute,value",                         "",   "",      request_vehicle_set_attr},
};

static char *
introspect_path(const char *object)
{
	char *ret;
	int i;
	char *def=".default_";
	int def_len=strlen(def);
	if (strncmp(object, object_path, strlen(object_path)))
		return NULL;
	ret=g_strdup(object+strlen(object_path));
	dbg(1,"path=%s\n",ret);
	for (i = strlen(ret)-1 ; i >= 0 ; i--) {
		if (ret[i] == '/' || (ret[i] >= '0' && ret[i] <= '9'))
			ret[i]='\0';
		else
			break;
	}
	for (i = 0 ; i < strlen(ret); i++)
		if (ret[i] == '/')
			ret[i]='.';
	
	for (i = strlen(ret)-1 ; i >= 0 ; i--) {
		if (!strncmp(ret+i, def, def_len)) {
			memmove(ret+1,ret+i+def_len,strlen(ret+i+def_len)+1);
			break;
		}
	}
	return ret;
}

static char *
generate_navitintrospectxml(const char *object)
{
    int i,methods_size,n=0;
    char *navitintrospectxml;
    char *path=introspect_path(object);
    if (!path)
	return NULL;
    dbg(1,"path=%s\n",path);
    
    // write header and make navit introspectable
    navitintrospectxml = g_strdup_printf("%s%s%s\n", navitintrospectxml_head1, object, navitintrospectxml_head2);
    
    methods_size=sizeof(dbus_methods)/sizeof(struct dbus_method);
    for (i = 0 ; i < methods_size ; i++) {
        // start new interface if it's the first method or it changed
	if (strcmp(dbus_methods[i].path, path))
		continue;
        if ((n == 0) || strcmp(dbus_methods[i-1].path, dbus_methods[i].path))
            navitintrospectxml = g_strconcat_printf(navitintrospectxml, "  <interface name=\"%s%s\">\n", service_name, dbus_methods[i].path);
	n++;
        
        // start the new method
        navitintrospectxml = g_strconcat_printf(navitintrospectxml, "    <method name=\"%s\">\n", dbus_methods[i].method);

        // set input signature if existent
        if (strcmp(dbus_methods[i].signature, ""))
            navitintrospectxml = g_strconcat_printf(navitintrospectxml, "      <arg direction=\"in\" name=\"%s\" type=\"%s\" />\n", dbus_methods[i].signature_name, dbus_methods[i].signature);
        
        // set response signature if existent
        if (strcmp(dbus_methods[i].response, ""))
            navitintrospectxml = g_strconcat_printf(navitintrospectxml, "      <arg direction=\"out\" name=\"%s\" type=\"%s\" />\n", dbus_methods[i].response_name, dbus_methods[i].response);
        
        // close the method
        navitintrospectxml = g_strconcat_printf(navitintrospectxml, "    </method>\n");
        
        // close the interface if we reached the last method or the interface changes
        if ((methods_size == i+1) || ((methods_size > i+1) && strcmp(dbus_methods[i+1].path, dbus_methods[i].path)))
            navitintrospectxml = g_strconcat_printf(navitintrospectxml, "  </interface>\n\n");
    }
    // close the "mother tag"
    navitintrospectxml = g_strconcat_printf(navitintrospectxml, "</node>\n");
    
    return navitintrospectxml;
}

static DBusHandlerResult
navit_handler_func(DBusConnection *connection, DBusMessage *message, void *user_data)
{
	int i;
	char *path;
	dbg(0,"type=%s interface=%s path=%s member=%s signature=%s\n", dbus_message_type_to_string(dbus_message_get_type(message)), dbus_message_get_interface(message), dbus_message_get_path(message), dbus_message_get_member(message), dbus_message_get_signature(message));
	if (dbus_message_is_method_call (message, "org.freedesktop.DBus.Introspectable", "Introspect")) {
		DBusMessage *reply;
            	char *navitintrospectxml = generate_navitintrospectxml(dbus_message_get_path(message));
		dbg(1,"Introspect %s:Result:%s\n",dbus_message_get_path(message), navitintrospectxml);
		if (navitintrospectxml) {
			reply = dbus_message_new_method_return(message);
			dbus_message_append_args(reply, DBUS_TYPE_STRING, &navitintrospectxml, DBUS_TYPE_INVALID);
			dbus_connection_send (connection, reply, NULL);
			dbus_message_unref (reply);
			g_free(navitintrospectxml);
			return DBUS_HANDLER_RESULT_HANDLED;
		}
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}
	
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

static int
dbus_cmd_send_signal(struct navit *navit, char *command, struct attr **in, struct attr ***out)
{
	DBusMessage* msg;
	char *opath=object_new("navit",navit);
	char *interface=g_strdup_printf("%s%s", service_name, ".navit");
	dbg(0,"enter %s %s %s\n",opath,command,interface);
	msg = dbus_message_new_signal(opath, interface, "signal");
	if (msg) {
		DBusMessageIter iter1,iter2;
		dbus_message_iter_init_append(msg, &iter1);
		dbus_message_iter_open_container(&iter1, DBUS_TYPE_ARRAY, "{sv}", &iter2);
		if (in) {
			while (*in) {
				encode_attr(&iter2, *in);
				in++;
			}
		}
		dbus_message_iter_close_container(&iter1, &iter2);
		dbus_connection_send(connection, msg, &dbus_serial);
		dbus_connection_flush(connection);
		dbus_message_unref(msg);
	}
	g_free(interface);
	return 0;
}
     

static struct command_table commands[] = {
        {"dbus_send_signal",command_cast(dbus_cmd_send_signal)},
};


static void
dbus_main_navit(struct navit *navit, int added)
{
	struct attr attr;
	if (added) {
		DBusMessage* msg;
		char *opath=object_new("navit",navit);
		char *interface=g_strdup_printf("%s%s", service_name, ".navit");
		command_add_table_attr(commands, sizeof(commands)/sizeof(struct command_table), navit, &attr);
		navit_add_attr(navit, &attr);
		msg = dbus_message_new_signal(opath, interface, "startup");
		dbus_connection_send(connection, msg, &dbus_serial);
		dbus_connection_flush(connection);
		dbus_message_unref(msg);
		g_free(interface);
	}
}

void plugin_init(void)
{
	DBusError error;

	struct attr callback;
	object_hash=g_hash_table_new(g_str_hash, g_str_equal);
	object_hash_rev=g_hash_table_new(NULL, NULL);
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
	callback.type=attr_callback;
	callback.u.callback=callback_new_attr_0(callback_cast(dbus_main_navit),attr_navit);
	config_add_attr(config, &callback);
}

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
#include "command.h"
#include "graphics.h"
#include "util.h"


static DBusConnection *connection;

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
resolve_object(const char *opath, char *type)
{
	char *prefix;
	void *ret=NULL;
	char *def_navit="/default_navit";
	char *def_graphics="/default_graphics";
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
	prefix=opath+strlen(object_path);
	if (!strncmp(prefix,def_navit,strlen(def_navit))) {
		prefix+=strlen(def_navit);
		struct navit *navit=main_get_navit(NULL);
		if (!prefix[0]) {
			dbg(0,"default_navit\n");
			return navit;
		}
		if (!strncmp(prefix,def_graphics,strlen(def_graphics))) {
			if (navit_get_attr(navit, attr_graphics, &attr, NULL)) {
				return attr.u.graphics;
			}
			return NULL;
		}
	}
	return NULL;
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
	return resolve_object(opath, type);
}

static void *
object_get_from_message(DBusMessage *message, char *type)
{
	return resolve_object(dbus_message_get_path(message), type);
}

static DBusHandlerResult
reply_simple_as_variant(DBusConnection *connection, DBusMessage *message, int value, int dbus_type)
{
	DBusMessage *reply;
    
	reply = dbus_message_new_method_return(message);
    dbus_message_append_args(reply,
                             dbus_type, &value,
                             DBUS_TYPE_INVALID);
	dbus_connection_send (connection, reply, NULL);
	dbus_message_unref (reply);

	return DBUS_HANDLER_RESULT_HANDLED;
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

static DBusHandlerResult
request_navit_draw(DBusConnection *connection, DBusMessage *message)
{
	struct navit *navit;

	navit=object_get_from_message(message, "navit");
	if (! navit)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

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
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

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
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	dbus_message_iter_init(message, &iter);

	if (!pcoord_get_from_message(message, &iter, &pc))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    
	navit_set_center(navit, &pc);
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
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	dbus_message_iter_init(message, &iter);

	if (!point_get_from_message(message, &iter, &p))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	navit_set_center_screen(navit, &p);
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
		if (p && !point_get_from_message(message, &iter, p))
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
	
	navit_handle_resize(navit, w, h);

	return empty_reply(connection, message);

}

static DBusHandlerResult
request_navit_get_attr(DBusConnection *connection, DBusMessage *message)
{
    struct navit *navit;
    DBusMessageIter iter;
    char * attr_type = NULL;
    struct attr attr;
    navit = object_get_from_message(message, "navit");
    if (! navit)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    dbus_message_iter_init(message, &iter);
    dbus_message_iter_get_basic(&iter, &attr_type);
    attr.type = attr_from_name(attr_type); 
    dbg(0, "attr value: 0x%x string: %s\n", attr.type, attr_type);

    if (attr.type == attr_none)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    
    if (attr.type > attr_type_item_begin && attr.type < attr_type_item_end)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    else if (attr.type > attr_type_int_begin && attr.type < attr_type_boolean_begin)
    {
        dbg(0, "int detected\n");
        if(navit_get_attr(navit, attr.type, &attr, NULL)) {
            dbg(0, "%s = %i\n", attr_type, attr.u.num);
            return reply_simple_as_variant(connection, message, attr.u.num, DBUS_TYPE_INT32);
        }
    }

    else if(attr.type > attr_type_boolean_begin && attr.type < attr_type_int_end)
    {
        dbg(0, "bool detected\n");
        if(navit_get_attr(navit, attr.type, &attr, NULL)) {
            dbg(0, "%s = %i\n", attr_type, attr.u.num);
            return reply_simple_as_variant(connection, message, attr.u.num, DBUS_TYPE_BOOLEAN);
        }
    }

    else if(attr.type > attr_type_string_begin && attr.type < attr_type_string_end)
    {
        dbg(0, "string detected\n");
        if(navit_get_attr(navit, attr.type, &attr, NULL)) {
            dbg(0, "%s = %s\n", attr_type, &attr.u.layout);
            return reply_simple_as_variant(connection, message, &attr.u.layout, DBUS_TYPE_STRING);
        }
    }

#if 0
    else if(attr.type > attr_type_special_begin && attr.type < attr_type_special_end)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    else if(attr.type > attr_type_double_begin && attr.type < attr_type_double_end)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    else if(attr.type > attr_type_coord_geo_begin && attr.type < attr_type_coord_geo_end)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    else if(attr.type > attr_type_color_begin && attr.type < attr_type_color_end)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    else if(attr.type > attr_type_object_begin && attr.type < attr_type_object_end)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    else if(attr.type > attr_type_coord_begin && attr.type < attr_type_coord_end)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    else if(attr.type > attr_type_pcoord_begin && attr.type < attr_type_pcoord_end)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    else if(attr.type > attr_type_callback_begin && attr.type < attr_type_callback_end)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
#endif
    else {
        dbg(0, "zomg really unhandled111\n");
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


static DBusHandlerResult
request_navit_set_attr(DBusConnection *connection, DBusMessage *message)
{
    struct navit *navit;
	DBusMessageIter iter, iterattr;
    struct attr attr;
    char *attr_type;

	navit = object_get_from_message(message, "navit");
	if (! navit)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    dbus_message_iter_init(message, &iter);
    dbus_message_iter_get_basic(&iter, &attr_type);
    attr.type = attr_from_name(attr_type); 
    dbg(0, "attr value: 0x%x string: %s\n", attr.type, attr_type);
    
    if (attr.type == attr_none)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    
    dbus_message_iter_next(&iter);
    dbus_message_iter_recurse(&iter, &iterattr);
    dbg(0, "seems valid. signature: %s\n", dbus_message_iter_get_signature(&iterattr));
    
    if (attr.type > attr_type_item_begin && attr.type < attr_type_item_end)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    else if (attr.type > attr_type_int_begin && attr.type < attr_type_boolean_begin) {
        if (dbus_message_iter_get_arg_type(&iterattr) == DBUS_TYPE_INT32)
        {
            dbus_message_iter_get_basic(&iterattr, &attr.u.num);
            if (navit_set_attr(navit, &attr))
                return empty_reply(connection, message);
        }
    }
    else if(attr.type > attr_type_boolean_begin && attr.type < attr_type_int_end) {
        if (dbus_message_iter_get_arg_type(&iterattr) == DBUS_TYPE_BOOLEAN)
        {
            dbus_message_iter_get_basic(&iterattr, &attr.u.num);
            if (navit_set_attr(navit, &attr))
                return empty_reply(connection, message);
        }
    }
#if 0
    else if(attr.type > attr_type_string_begin && attr.type < attr_type_string_end)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    else if(attr.type > attr_type_special_begin && attr.type < attr_type_special_end)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    else if(attr.type > attr_type_double_begin && attr.type < attr_type_double_end)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    else if(attr.type > attr_type_coord_geo_begin && attr.type < attr_type_coord_geo_end)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    else if(attr.type > attr_type_color_begin && attr.type < attr_type_color_end)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    else if(attr.type > attr_type_object_begin && attr.type < attr_type_object_end)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    else if(attr.type > attr_type_coord_begin && attr.type < attr_type_coord_end)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    else if(attr.type > attr_type_pcoord_begin && attr.type < attr_type_pcoord_end)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    else if(attr.type > attr_type_callback_begin && attr.type < attr_type_callback_end)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
#endif
    else {
        dbg(0, "zomg really unhandled111\n");
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusHandlerResult
request_navit_set_position(DBusConnection *connection, DBusMessage *message)
{
	struct pcoord pc;
	struct navit *navit;
	DBusMessageIter iter;

	navit = object_get_from_message(message, "navit");
	if (! navit)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	dbus_message_iter_init(message, &iter);
	if (!pcoord_get_from_message(message, &iter, &pc))
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	
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
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	dbus_message_iter_init(message, &iter);
	if (!pcoord_get_from_message(message, &iter, &pc))
	    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	
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
	int *error;

	navit = object_get_from_message(message, "navit");
	if (! navit)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	attr.type=attr_navit;
	attr.u.navit=navit;
        if (!dbus_message_get_args(message, NULL, DBUS_TYPE_STRING, &command, DBUS_TYPE_INVALID))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	result=command_evaluate_to_string(&attr, command, &error);
	reply = dbus_message_new_method_return(message);
	if (error)
		dbus_message_append_args(reply, DBUS_TYPE_INT32, error, DBUS_TYPE_INVALID);
	else
		dbus_message_append_args(reply, DBUS_TYPE_STRING, &result, DBUS_TYPE_INVALID);
	dbus_connection_send (connection, reply, NULL);
	dbus_message_unref (reply);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
request_graphics_get_data(DBusConnection *connection, DBusMessage *message)
{
	struct graphics *graphics;
	char *data;
	struct graphics_data_image *image;
	struct attr attr;
	DBusMessage *reply;
	int *error;
	int i;

	graphics = object_get_from_message(message, "graphics");
	if (! graphics)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

        if (!dbus_message_get_args(message, NULL, DBUS_TYPE_STRING, &data, DBUS_TYPE_INVALID))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
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
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
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
	{"",        "iter",                "",        "",                                        "o",  "navit", request_main_iter},
	{"",        "iter_destroy",        "o",       "navit",                                   "",   "",      request_main_iter_destroy},
	{"",        "get_navit",           "o",       "navit",                                   "o",  "",      request_main_get_navit},
	{".navit",  "draw",                "",        "",                                        "",   "",      request_navit_draw},
	{".navit",  "add_message",         "s",       "message",                                 "",   "",      request_navit_add_message},
	{".navit",  "set_center",          "s",       "(coordinates)",                           "",   "",      request_navit_set_center},
	{".navit",  "set_center",          "(is)",    "(projection,coordinates)",                "",   "",      request_navit_set_center},
	{".navit",  "set_center",          "(iii)",   "(projection,longitude,latitude)",         "",   "",      request_navit_set_center},
	{".navit",  "set_center_screen",   "(ii)",    "(pixel_x,pixel_y)",                       "",   "",      request_navit_set_center_screen},
	{".navit",  "set_layout",          "s",       "layoutname",                              "",   "",      request_navit_set_layout},
	{".navit",  "zoom",                "i(ii)",   "factor(pixel_x,pixel_y)",                 "",   "",      request_navit_zoom},
	{".navit",  "zoom",                "i",       "factor",                                  "",   "",      request_navit_zoom},
	{".navit",  "resize",              "ii",      "upperleft,lowerright",                    "",   "",      request_navit_resize},
	{".navit",  "get_attr",            "s",       "attribute",                               "v",  "value", request_navit_get_attr},
	{".navit",  "set_attr",            "sv",      "attribute,value",                         "",   "",      request_navit_set_attr},
	{".navit",  "set_position",        "s",       "(coordinates)",                           "",   "",      request_navit_set_position},
	{".navit",  "set_position",        "(is)",    "(projection,coordinated)",                "",   "",      request_navit_set_position},
	{".navit",  "set_position",        "(iii)",   "(projection,longitude,latitude)",         "",   "",      request_navit_set_position},
	{".navit",  "set_destination",     "ss",      "coordinates,comment",                     "",   "",      request_navit_set_destination},
	{".navit",  "set_destination",     "(is)s",   "(projection,coordinates)comment",         "",   "",      request_navit_set_destination},
	{".navit",  "set_destination",     "(iii)s",  "(projection,longitude,latitude)comment",  "",   "",      request_navit_set_destination},
	{".navit",  "evaluate", 	   "s",	      "command",				 "s",  "",     request_navit_evaluate},
	{".graphics","get_data", 	   "s",	      "type",				 	 "ay",  "data", request_graphics_get_data},
#if 0
    {".navit",  "toggle_announcer",    "",        "",                                        "",   "",      request_navit_toggle_announcer},
	{".navit",  "toggle_announcer",    "i",       "",                                        "",   "",      request_navit_toggle_announcer},
#endif
};

static char *
introspect_path(char *object)
{
	char *ret,*s,*d;
	int i;
	char *def=".default_";
	int def_len=strlen(def);
	if (strncmp(object, object_path, strlen(object_path)))
		return NULL;
	ret=g_strdup(object+strlen(object_path));
	dbg(0,"path=%s\n",ret);
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
generate_navitintrospectxml(char *object)
{
    int i,n=0;
    char *navitintrospectxml;
    char *path=introspect_path(object);
    if (!path)
	return NULL;
    dbg(0,"path=%s\n",path);
    
    // write header and make navit introspectable
    navitintrospectxml = g_strdup_printf("%s%s%s\n", navitintrospectxml_head1, object, navitintrospectxml_head2);
    
    for (i = 0 ; i < sizeof(dbus_methods)/sizeof(struct dbus_method) ; i++) {
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
        if ((sizeof(dbus_methods)/sizeof(struct dbus_method) == (i+1)) || strcmp(dbus_methods[i+1].path, dbus_methods[i].path))
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
		dbg(0,"Introspect %s:Result:%s\n",dbus_message_get_path(message), navitintrospectxml);
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

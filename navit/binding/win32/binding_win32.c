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
#include <glib.h>
#include <windows.h>
#include "config.h"
#include "config_.h"
#include "navit.h"
#include "coord.h"
#include "point.h"
#include "plugin.h"
#include "debug.h"
#include "item.h"
#include "xmlconfig.h"
#include "attr.h"
#include "layout.h"
#include "navigation.h"
#include "command.h"
#include "callback.h"
#include "graphics.h"
#include "track.h"
#include "vehicle.h"
#include "vehicleprofile.h"
#include "map.h"
#include "mapset.h"
#include "osd.h"
#include "route.h"
#include "search.h"
#include "callback.h"
#include "gui.h"
#include "util.h"
#include "binding_win32.h"

struct win32_binding_private {
    struct navit* navit;
};



/* TODO: do something meaningful here
 *
 */
static int win32_cmd_send_signal(struct navit *navit, char *command, struct attr **in, struct attr ***out) {
    dbg(lvl_error,"this function is a stub");
    if (in) {
        while (*in) {
            dbg(lvl_debug,"another attribute to be sent");
            in++;
        }
    }
    return 0;
}


static struct command_table commands[] = {
    {"win32_send",command_cast(win32_cmd_send_signal)},
};


static void win32_wm_copydata(struct win32_binding_private *this, int *hwndSender, COPYDATASTRUCT *cpd) {
    struct attr navit;
    struct navit_binding_w32_msg *msg;
    navit.type=attr_navit;
    navit.u.navit=this->navit;
    if(cpd->dwData!=NAVIT_BINDING_W32_DWDATA) {
        dbg(lvl_error,"COPYDATA message came with wrong DWDATA value, expected %d, got %d.",NAVIT_BINDING_W32_DWDATA,
            cpd->dwData);
        return;
    }
    if(cpd->cbData<sizeof(*msg)) {
        dbg(lvl_error,"COPYDATA message too short, expected >=%d, got %d.",sizeof(*msg),cpd->cbData);
        return;
    }
    msg=cpd->lpData;
    if(cpd->dwData!=NAVIT_BINDING_W32_VERSION) {
        dbg(lvl_error,"Got request with wrong version number, expected %d, got %d.",NAVIT_BINDING_W32_VERSION,msg->version);
        return;
    }
    if(strcmp(NAVIT_BINDING_W32_MAGIC,msg->magic)) {
        dbg(lvl_error,"Got request with wrong MAGIC, expected %s, got %*s.",NAVIT_BINDING_W32_MAGIC, msg->magic,
            sizeof(msg->magic));
        return;
    }
    dbg(lvl_debug,"Running command %s", msg->text);
    command_evaluate(&navit, msg->text);
}

static void win32_cb_graphics_ready(struct win32_binding_private *this, struct navit *navit) {
    struct graphics *gra;
    struct callback *gcb;

    gcb=callback_new_attr_1(callback_cast(win32_wm_copydata),attr_wm_copydata, this);
    gra=navit_get_graphics(navit);
    dbg_assert(gra);
    graphics_add_callback(gra, gcb);
}

static void win32_main_navit(struct win32_binding_private *this, struct navit *navit, int added) {
    struct attr attr;
    dbg(lvl_debug,"enter");
    if (added==1) {
        dbg(lvl_debug,"enter2");
        this->navit=navit;
        command_add_table_attr(commands, sizeof(commands)/sizeof(struct command_table), navit, &attr);
        navit_add_attr(navit, &attr);
        navit_add_callback(navit,callback_new_attr_1(callback_cast(win32_cb_graphics_ready),attr_graphics_ready, this));
    }

}



void plugin_init(void) {
    struct attr callback;
    struct win32_binding_private *this=g_new0(struct win32_binding_private,1);
    dbg(lvl_debug,"enter");
    callback.type=attr_callback;
    callback.u.callback=callback_new_attr_1(callback_cast(win32_main_navit),attr_navit,this);
    config_add_attr(config, &callback);
}


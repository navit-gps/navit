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

#include <stdlib.h>
#include <glib.h>
#include "config.h"
#include "item.h"
#include "plugin.h"
#include "navit.h"
#include "attr.h"
#include "callback.h"
#include "speech.h"

struct speech_priv {
    struct navit *nav;
};

static int speech_dbus_say(struct speech_priv *this, const char *text) {
    struct attr attr1,attr2,cb,*attr_list[3];
    int valid=0;
    attr1.type=attr_type;
    attr1.u.str="speech";
    attr2.type=attr_data;
    attr2.u.str=(char *)text;
    attr_list[0]=&attr1;
    attr_list[1]=&attr2;
    attr_list[2]=NULL;
    if (navit_get_attr(this->nav, attr_callback_list, &cb, NULL))
        callback_list_call_attr_4(cb.u.callback_list, attr_command, "dbus_send_signal", attr_list, NULL, &valid);
    return 0;
}

static void speech_dbus_destroy(struct speech_priv *this) {
    g_free(this);
}

static struct speech_methods speech_dbus_meth = {
    speech_dbus_destroy,
    speech_dbus_say,
};

static struct speech_priv *speech_dbus_new(struct speech_methods *meth, struct attr **attrs, struct attr *parent) {
    struct speech_priv *this;
    if (!parent || parent->type != attr_navit)
        return NULL;
    this=g_new(struct speech_priv,1);
    this->nav=parent->u.navit;
    *meth=speech_dbus_meth;
    return this;
}


void plugin_init(void) {
    plugin_register_category_speech("dbus", speech_dbus_new);
}

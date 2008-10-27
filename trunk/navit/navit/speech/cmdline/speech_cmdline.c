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
#include "speech.h"

struct speech_priv {
	char *cmdline;
};

static int 
speechd_say(struct speech_priv *this, const char *text)
{
	char *cmdline;

	cmdline=g_strdup_printf(this->cmdline, text);
	return system(cmdline);
}

static void 
speechd_destroy(struct speech_priv *this) {
	g_free(this->cmdline);
	g_free(this);
}

static struct speech_methods speechd_meth = {
	speechd_destroy,
	speechd_say,
};

static struct speech_priv *
speechd_new(struct speech_methods *meth, struct attr **attrs) {
	struct speech_priv *this;
	struct attr *data;
	data=attr_search(attrs, NULL, attr_data);
	if (! data)
		return NULL;
	this=g_new(struct speech_priv,1);
	this->cmdline=g_strdup(data->u.str);
	*meth=speechd_meth;
	return this;
}


void
plugin_init(void)
{
	plugin_register_speech_type("cmdline", speechd_new);
}

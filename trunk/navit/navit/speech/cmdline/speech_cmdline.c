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
#ifdef HAVE_API_WIN32_BASE
#include <windows.h>
#include "util.h"
#endif
#ifdef USE_EXEC
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#endif

struct speech_priv {
	char *cmdline;
};

static int 
speechd_say(struct speech_priv *this, const char *text)
{
#ifdef USE_EXEC
	if (!fork()) {
		char *cmdline=g_strdup_printf(this->cmdline, text);
                int argcmax=10;
                char *argv[argcmax];
                int argc=0;
                char *pos=cmdline,end;
                while (*pos && argc < argcmax-1) {
                        end=' ';
                        if (*pos == '\'' || *pos == '\"') {
                                end=*pos++;
                        }
                        argv[argc]=pos;
                        while (*pos && *pos != end)
                                pos++;
                        if (*pos)
                                *pos++='\0';
                        while (*pos == ' ')
                                pos++;
                        if (strcmp(argv[argc], "2>/dev/null") && strcmp(argv[argc],">/dev/null") && strcmp(argv[argc],"&"))
                                argc++;
                }
                argv[argc++]=NULL;
                execvp(argv[0], argv);
                exit(1);
        }
	return 0;
#else
#ifdef HAVE_API_WIN32_BASE
	char *cmdline,*p;
	PROCESS_INFORMATION pr;
	LPCWSTR cmd,arg;
	int ret;


	cmdline=g_strdup_printf(this->cmdline, text);
	p=cmdline;
	while (*p != ' ' && *p != '\0')
		p++;
	if (*p == ' ')
		*p++='\0';
	cmd = newSysString(cmdline);
	arg = newSysString(p);
	ret=!CreateProcess(cmd, arg, NULL, NULL, 0, CREATE_NEW_CONSOLE, NULL, NULL, NULL, &pr);
	g_free(cmd);
	g_free(arg);
	return ret;
#else
	char *cmdline;

	cmdline=g_strdup_printf(this->cmdline, text);
	return system(cmdline);
#endif
#endif
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

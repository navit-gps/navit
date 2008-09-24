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

#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <glib.h>
#include <sys/types.h>

#ifndef _WIN32
#include <sys/wait.h>
#include <signal.h>
#endif

#include <unistd.h>
#include <libintl.h>
#include "config.h"
#include "file.h"
#include "debug.h"
#include "main.h"
#include "navit.h"
#include "gui.h"
#include "xmlconfig.h"
#include "item.h"
#include "coord.h"
#include "route.h"
#include "navigation.h"
#include "event.h"

#define _(STRING)    gettext(STRING)

struct map_data *map_data_default;

static void sigchld(int sig)
{
#ifndef _WIN32
	int status;
	while (waitpid(-1, &status, WNOHANG) > 0);
#endif
}


gchar *get_home_directory(void)
{
	static gchar *homedir = NULL;

	if (homedir) return homedir;
	homedir = getenv("HOME");
	if (!homedir)
	{
		g_warning("Could not find home directory. Using current directory as home directory.");
		homedir = ".";
	} else {
		homedir=g_strdup(homedir);
	}
	return homedir;
}

static GList *navit;

struct iter {
	GList *list;
};

struct iter *
main_iter_new(void)
{
	struct iter *ret=g_new0(struct iter, 1);
	ret->list=navit;
	return ret;
}

void
main_iter_destroy(struct iter *iter)
{
	g_free(iter);
}

struct navit *
main_get_navit(struct iter *iter)
{
	GList *list;
	struct navit *ret=NULL;
	if (iter)
		list=iter->list;
	else
		list=navit;
	if (list) {
		ret=(struct navit *)(list->data);
		if (iter)
			iter->list=g_list_next(iter->list);
	}
	return ret;
	
}
void
main_add_navit(struct navit *nav)
{
	navit=g_list_prepend(navit, nav);
}

void
main_remove_navit(struct navit *nav)
{
	navit=g_list_remove(navit, nav);
	if (! navit) 
		event_main_loop_quit();
}

void
main_init(char *program)
{
	GError *error = NULL;
	char *config_file = NULL;
	char *s;
	int l;
	int opt;
    GList *list = NULL, *li;


#ifndef _WIN32
	signal(SIGCHLD, sigchld);
#endif

	setenv("LC_NUMERIC","C",1);
	setlocale(LC_ALL,"");
	setlocale(LC_NUMERIC,"C");
	if (file_exists("navit.c") || file_exists("navit.o") || file_exists("navit.lo")) {
		char buffer[PATH_MAX];
		printf(_("Running from source directory\n"));
		getcwd(buffer, PATH_MAX);
		setenv("NAVIT_PREFIX", buffer, 0);
		setenv("NAVIT_LIBDIR", buffer, 0);
		setenv("NAVIT_SHAREDIR", buffer, 0);
		setenv("NAVIT_LIBPREFIX", "*/.libs/", 0);
		s=g_strdup_printf("%s/../locale", buffer);
		setenv("NAVIT_LOCALEDIR", s, 0);
		g_free(s);
		setenv("NAVIT_USER_DATADIR", "./", 0);
	} else {
		if (!getenv("NAVIT_PREFIX")) {
			l=strlen(program);
			if (l > 10 && !strcmp(program+l-10,"/bin/navit")) {
				s=g_strdup(program);
				s[l-10]='\0';
				if (strcmp(s, PREFIX))
					printf(_("setting '%s' to '%s'\n"), "NAVIT_PREFIX", s);
				setenv("NAVIT_PREFIX", s, 0);
				g_free(s);
			} else
				setenv("NAVIT_PREFIX", PREFIX, 0);
		}
#ifdef _WIN32
		s=g_strdup_printf("locale");
#else
		s=g_strdup_printf("%s/share/locale", getenv("NAVIT_PREFIX"));
#endif
		setenv("NAVIT_LOCALEDIR", s, 0);
		g_free(s);
#ifdef _WIN32
		s=g_strdup_printf(".");
#else
		s=g_strdup_printf("%s/share/navit", getenv("NAVIT_PREFIX"));
#endif
		setenv("NAVIT_SHAREDIR", s, 0);
		g_free(s);
		s=g_strdup_printf("%s/lib/navit", getenv("NAVIT_PREFIX"));
		setenv("NAVIT_LIBDIR", s, 0);
		g_free(s);
		setenv("NAVIT_USER_DATADIR", g_strjoin(NULL, get_home_directory(), "/.navit/", NULL), 0);
	}
	if (getenv("LC_ALL")) 
		dbg(0,"Warning: LC_ALL is set, this might lead to problems\n");
	s = getenv("NAVIT_WID");
	if (s) {
		setenv("SDL_WINDOWID", s, 0);
	}
}

void
main_init_nls(void)
{
	bindtextdomain(PACKAGE, getenv("NAVIT_LOCALEDIR"));
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain(PACKAGE);
}

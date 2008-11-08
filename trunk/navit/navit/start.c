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
#include <stdlib.h>
#include <glib.h>
#include <getopt.h>
#include "config.h"
#include "version.h"
#include "item.h"
#include "coord.h"
#include "main.h"
#include "route.h"
#include "navigation.h"
#include "track.h"
#include "debug.h"
#include "event.h"
#include "event_glib.h"
#include "xmlconfig.h"
#include "file.h"
#include "search.h"
#include "navit_nls.h"
#ifdef HAVE_API_WIN32_CE
#include <windows.h>
#include <winbase.h>
#endif

static void
print_usage(void)
{
	printf(_("navit usage:\nnavit [options] [configfile]\n\t-c <file>: use <file> as config file\n\t-d <n>: set the debug output level to <n>. (TODO)\n\t-h: print this usage info and exit.\n\t-v: Print the version and exit.\n"));
}


static gchar *
get_home_directory(void)
{
	static gchar *homedir = NULL;

	if (homedir) return homedir;
	homedir = getenv("HOME");
	if (!homedir)
	{
		dbg(0,"Could not find home directory. Using current directory as home directory.");
		homedir = ".";
	} else {
		homedir=g_strdup(homedir);
	}
	return homedir;
}

static void setup_dummy_environment(void)
{
#ifdef HAVE_API_WIN32_CE
	char buf[PATH_MAX];
	wchar_t filename[MAX_PATH + 1];
	char basedir[MAX_PATH + 1];
	char *cp;
	int sz;

	sz = GetModuleFileName(NULL, filename, MAX_PATH);
	if (sz > 0) {
		wcstombs(basedir, filename, sz + 1);
		cp = strrchr(basedir,'/');
		if (!cp)
			cp = strrchr(basedir,'\\');
		if (cp)
			*cp = '\0';
		else
			sprintf(basedir, ".");
	} else {
		sprintf(basedir, "%s", "/Storage Card/navit");
	}
	sprintf(buf, "%s", basedir);
	setenv("HOME", buf, 0);
	setenv("NAVIT_PREFIX", buf, 0);
	sprintf(buf, "%s/lib", basedir);
	setenv("NAVIT_LIBDIR", buf, 0);
	sprintf(buf, "%s/locale", basedir);
	setenv("NAVIT_LOCALEDIR", buf, 0);
	sprintf(buf, "%s/data", basedir);
	setenv("NAVIT_USER_DATADIR", buf ,0);
	sprintf(buf, "%s", basedir);
	setenv("NAVIT_SHAREDIR", buf, 0);
	sprintf(buf, "%s/navit.log", basedir);
	setenv("NAVIT_LOGFILE", buf, 0);
#endif

}
int main(int argc, char **argv)
{
	GError *error = NULL;
	char *config_file = NULL;
	int opt;
	char *cp;

	GList *list = NULL, *li;


#ifdef HAVE_GLIB
	event_glib_init();
#endif
	setup_dummy_environment();
	main_init(argv[0]);
	main_init_nls();
	debug_init(argv[0]);
	
	cp = getenv("NAVIT_LOGFILE");
	if (cp)
		debug_set_logfile(cp);
#ifdef HAVE_API_WIN32_CE
	else
		debug_set_logfile("/Storage Card/navit.log");
#endif
	file_init();
#ifndef USE_PLUGINS
	extern void builtin_init(void);
	builtin_init();
#endif
	route_init();
	navigation_init();
	tracking_init();
	search_init();
	config_file=NULL;
	opterr=0;  //don't bomb out on errors.
	if (argc > 1) {
		/* DEVELOPPERS : don't forget to update the manpage if you modify theses options */
		while((opt = getopt(argc, argv, ":hvc:d:")) != -1) {
			switch(opt) {
			case 'h':
				print_usage();
				exit(0);
				break;
			case 'v':
				printf("%s %s\n", "navit", PACKAGE_VERSION" "SVN_VERSION); 
				exit(0);
				break;
			case 'c':
				printf("config file n is set to `%s'\n", optarg);
	            config_file = optarg;
				break;
			case 'd':
				printf("TODO Verbose option is set to `%s'\n", optarg);
				break;
			case ':':
				fprintf(stderr, "navit: Error - Option `%c' needs a value\n", optopt);
				print_usage();
				exit(1);
				break;
			case '?':
				fprintf(stderr, "navit: Error - No such option: `%c'\n", optopt);
				print_usage();
				exit(1);
			}
	    }
	}
	// use 1st cmd line option that is left for the config file
	if (optind < argc) config_file = argv[optind];

    // if config file is explicitely given only look for it, otherwise try std paths
	if (config_file) list = g_list_append(list,g_strdup(config_file));
    else {
		list = g_list_append(list,g_strjoin(NULL,get_home_directory(), "/.navit/navit.xml" , NULL));
		list = g_list_append(list,g_strdup("navit.xml.local"));
		list = g_list_append(list,g_strdup("navit.xml"));
		list = g_list_append(list,g_strjoin(NULL,getenv("NAVIT_SHAREDIR"), "/navit.xml.local" , NULL));
		list = g_list_append(list,g_strjoin(NULL,getenv("NAVIT_SHAREDIR"), "/navit.xml" , NULL));
		#ifndef _WIN32
		list = g_list_append(list,g_strdup("/etc/navit/navit.xml"));
		#endif
	}
	li = list;
	do {
		if (li == NULL) {
			// We have not found an existing config file from all possibilities
			printf(_("No config file navit.xml, navit.xml.local found\n"));
			exit(1);
		}
        // Try the next config file possibility from the list
		config_file = li->data;
		if (!file_exists(config_file)) g_free(config_file);
		li = g_list_next(li);
	} while (!file_exists(config_file));
	g_list_free(list);

#ifdef HAVE_GLIB
	event_request_system("glib","start");
#endif
#ifdef HAVE_API_WIN32_CE
	config_file="\\Storage Card\\navit.xml";
#endif
	if (!config_load(config_file, &error)) {
		dbg(0, _("Error parsing '%s': %s\n"), config_file, error ? error->message : "");
	} else {
		dbg(0, _("Using '%s'\n"), config_file);
	}
	if (! main_get_navit(NULL)) {
		dbg(0, _("No instance has been created, exiting\n"));
		exit(1);
	}
	event_main_loop_run();

	return 0;
}

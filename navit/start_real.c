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
#include "config.h"
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include <XGetopt.h>
#endif
#include "config_.h"
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
#include "start_real.h"
#include "linguistics.h"
#include "navit_nls.h"
#include "atom.h"
#include "command.h"
#include "geom.h"
#ifdef HAVE_API_WIN32_CE
#include <windows.h>
#include <winbase.h>
#endif

char *version=PACKAGE_VERSION"+git:"GIT_VERSION""NAVIT_VARIANT;
int main_argc;
char * const* main_argv;

static void print_usage(void) {
    printf("%s",_("navit usage:\n"
                  "navit [options] [configfile]\n"
                  "\t-c <file>: use <file> as config file, instead of using the default file.\n"
                  "\t-d <n>: set the global debug output level to <n> (0=error, 1=warning, 2=info, 3=debug).\n"
                  "\tSettings from config file will still take effect where they set a higher level.\n"
                  "\t-h: print this usage info and exit.\n"
                  "\t-v: print the version and exit.\n"));
}


#ifndef USE_PLUGINS
extern void builtin_init(void);
#endif /* USE_PLUGINS*/

int main_real(int argc, char * const* argv) {
    xmlerror *error = NULL;
    char *config_file = NULL, *command=NULL, *startup_file=NULL;
    int opt;
    char *cp;
    struct attr navit, conf;

    GList *list = NULL, *li;
    main_argc=argc;
    main_argv=argv;

#ifdef HAVE_GLIB
    event_glib_init();
#else
    _g_slice_thread_init_nomessage();
#endif
    atom_init();
    main_init(argv[0]);
    navit_nls_main_init();
    debug_init(argv[0]);

    cp = getenv("NAVIT_LOGFILE");
    if (cp) {
        debug_set_logfile(cp);
    }
#ifdef HAVE_API_WIN32_CE
    else {
        debug_set_logfile("/Storage Card/navit.log");
    }
#endif
    file_init();
#ifndef USE_PLUGINS
    builtin_init();
#endif
    route_init();
    navigation_init();
    tracking_init();
    search_init();
    linguistics_init();
    geom_init();
    config_file=NULL;
#ifdef HAVE_GETOPT_H
    opterr=0;  //don't bomb out on errors.
#endif /* _MSC_VER */
    /* ingore iphone command line argument */
    if (argc == 2 && !strcmp(argv[1],"-RegisterForSystemEvents"))
        argc=1;
    if (argc > 1) {
        /* Don't forget to update the manpage if you modify theses options */
        while((opt = getopt(argc, argv, ":hvc:d:e:s:")) != -1) {
            switch(opt) {
            case 'h':
                print_usage();
                exit(0);
                break;
            case 'v':
                printf("%s %s\n", "navit", version);
                exit(0);
                break;
            case 'c':
                printf("config file n is set to `%s'\n", optarg);
                config_file = optarg;
                break;
            case 'd':
                debug_set_global_level(atoi(optarg), 1);
                break;
            case 'e':
                command=optarg;
                break;
            case 's':
                startup_file=optarg;
                break;
#ifdef HAVE_GETOPT_H
            case ':':
                fprintf(stderr, "navit: Error - Option `%c' needs a value\n", optopt);
                print_usage();
                exit(2);
                break;
            case '?':
                fprintf(stderr, "navit: Error - No such option: `%c'\n", optopt);
                print_usage();
                exit(3);
#endif
            }
        }
        // use 1st cmd line option that is left for the config file
        if (optind < argc) config_file = argv[optind];
    }

    // if config file is explicitely given only look for it, otherwise try std paths
    if (config_file) {
        list = g_list_append(list,g_strdup(config_file));
    } else {
        list = g_list_append(list,g_strjoin(NULL,getenv("NAVIT_USER_DATADIR"), "/navit.xml", NULL));
        list = g_list_append(list,g_strdup("navit.xml.local"));
        list = g_list_append(list,g_strdup("navit.xml"));
        list = g_list_append(list,g_strjoin(NULL,getenv("NAVIT_SHAREDIR"), "/navit.xml.local", NULL));
        list = g_list_append(list,g_strjoin(NULL,getenv("NAVIT_SHAREDIR"), "/navit.xml", NULL));
#ifndef _WIN32
        list = g_list_append(list,g_strdup("/etc/navit/navit.xml"));
#endif
    }
    li = list;
    for (;;) {
        if (li == NULL) {
            // We have not found an existing config file from all possibilities
            dbg(lvl_error, "%s", _("No config file navit.xml, navit.xml.local found"));
            return 4;
        }
        // Try the next config file possibility from the list
        config_file = li->data;
        dbg(lvl_debug,"trying %s",config_file);
        if (file_exists(config_file)) {
            break;
        }
        g_free(config_file);
        li = g_list_next(li);
    }

    dbg(lvl_debug,"Loading %s",config_file);
    if (!config_load(config_file, &error)) {
        dbg(lvl_error, _("Error parsing config file '%s': %s"), config_file, error ? error->message : "");
    } else {
        dbg(lvl_info, _("Using config file '%s'"), config_file);
    }
    if (! config) {
        dbg(lvl_error, _("Error: No configuration found in config file '%s'"), config_file);
    }
    while (li) {
        g_free(li->data);
        li = g_list_next(li);
    }
    g_list_free(list);
    if (! (config && config_get_attr(config, attr_navit, &navit, NULL))) {
        dbg(lvl_error, "%s", _("Internal initialization failed, exiting. Check previous error messages."));
        exit(5);
    }
    conf.type=attr_config;
    conf.u.config=config;
    if (startup_file) {
        FILE *f = fopen(startup_file,"r");
        if (f) {
            char buffer[4096];
            int fclose_ret;
            while(fgets(buffer, sizeof(buffer), f)) {
                command_evaluate(&conf, buffer);
            }
            fclose_ret = fclose(f);
            if (fclose_ret != 0) {
                dbg(lvl_error, "Could not close the specified startup file: %s", startup_file);
            }
        } else {
            dbg(lvl_error, "Could not open the specified startup file: %s", startup_file);
        }
    }
    if (command) {
        command_evaluate(&conf, command);
    }
    event_main_loop_run();

    /* TODO: Android actually has no event loop, so we can't free all allocated resources here. Have to find better place to
     *  free all allocations on program exit. And don't forget to free all the stuff allocated in the code above.
     */
#ifndef HAVE_API_ANDROID
    linguistics_free();
    debug_finished();
#endif
    return 0;
}

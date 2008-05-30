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
print_usage(void)
{
	printf(_("navit usage:\nnavit [options] [configfile]\n\t-c <file>: use <file> as config file\n\t-d <n>: set the debug output level to <n>. (TODO)\n\t-h: print this usage info and exit.\n\t-v: Print the version and exit.\n"));
}

int main(int argc, char **argv)
{
	GError *error = NULL;
	char *config_file = NULL;
	char *s;
	int l;


#ifndef _WIN32
	signal(SIGCHLD, sigchld);
#endif

	setenv("LC_NUMERIC","C",1);
	setlocale(LC_ALL,"");
	setlocale(LC_NUMERIC,"C");
	if (file_exists("navit.c") || file_exists("navit.o")) {
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
	} else {
		if (!getenv("NAVIT_PREFIX")) {
			l=strlen(argv[0]);
			if (l > 10 && !strcmp(argv[0]+l-10,"/bin/navit")) {
				s=g_strdup(argv[0]);
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
	}
        bindtextdomain(PACKAGE, getenv("NAVIT_LOCALEDIR"));
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain(PACKAGE);

	debug_init(argv[0]);
	if (getenv("LC_ALL")) 
		dbg(0,"Warning: LC_ALL is set, this might lead to problems\n");
#ifndef USE_PLUGINS
	extern void builtin_init(void);
	builtin_init();
#endif
#if 0
	/* handled in gui/gtk */
	gtk_set_locale();
	gtk_init(&argc, &argv);
	gdk_rgb_init();
#endif
	s = getenv("NAVIT_WID");
	if (s) {
		setenv("SDL_WINDOWID", s, 0);
	}
	route_init();
	navigation_init();
	config_file=NULL;
	int opt;
	opterr=0;  //don't bomb out on errors.
	if (argc > 1) {
		while((opt = getopt(argc, argv, ":hvc:d:")) != -1) {
			switch(opt) {
			case 'h':
				print_usage();
				exit(0);
				break;
			case 'v':
				printf("%s %s\n", "navit", "0.0.4+svn"); 
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

    GList *list = NULL, *li;
    // if config file is explicitely given only look for it, otherwise try std paths
	if (config_file) list = g_list_append(list,g_strdup(config_file));
    else {
		list = g_list_append(list,g_strjoin(NULL,get_home_directory(), "/.navit/navit.xml" , NULL));
		list = g_list_append(list,g_strdup("navit.xml.local"));
		list = g_list_append(list,g_strdup("navit.xml"));
		list = g_list_append(list,g_strjoin(NULL,getenv("NAVIT_SHAREDIR"), "/navit.xml.local" , NULL));
		list = g_list_append(list,g_strjoin(NULL,getenv("NAVIT_SHAREDIR"), "/navit.xml" , NULL));
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

	if (!config_load(config_file, &error)) {
		printf(_("Error parsing '%s': %s\n"), config_file, error->message);
		exit(1);
	} else {
		printf(_("Using '%s'\n"), config_file);
	}
	if (! navit) {
		printf(_("No instance has been created, exiting\n"));
		exit(1);
	}
	if (main_loop_gui) {
		gui_run_main_loop(main_loop_gui);
	} else 
		event_main_loop_run();

	return 0;
}

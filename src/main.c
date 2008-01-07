#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
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
#ifdef USE_GTK_MAIN_LOOP
#include <gtk/gtk.h>
#endif
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

#define _(STRING)    gettext(STRING)

struct map_data *map_data_default;

static void sigchld(int sig)
{
#ifndef _WIN32
	int status;
	while (waitpid(-1, &status, WNOHANG) > 0);
#endif
}


static gchar *get_home_directory(void)
{
	static gchar *homedir = NULL;

	if (homedir) return homedir;
	homedir = getenv("HOME");
	if (!homedir)
	{
//		struct passwd *p;

// 		p = getpwuid(getuid());
// 		if (p) homedir = p->pw_dir;
	}
	if (!homedir)
	{
		g_warning("Could not find home directory. Using current directory as home directory.");
		homedir = ".";
	}
	return homedir;
}

static GList *navit;
#ifndef USE_GTK_MAIN_LOOP
static GMainLoop *loop;
#endif

void
main_add_navit(struct navit *nav)
{
	navit=g_list_prepend(navit, nav);
}

void
main_remove_navit(struct navit *nav)
{
	navit=g_list_remove(navit, nav);
	if (! navit) {
#ifdef USE_GTK_MAIN_LOOP
		gtk_main_quit();
#else
		if (loop)
			g_main_loop_quit(loop);
#endif
	}
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

	debug_init();
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
	if (argc > 1) 
		config_file=argv[1];
	if (! config_file) {
		config_file=g_strjoin(NULL,get_home_directory(), "/.navit/navit.xml" , NULL);
		if (!file_exists(config_file)) {
			g_free(config_file);
			config_file=NULL;
			}
	}
	if (! config_file) {
		if (file_exists("navit.xml.local"))
			config_file="navit.xml.local";
	}
	if (! config_file) {
		if (file_exists("navit.xml"))
			config_file="navit.xml";
	}
	if (! config_file) {
		config_file=g_strjoin(NULL,getenv("NAVIT_SHAREDIR"), "/navit.xml.local" , NULL);
		if (!file_exists(config_file)) {
			g_free(config_file);
			config_file=NULL;
		}
	}
	if (! config_file) {
		config_file=g_strjoin(NULL,getenv("NAVIT_SHAREDIR"), "/navit.xml" , NULL);
		if (!file_exists(config_file)) {
			g_free(config_file);
			config_file=NULL;
		}
	}
	if (!config_file) {
		printf(_("No config file navit.xml, navit.xml.local found\n"));
		exit(1);
	}
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
	} else {
#ifdef USE_GTK_MAIN_LOOP
		gtk_main();
#else
		loop = g_main_loop_new (NULL, TRUE);
		if (g_main_loop_is_running (loop))
		{
			g_main_loop_run (loop);
		}
#endif
	}

	return 0;
}

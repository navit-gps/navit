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
#include "config.h"
#include "file.h"
#include "debug.h"
#include "main.h"
#include "navit.h"
#include "gui.h"
#include "item.h"
#include "xmlconfig.h"
#include "coord.h"
#include "route.h"
#include "navigation.h"
#include "event.h"
#include "callback.h"
#include "navit_nls.h"
#if HAVE_API_WIN32_BASE
#include <windows.h>
#include <winbase.h>
#endif


struct map_data *map_data_default;

struct callback_list *cbl;


static void sigchld(int sig)
{
#if !defined(_WIN32) && !defined(__CEGCC__)
	int status;
	while (waitpid(-1, &status, WNOHANG) > 0);
#endif
}

#ifdef HAVE_API_WIN32
void
setenv(char *var, char *val, int overwrite)
{
	char *str=g_strdup_printf("%s=%s",var,val);
	if (overwrite || !getenv(var))
		putenv(str);
	g_free(str);
}
#endif

/*
 * environment_vars[][0:name,1-3:mode]
 * ':'  replaced with NAVIT_PREFIX
 * '::' replaced with NAVIT_PREFIX and LIBDIR
 * '~'  replaced with HOME
*/
static char *environment_vars[][5]={
	{"NAVIT_LIBDIR",      ":",          "::/navit",      ":\\lib",      ":/lib"},
	{"NAVIT_SHAREDIR",    ":",          ":/share/navit", ":",           ":/share"},
	{"NAVIT_LOCALEDIR",   ":/../locale",":/share/locale",":\\locale",   ":/locale"},
	{"NAVIT_USER_DATADIR",":",          "~/.navit",      ":\\data",     ":/home"},
#if 1
	{"NAVIT_LOGFILE",     NULL,         NULL,            ":\\navit.log",NULL},
#endif
	{"NAVIT_LIBPREFIX",   "*/.libs/",   NULL,            NULL,          NULL},
	{NULL,                NULL,         NULL,            NULL,          NULL},
};

static void
main_setup_environment(int mode)
{
	int i=0;
	char *var,*val,*homedir;
	while ((var=environment_vars[i][0])) {
		val=environment_vars[i][mode+1];
		if (val) {
			switch (val[0]) {
			case ':':
				if (val[1] == ':')
					val=g_strdup_printf("%s/%s%s", getenv("NAVIT_PREFIX"), LIBDIR+sizeof(PREFIX), val+2);
				else
					val=g_strdup_printf("%s%s", getenv("NAVIT_PREFIX"), val+1);
				break;
			case '~':
				homedir=getenv("HOME");
				if (!homedir)
					homedir="./";
				val=g_strdup_printf("%s%s", homedir, val+1);
				break;
			default:
				val=g_strdup(val);
				break;
			}
			setenv(var, val, 0);
			g_free(val);
		}
		i++;
	}
}

#ifdef HAVE_API_WIN32_BASE
char *nls_table[][3]={
	//{"LANGNAME", "CTRYNAME", "Language Code"},
	{"DAN","DNK","da_DK"},		// Danish
	{"DEU","DEU","de_DE"},		// German
	{"DEA","AUT","de_AT"},		// German - Austrian
	{"ENU","USA","en_US"},		// English - US
	{"FRA","FRA","fr_FR"},		// French
	{"RUS","RUS","ru_RU"},		// Russian
	{"ENI","IRL","en_IE"},		// English - Ireland
	{"PLK", "POL", "pl_PL"},	// Polish
	{"BGR", "BGR", "bg_BG"},	// Bulgarian
	{"ITA", "ITA", "it_IT"},	// Italian
	{"CSY", "CZE", "cs_CZ"},	// Czech
	{"FIN", "FIN", "fi_FI"},	// Finish
	{"ROM", "ROM", "ro_RO"},	// Romanian
	{NULL,NULL,NULL},		
};

static void
win_set_nls(void)
{
	wchar_t wcountry[32],wlang[32];	
	char country[32],lang[32];
	int i=0;

	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVLANGNAME, wlang, sizeof(wlang));
	WideCharToMultiByte(CP_ACP,0,wlang,-1,lang,sizeof(lang),NULL,NULL);
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVCTRYNAME, wcountry, sizeof(wcountry));
	WideCharToMultiByte(CP_ACP,0,wcountry,-1,country,sizeof(country),NULL,NULL);
	while (nls_table[i][0]) {
		if (!strcmp(nls_table[i][0], lang) && !(strcmp(nls_table[i][1], country))) {
			dbg(1,"Setting LANG=%s for Lang %s Country %s\n",nls_table[i][2], lang, country);
			setenv("LANG",nls_table[i][2],0);
			return;
		}
		i++;
	}
	dbg(1,"Lang %s Country %s not found\n",lang,country);
}
#endif

void
main_init(const char *program)
{
	char *s;
	int l;

#ifndef _WIN32
	signal(SIGCHLD, sigchld);
#endif
	cbl=callback_list_new();
#ifdef HAVE_API_WIN32_BASE
	win_set_nls();
#endif
	setenv("LC_NUMERIC","C",1);
	setlocale(LC_ALL,"");
	setlocale(LC_NUMERIC,"C");
#if !defined _WIN32 && !defined _WIN32_WCE
	if (file_exists("navit.c") || file_exists("navit.o") || file_exists("navit.lo")) {
		char buffer[PATH_MAX];
		printf(_("Running from source directory\n"));
		getcwd(buffer, PATH_MAX);		/* libc of navit returns "dummy" */
		setenv("NAVIT_PREFIX", buffer, 0);
		main_setup_environment(0);
	} else {
		if (!getenv("NAVIT_PREFIX")) {
		int progpath_len;
			char *progpath="/bin/navit";
			l=strlen(program);
			progpath_len=strlen(progpath);
			if (l > progpath_len && !strcmp(program+l-progpath_len,progpath)) {
				s=g_strdup(program);
				s[l-progpath_len]='\0';
				if (strcmp(s, PREFIX))
					printf(_("setting '%s' to '%s'\n"), "NAVIT_PREFIX", s);
				setenv("NAVIT_PREFIX", s, 0);
				g_free(s);
			} else
				setenv("NAVIT_PREFIX", PREFIX, 0);
		}
#ifdef HAVE_API_ANDROID
		main_setup_environment(3);
#else
		main_setup_environment(1);
#endif
	}

#else		/* _WIN32 || _WIN32_WCE */
	if (!getenv("NAVIT_PREFIX"))
	{
		char  filename[MAX_PATH + 1],
		     *end;

		*filename = '\0';
#ifdef _UNICODE		/* currently for wince */
		wchar_t wfilename[MAX_PATH + 1];
		if (GetModuleFileNameW(NULL, wfilename, MAX_PATH))
		{
			wcstombs(filename, wfilename, MAX_PATH);
#else
		if (GetModuleFileName(NULL, filename, MAX_PATH))
		{
#endif
			end = strrchr(filename, L'\\');	/* eliminate the file name which is on the right side */
			if(end)
				*end = '\0';
		}
		setenv("NAVIT_PREFIX", filename, 0);
	}
	if (!getenv("HOME"))
		setenv("HOME", getenv("NAVIT_PREFIX"), 0);
	main_setup_environment(2);
#endif	/* _WIN32 || _WIN32_WCE */

	if (getenv("LC_ALL"))
		dbg(0,"Warning: LC_ALL is set, this might lead to problems (e.g. strange positions from GPS)\n");
	s = getenv("NAVIT_WID");
	if (s) {
		setenv("SDL_WINDOWID", s, 0);
	}
}

void
main_init_nls(void)
{
#ifdef ENABLE_NLS
#ifdef FORCE_LOCALE
#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)
	setlocale(LC_MESSAGES,STRINGIFY(FORCE_LOCALE));
#endif
	bindtextdomain(PACKAGE, getenv("NAVIT_LOCALEDIR"));
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain(PACKAGE);
#endif
}

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
#include <string.h>
#include <glib.h>
#include "config.h"
#include "debug.h"
#include "item.h"
#include "plugin.h"
#include "file.h"
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


static GList *
speech_cmdline_search(GList *l, int offset, int suffix_len, const char *s)
{
	GList *li=l,*ret=NULL,*tmp;
	int len=0;
	while (li) {
		char *snd=li->data;
		int snd_len;
		snd+=offset;
		snd_len=strlen(snd)-suffix_len;
		if (!strncasecmp(s, snd, snd_len)) {
			const char *ss=s+snd_len;
			while (*ss == ' ' || *ss == ',')
				ss++;
			dbg(1,"found %s remaining %s\n",snd,ss);
			if (*ss) 
				tmp=speech_cmdline_search(l, offset, suffix_len, ss);
			else 
				tmp=NULL;
			if (!ret || g_list_length(tmp) < len) {
				g_list_free(ret);
				ret=tmp;
				if (!*ss || tmp) 
					ret=g_list_prepend(ret, snd);
			} else
				g_list_free(tmp);
		}
		li=g_list_next(li);
	}
	return ret;
}
#if 0

	r=search(l, strlen(path)+1, suffix_len, argv[1]);
	while (r) {
		printf("%s/%s\n",path,r->data);
		r=g_list_next(r);
	}
	return 0;
#endif

struct speech_priv {
	char *cmdline;
	char *sample_dir;
	char *sample_suffix;
	GList *samples;
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
		char *pos=cmdline, end;
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
		g_free(cmdline);
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
	g_free(cmdline);
	g_free(cmd);
	g_free(arg);
	return ret;
#else
	char *cmdline;
	int ret;
	if (this->sample_dir && this->sample_suffix)  {
		GList *l=speech_cmdline_search(this->samples, strlen(this->sample_dir)+1, strlen(this->sample_suffix), text);
		char *sep="";
		cmdline=g_strdup("");
		while (l) {
			char *new_cmdline=g_strdup_printf("%s%s\"%s\"",cmdline,sep,(char *)l->data);
			g_free(cmdline);
			cmdline=new_cmdline;
			sep=" ";
			l=g_list_next(l);
		}
	} else
		cmdline=g_strdup_printf(this->cmdline, text);
	ret = system(cmdline);
	g_free(cmdline);
	return ret;
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
speechd_new(struct speech_methods *meth, struct attr **attrs, struct attr *parent) {
	struct speech_priv *this;
	struct attr *attr;
	attr=attr_search(attrs, NULL, attr_data);
	if (! attr)
		return NULL;
	this=g_new0(struct speech_priv,1);
	this->cmdline=g_strdup(attr->u.str);
	if ((attr=attr_search(attrs, NULL, attr_sample_dir)))
		this->sample_dir=g_strdup(attr->u.str);
	if ((attr=attr_search(attrs, NULL, attr_sample_suffix)))
		this->sample_suffix=g_strdup(attr->u.str);
	if (this->sample_dir && this->sample_suffix) {
		void *handle=file_opendir(this->sample_dir);
		char *name;
		int suffix_len=strlen(this->sample_suffix);
		while((name=file_readdir(handle))) {
			int len=strlen(name);
			if (len > suffix_len) {
				if (!strcmp(name+len-suffix_len, this->sample_suffix)) {
					this->samples=g_list_prepend(this->samples, g_strdup_printf("%s/%s",this->sample_dir,name));
				}
			}
		}
		file_closedir(handle);
	}
	*meth=speechd_meth;
	return this;
}


void
plugin_init(void)
{
	plugin_register_speech_type("cmdline", speechd_new);
}

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
#include "util.h"
#ifdef HAVE_API_WIN32_BASE
#include <windows.h>
#endif
#ifdef USE_EXEC
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#endif


static char *urldecode(char *str)
{
	char *ret=g_strdup(str);
	char *src=ret;
	char *dst=ret;
	while (*src) {
		if (*src == '%') {
			int val;
			if (sscanf(src+1,"%02x",&val)) {
				src+=2;
				*dst++=val;
			}
			src++;
		} else
			*dst++=*src++;
	}
	*dst++='\0';
	return ret;
}

static GList *
speech_cmdline_search(GList *l, int suffix_len, const char *s, int decode)
{
	GList *li=l,*ret=NULL,*tmp;
	int len=0;
	while (li) {
		char *snd=li->data;
		int snd_len;
		if (decode)
			snd=urldecode(snd);
		snd_len=strlen(snd)-suffix_len;
		if (!g_strncasecmp(s, snd, snd_len)) {
			const char *ss=s+snd_len;
			while (*ss == ' ' || *ss == ',')
				ss++;
			dbg(1,"found %s remaining %s\n",snd,ss);
			if (*ss) 
				tmp=speech_cmdline_search(l, suffix_len, ss, decode);
			else 
				tmp=NULL;
			if (!ret || (tmp && g_list_length(tmp) < len)) {
				len=g_list_length(tmp);
				g_list_free(ret);
				ret=tmp;
				if (!*ss || tmp) 
					ret=g_list_prepend(ret, li->data);
			} else
				g_list_free(tmp);
		}
		if (decode)
			g_free(snd);
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
	int flags;
	GList *samples;
	struct spawn_process_info *spi;
};

static int 
speechd_say(struct speech_priv *this, const char *text)
{
	char **cmdv=g_strsplit(this->cmdline," ", -1);
	int variable_arg_no=-1;
	GList *argl=NULL;
	guint listlen;
	int samplesmode=0;
	int i;
	
	for(i=0;cmdv[i];i++)
		if(strchr(cmdv[i],'%')) {
			variable_arg_no=i;
			break;
		}
	
	if (this->sample_dir && this->sample_suffix)  {
		argl=speech_cmdline_search(this->samples, strlen(this->sample_suffix), text, !!(this->flags & 1));
		samplesmode=1;
		listlen=g_list_length(argl);
	} else {
		listlen=1;
	}
	dbg(0,"text '%s'\n",text);
	if(listlen>0) {
		int argc;
		char**argv;
		int j;
		int cmdvlen=g_strv_length(cmdv);
		argc=cmdvlen + listlen - (variable_arg_no>0?1:0);
		argv=g_new(char *,argc+1);
		if(variable_arg_no==-1) {
			argv[cmdvlen]=g_strdup("%s");
			variable_arg_no=cmdvlen;
		}
		
		for(i=0,j=0;j<argc;) {
			if( i==variable_arg_no ) {
				if (samplesmode) {
					GList *l=argl;
					while(l) {
						char *new_arg;
						new_arg=g_strdup_printf("%s/%s",this->sample_dir,(char *)l->data);
						dbg(0,"new_arg %s\n",new_arg);
						argv[j++]=g_strdup_printf(cmdv[i],new_arg);
						g_free(new_arg);
						l=g_list_next(l);
					}
				} else {
					argv[j++]=g_strdup_printf(cmdv[i],text);
				}
				i++;
			} else {
				argv[j++]=g_strdup(cmdv[i++]);
			}
		}
		argv[j]=NULL;
		if (argl)
			// No need to free data elements here as they are
			// still referenced from this->samples 
			g_list_free(argl); 

		if(this->spi) {
			spawn_process_check_status(this->spi,1); // Block until previous spawned speech process is terminated.
			spawn_process_info_free(this->spi);
		}
		this->spi=spawn_process(argv);
		g_strfreev(argv);
	}
	g_strfreev(cmdv);
	return 0;
}


static void 
speechd_destroy(struct speech_priv *this) {
	GList *l=this->samples;
	g_free(this->cmdline);
	g_free(this->sample_dir);
	g_free(this->sample_suffix);
	while(l) {
		g_free(l->data);
	}
	g_list_free(this->samples);
	if(this->spi)
		spawn_process_info_free(this->spi);
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
	if ((attr=attr_search(attrs, NULL, attr_flags)))
		this->flags=attr->u.num;
	if (this->sample_dir && this->sample_suffix) {
		void *handle=file_opendir(this->sample_dir);
		char *name;
		int suffix_len=strlen(this->sample_suffix);
		while((name=file_readdir(handle))) {
			int len=strlen(name);
			if (len > suffix_len) {
				if (!strcmp(name+len-suffix_len, this->sample_suffix)) {
					dbg(0,"found %s\n",name);
					this->samples=g_list_prepend(this->samples, g_strdup(name));
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

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
#include <limits.h>
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


static char *urldecode(char *str) {
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

static GList *speech_cmdline_search(GList *samples, int suffix_len, const char *text, int decode) {
    GList *loop_samples=samples,*result=NULL,*recursion_result;
    int shortest_result_length=INT_MAX;
    dbg(lvl_debug,"searching samples for text: '%s'",text);
    while (loop_samples) {
        char *sample_name=loop_samples->data;
        int sample_name_len;
        if (decode)
            sample_name=urldecode(sample_name);
        sample_name_len=strlen(sample_name)-suffix_len;
        // TODO: Here we compare UTF-8 text with a filename.
        // It's unclear how a case-insensitive comparison should work
        // in general, so for now we only do it for ASCII text.
        if (!g_ascii_strncasecmp(text, sample_name, sample_name_len)) {
            const char *remaining_text=text+sample_name_len;
            while (*remaining_text == ' ' || *remaining_text == ',')
                remaining_text++;
            dbg(lvl_debug,"sample '%s' matched; remaining text: '%s'",sample_name,remaining_text);
            if (*remaining_text) {
                recursion_result=speech_cmdline_search(samples, suffix_len, remaining_text, decode);
                if (recursion_result && g_list_length(recursion_result) < shortest_result_length) {
                    g_list_free(result);
                    result=recursion_result;
                    result=g_list_prepend(result, loop_samples->data);
                    shortest_result_length=g_list_length(result);
                } else {
                    dbg(lvl_debug,"no (shorter) result found for remaining text '%s', "
                        "trying next sample\n", remaining_text);
                    g_list_free(recursion_result);
                }
            } else {
                g_list_free(result);
                result=g_list_prepend(NULL, loop_samples->data);
                break;
            }
        }
        if (decode)
            g_free(sample_name);
        loop_samples=g_list_next(loop_samples);
    }
    return result;
}

struct speech_priv {
    char *cmdline;
    char *sample_dir;
    char *sample_suffix;
    int flags;
    GList *samples;
    struct spawn_process_info *spi;
};

static int speechd_say(struct speech_priv *this, const char *text) {
    char **cmdv=g_strsplit(this->cmdline," ", -1);
    int variable_arg_no=-1;
    GList *argl=NULL;
    guint listlen;
    int samplesmode=0;
    int i;

    for(i=0; cmdv[i]; i++)
        if(strchr(cmdv[i],'%')) {
            variable_arg_no=i;
            break;
        }

    if (this->sample_dir && this->sample_suffix)  {
        argl=speech_cmdline_search(this->samples, strlen(this->sample_suffix), text, !!(this->flags & 1));
        samplesmode=1;
        listlen=g_list_length(argl);
        dbg(lvl_debug,"For text: '%s', found %d samples.",text,listlen);
        if (!listlen) {
            dbg(lvl_error,"No matching samples found. Cannot speak text: '%s'",text);
        }
    } else {
        listlen=1;
    }
    if(listlen>0) {
        dbg(lvl_debug,"Speaking text '%s'",text);
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

        for(i=0,j=0; j<argc;) {
            if( i==variable_arg_no ) {
                if (samplesmode) {
                    GList *l=argl;
                    while(l) {
                        char *new_arg;
                        new_arg=g_strdup_printf("%s/%s",this->sample_dir,(char *)l->data);
                        dbg(lvl_debug,"new_arg %s",new_arg);
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


static void speechd_destroy(struct speech_priv *this) {
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

static struct speech_priv *speechd_new(struct speech_methods *meth, struct attr **attrs, struct attr *parent) {
    struct speech_priv *this;
    struct attr *attr;
    attr=attr_search(attrs, attr_data);
    if (! attr)
        return NULL;
    this=g_new0(struct speech_priv,1);
    this->cmdline=g_strdup(attr->u.str);
    if ((attr=attr_search(attrs, attr_sample_dir)))
        this->sample_dir=g_strdup(attr->u.str);
    if ((attr=attr_search(attrs, attr_sample_suffix)))
        this->sample_suffix=g_strdup(attr->u.str);
    if ((attr=attr_search(attrs, attr_flags)))
        this->flags=attr->u.num;
    if (this->sample_dir && this->sample_suffix) {
        void *handle=file_opendir(this->sample_dir);
        if (!handle) {
            dbg(lvl_error,"Cannot read sample directory contents: %s", this->sample_dir);
            return NULL;
        }
        char *name;
        int suffix_len=strlen(this->sample_suffix);
        while((name=file_readdir(handle))) {
            int len=strlen(name);
            if (len > suffix_len) {
                if (!strcmp(name+len-suffix_len, this->sample_suffix)) {
                    dbg(lvl_debug,"found %s",name);
                    this->samples=g_list_prepend(this->samples, g_strdup(name));
                }
            }
        }
        file_closedir(handle);
    }
    *meth=speechd_meth;
    return this;
}


void plugin_init(void) {
    plugin_register_category_speech("cmdline", speechd_new);
}

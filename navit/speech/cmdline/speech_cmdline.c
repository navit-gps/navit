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

static gint get_longest_cmp(gconstpointer s1, gconstpointer s2) {
    gint diff=strlen((char *)s1)-strlen((char *)s2);
    return diff;
}

static GList *speech_cmdline_search(GList *samples, int sample_count, gchar *suffix, const char *text, int decode) {
    GList *loop_samples=samples,*loop_samples_sorted=NULL,*result=NULL;
    int sample_name_len;
    int suffix_len=strlen(suffix);
    int result_length_start;
    int result_length_end;
    int sample_index;
    gchar *sample_missing=g_strdup("missing");
    gchar *text_missing=g_strdup(sample_missing);
    gchar *text_tts;
    gchar *text_first[2];

    dbg(lvl_debug,"searching samples for text: '%s'",text);

    sample_missing=g_strconcat(sample_missing,suffix);

    loop_samples_sorted=loop_samples;
    loop_samples_sorted=g_list_first(loop_samples_sorted);
    loop_samples_sorted=g_list_sort(loop_samples_sorted, get_longest_cmp);
    loop_samples_sorted=g_list_first(loop_samples_sorted);
    loop_samples_sorted=g_list_reverse(loop_samples_sorted);

    result_length_start=0;
    result_length_end=0;

    while (*text) {

       if (result) {
          result_length_start=g_list_length(result);
          result_length_end=result_length_start;
       }

       sample_index=0;
       loop_samples_sorted=g_list_first(loop_samples_sorted);
       while (sample_index <= sample_count-1) {
          char *sample_name=loop_samples_sorted->data;

          if (decode)
              sample_name=urldecode(sample_name);
          sample_name_len=strlen(sample_name)-suffix_len;
          // TODO: Here we compare UTF-8 text with a filename.
          // It's unclear how a case-insensitive comparison should work
          // in general, so for now we only do it for ASCII text.
          if (!g_ascii_strncasecmp(text, sample_name, sample_name_len)) {
             result=g_list_prepend(result, loop_samples_sorted->data);
             text=text+sample_name_len;

             dbg(lvl_debug,"sample '%s' matched",sample_name);
             break;
          }
          if (decode)
             g_free(sample_name);

          if (sample_index < sample_count-1)
             loop_samples_sorted=g_list_next(loop_samples_sorted);
          else
             break;
          sample_index++;

       } // sample_loop

       if (result)
          result_length_end=g_list_length(result);

       if (result_length_start == result_length_end) {
          text_tts=g_strdup(text_missing);
          while (g_ascii_strncasecmp(text, " ", 1) && g_ascii_strncasecmp(text, ",", 1) && g_ascii_strncasecmp(text, "-", 1)) {
             g_strlcpy(text_first,text,2);
             if (strlen(text_first) == 0)
                break;
             text_tts=g_strconcat(text_tts, text_first);
             text++;
          }
          result=g_list_prepend(result, g_strdup(text_tts));
       }

       while (!g_ascii_strncasecmp(text, " ", 1) || !g_ascii_strncasecmp(text, ",", 1) || !g_ascii_strncasecmp(text, "-", 1)) {
          text++;
       }
    } // while text

    g_free(sample_missing);
    result=g_list_reverse(result);

    return result;
}

struct speech_priv {
    char *cmdline;
    char *cmdline_tts;
    char *sample_dir;
    char *sample_suffix;
    int flags;
    GList *samples;
    int sample_count;
    struct spawn_process_info *spi;
};

static int speechd_say(struct speech_priv *this, const char *text) {
    char **cmdv=g_strsplit(this->cmdline," ", -1);
    char **cmdv_tts=g_strsplit(this->cmdline_tts," ", -1);
    int variable_arg_no=-1;
    int variable_arg_no_tts=-1;
    GList *argl=NULL;
    GList *samples=NULL;
    guint listlen;
    int samplesmode=0;
    int i;
    int index;
    int speak_text;
    int sample_index;
    int speak_index_start;
    int speak_index_end;
    int sample_count;
    int missing;
    char *missing_text="missing";
    char *text_tts;


    for(i=0; cmdv[i]; i++)
        if(strchr(cmdv[i],'%')) {
            variable_arg_no=i;
            break;
        }

    for(i=0; cmdv_tts[i]; i++)
        if(strchr(cmdv_tts[i],'%')) {
            variable_arg_no_tts=i;
            break;
        }

    if (this->sample_dir && this->sample_suffix)  {
        samplesmode=1;
        argl=speech_cmdline_search(this->samples, this->sample_count, this->sample_suffix, text, !!(this->flags & 1));
        samples=g_list_copy(argl);
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
        int cmdvttslen=g_strv_length(cmdv_tts);


        speak_index_start=0;
        speak_index_end=0; 
        sample_index=0;
        missing=0;
        argl=g_list_first(argl);

        while(argl) {

            if(samplesmode) {
               if (!g_ascii_strncasecmp((char *)argl->data, missing_text, strlen(missing_text))) {
                  missing=1;
                  speak_text=1;
               }
               if(sample_index==listlen-1){
                  speak_text=1;
               }
            } else {
               speak_text=1;
               sample_count=listlen;
            }

            if(speak_text) {
               speak_text=0;
               if(samplesmode) {
                  if(missing) {
                     speak_index_end=sample_index-1;
                  } else {
                     speak_index_end=sample_index;
                  }
                  sample_count=speak_index_end-speak_index_start+1;
               }

               if(sample_count > 0) { 
                  argc=cmdvlen + sample_count - (variable_arg_no>0?1:0);
                  argv=g_new(char *,cmdvlen+sample_count+1);
                  if(variable_arg_no==-1) {
                     argv[cmdvlen]=g_strdup("%s");
                     variable_arg_no=cmdvlen;
                  }

                  for(i=0,j=0; j<argc;) {
                      if( i==variable_arg_no ) {
                          if (samplesmode) {
                              for(index=speak_index_start;index<=speak_index_end;index++){
                                  char *new_arg;
                                  new_arg=g_strdup_printf("%s/%s",this->sample_dir,(char *)g_list_nth_data(samples,index));
                                  dbg(lvl_debug,"new_arg %s",new_arg);
                                  argv[j++]=g_strdup_printf(cmdv[i],new_arg);
                                  g_free(new_arg);
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

                  if(this->spi) {
                      spawn_process_check_status(this->spi,1); // Block until previous spawned speech process is terminated.
                      spawn_process_info_free(this->spi);
                  }
                  this->spi=spawn_process(argv);
                  g_strfreev(argv);

               } // sample_count

               if(samplesmode && missing) {
                  missing=0;
                  speak_index_start=sample_index+1;

                  argv=g_new(char *,cmdvttslen+2);
                  for(i=0;i<=cmdvttslen;i++) {
                      argv[i]=g_strdup_printf("%s",cmdv_tts[i]);
                      if (i==cmdvttslen-1){
                          text_tts=g_strdup_printf("%s",(char *)g_list_nth_data(samples,sample_index));
                          text_tts=g_strdup(&text_tts[strlen(missing_text)]);
                          argv[i]=g_strdup_printf("%s",text_tts);
                          g_free(text_tts);
                      }
                   }
                   argv[i]=NULL;

                   if(this->spi) {
                      spawn_process_check_status(this->spi,1); // Block until previous spawned speech process is terminated.
                      spawn_process_info_free(this->spi);
                   }
                   this->spi=spawn_process(argv);

                   g_strfreev(argv);
               } // samplesmode && missing

            } // speak_text
            sample_index++;
            argl=g_list_next(argl);
        } // argl
    } // listlen

    if (argl)
        // No need to free data elements here as they are
        // still referenced from this->samples
        g_list_free(argl);
        g_list_free(samples);

    g_strfreev(cmdv);
    g_strfreev(cmdv_tts);

    return 0;
}


static void speechd_destroy(struct speech_priv *this) {
    GList *l=this->samples;
    g_free(this->cmdline);
    g_free(this->cmdline_tts);
    g_free(this->sample_dir);
    g_free(this->sample_suffix);
    while(l) {
        g_free(l->data);
    }
    g_list_free(this->samples);
    g_free(this->sample_count);
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
//-------------------------------------------------------
//  TODO: get attribute 'data_tts' in 'speech' tag from xml file
//  if ((attr=attr_search(attrs, attr_data_tts)))
//      this->cmdline_tts=g_strdup(attr->u.str);
    FILE *file_tts;
    char tts_command[50];
    file_tts=fopen("tts.txt","r");
    if(file_tts){
      if(fgets(tts_command, 50, file_tts) != NULL){
         puts(tts_command);
      }
      fclose(file_tts);
    }
    this->cmdline_tts=tts_command;
//-------------------------------------------------------
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
        this->sample_count=0;
        while((name=file_readdir(handle))) {
            int len=strlen(name);
            if (len > suffix_len) {
                if (!strcmp(name+len-suffix_len, this->sample_suffix)) {
                    dbg(lvl_debug,"found %s",name);
                    this->samples=g_list_prepend(this->samples, g_strdup(name));
                    this->sample_count++;
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

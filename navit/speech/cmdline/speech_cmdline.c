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

#include "config.h"
#include "debug.h"
#include "file.h"
#include "item.h"
#include "plugin.h"
#include "speech.h"
#include "util.h"
#include <glib.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#ifdef HAVE_API_WIN32_BASE
#    include <windows.h>
#endif
#ifdef USE_EXEC
#    include <string.h>
#    include <sys/types.h>
#    include <unistd.h>
#endif

struct speech_priv {
    char *name;
    char *cmdline;
    char *cmdline_tts;
    char *sample_dir;
    char *sample_suffix;
    int flags;
    GList *samples;
    int sample_count;
    struct spawn_process_info *spi;
};

static char *urldecode(char *str) {
    char *ret = g_strdup(str);
    char *src = ret;
    char *dst = ret;
    while (*src) {
        if (*src == '%') {
            int val;
            if (sscanf(src + 1, "%02x", &val)) {
                src += 2;
                *dst++ = val;
            }
            src++;
        } else
            *dst++ = *src++;
    }
    *dst++ = '\0';
    return ret;
}

static gint get_longest_cmp(gconstpointer s1, gconstpointer s2) {
    gint diff = strlen((char *)s1) - strlen((char *)s2);
    return diff;
}

static GList *speech_cmdline_search(GList *samples, int sample_count, gchar *suffix, const char *text, int decode) {
    GList *loop_samples = samples,*loop_samples_sorted = NULL,*result = NULL;
    int sample_name_len;
    int suffix_len = strlen(suffix);
    int result_length_start;
    int result_length_end;
    int sample_index;
    int sample_matched;
    gchar *sample_missing = g_strdup("missing");
    gchar *text_missing = g_strdup(sample_missing);
    gchar *text_tts;
    gchar text_first[2];
    gchar text_next[2];

    dbg(lvl_debug, "searching samples for text: '%s'", text);

    sample_missing = g_strconcat(sample_missing, suffix, NULL);

    loop_samples_sorted = loop_samples;
    loop_samples_sorted = g_list_first(loop_samples_sorted);
    loop_samples_sorted = g_list_sort(loop_samples_sorted, get_longest_cmp);
    loop_samples_sorted = g_list_first(loop_samples_sorted);
    loop_samples_sorted = g_list_reverse(loop_samples_sorted);

    result_length_start = 0;
    result_length_end = 0;

    while (*text) {

       if (result) {
          result_length_start = g_list_length(result);
          result_length_end = result_length_start;
       }

       sample_index = 0;
       loop_samples_sorted = g_list_first(loop_samples_sorted);
       while (sample_index <= sample_count-1) {
          char *sample_name = loop_samples_sorted->data;

          if (decode)
              sample_name = urldecode(sample_name);
          sample_name_len = strlen(sample_name) - suffix_len;
          // TODO: Here we compare UTF-8 text with a filename.
          // It's unclear how a case-insensitive comparison should work
          // in general, so for now we only do it for ASCII text.
          sample_matched = 0;
          if (!g_ascii_strncasecmp(text, sample_name, sample_name_len)) {
              if (sample_name_len == 1){
                  text++;
                  g_strlcpy(text_next, text, 2);
                  text--;
                  if (!g_ascii_strncasecmp(text_next, "1", 1) || !g_ascii_strncasecmp(text_next, "2", 1) ||
                      !g_ascii_strncasecmp(text_next, "3", 1) || !g_ascii_strncasecmp(text_next, "4", 1) ||
                      !g_ascii_strncasecmp(text_next, "5", 1) || !g_ascii_strncasecmp(text_next, "6", 1) ||
                      !g_ascii_strncasecmp(text_next, "7", 1) || !g_ascii_strncasecmp(text_next, "8", 1) ||
                      !g_ascii_strncasecmp(text_next, "9", 1) || !g_ascii_strncasecmp(text_next, "0", 1) ||
                      !g_ascii_strncasecmp(text_next, " ", 1) || !g_ascii_strncasecmp(text_next, ",", 1) ||
                      !g_ascii_strncasecmp(text_next, ".", 1) || !g_ascii_strncasecmp(text_next, ";", 1))
                      {
                         sample_matched = 1;
                  }
              } else {
                 sample_matched = 1;
              }

             if(sample_matched == 1){
               result = g_list_prepend(result, loop_samples_sorted->data);
               text = text + sample_name_len;

               dbg(lvl_debug, "sample '%s' matched", sample_name);
               break;
             }
          } // if strncasecmp
          if (decode)
             g_free(sample_name);

          if (sample_index < sample_count-1)
             loop_samples_sorted = g_list_next(loop_samples_sorted);
          else
             break;
          sample_index++;

       } // sample_loop

       if (result)
          result_length_end = g_list_length(result);

       if (result_length_start == result_length_end) {
          text_tts = g_strdup(text_missing);
          while (g_ascii_strncasecmp(text, " ", 1) && g_ascii_strncasecmp(text, ",", 1) &&
                 g_ascii_strncasecmp(text, "-", 1)) {
             g_strlcpy(text_first,text,2);
             if (strlen(text_first) == 0)
                break;
             text_tts = g_strconcat(text_tts, text_first, NULL);
             text++;
          }
          result = g_list_prepend(result, g_strdup(text_tts));
       }

       while (!g_ascii_strncasecmp(text, " ", 1) || !g_ascii_strncasecmp(text, ",", 1) ||
              !g_ascii_strncasecmp(text, "-", 1)) {
          text++;
       }
    } // while text

    g_free(sample_missing);
    result = g_list_reverse(result);

    return result;
}

static int speechd_say(struct speech_priv *this, const char *text) {
    char **cli;
    char **cli_tts;
    char **cli_command;
    char **cli_tts_command;
    int cli_argument_count;
    int cli_tts_argument_count;
    int cli_text_argument_position = -1;
    int cli_tts_text_argument_position = -1;
    GList *argl = NULL;
    GList *samples = NULL;
    guint listlen;
    int samplesmode = 0;
    int i;
    int index;
    int speak_text;
    int sample_index;
    int speak_index_start;
    int speak_index_end;
    int sample_count;
    int missing;
    char *missing_text = "missing";
    char *text_tts;
    char *new_arg;

    // Set sample and TTS commands
    cli = g_strsplit(this->cmdline," ", -1);
    cli_tts = g_strsplit(this->cmdline_tts," ", -1);
    cli_argument_count = g_strv_length(cli);
    cli_tts_argument_count = g_strv_length(cli_tts);
    dbg(lvl_debug, "cli_argument_count from navit.xml: %i", cli_argument_count);
    dbg(lvl_debug, "cli_tts_argument_count from navit.xml: %i", cli_tts_argument_count);

    // There has to be a NULL argument at the end
    dbg(lvl_debug, "cli_argument_count: %i", cli_argument_count);
    dbg(lvl_debug, "cli_tts_argument_count: %i", cli_tts_argument_count);
    cli_command = g_new(char *, cli_argument_count + 1);
    cli_tts_command = g_new(char *, cli_tts_argument_count + 1);
    cli_command[cli_argument_count] = NULL;
    cli_tts_command[cli_tts_argument_count] = NULL;
    dbg(lvl_debug, "cli_command length: %i", g_strv_length(cli_command));
    dbg(lvl_debug, "cli_tts_command length: %i", g_strv_length(cli_tts_command));

    for (i = 0; i <= cli_argument_count - 1; i++) {
         dbg(lvl_debug, "i: %i", i);
         cli_command[i] = g_strdup(cli[i]);
         dbg(lvl_debug, "cli_command[i]: %s", g_strdup(cli_command[i]));
         if (!g_ascii_strncasecmp(cli_command[i], "%s", 2))
             cli_text_argument_position = i;
    }
    dbg(lvl_debug, "cli_text_argument_position: %i", cli_text_argument_position);

    for (i = 0; i <= cli_tts_argument_count - 1; i++) {
         dbg(lvl_debug, "i: %i", i);
         cli_tts_command[i] = g_strdup(cli_tts[i]);
         dbg(lvl_debug, "cli_tts_command[i]: %s", g_strdup(cli_tts_command[i]));
         if (!g_ascii_strncasecmp(cli_tts_command[i], "%s", 2))
             cli_tts_text_argument_position = i;
    }
    dbg(lvl_debug, "cli_tts_text_argument_position: %i", cli_tts_text_argument_position);
    // Set sample and TTS commands end


    // Speech command must have an '%s' argument
    if (cli_text_argument_position >= 1) {

        // Play text //

        // Samples and TTS has to be configured if samples are used
        // (There are always texts without samples to use (i.e. town names))
        if (this->sample_dir && this->sample_suffix && cli_tts_text_argument_position >= 1)  {
            samplesmode = 1;
            argl = speech_cmdline_search(this->samples, this->sample_count, this->sample_suffix, text,
                                     !!(this->flags & 1));
            samples = g_list_copy(argl);
            listlen = g_list_length(argl);
            dbg(lvl_debug, "For text: '%s', found %d samples.", text, listlen);

            if (!listlen) {
                dbg(lvl_error, "No matching samples found. Cannot speak text: '%s'", text);
            }
        } else {
            listlen = 1;
        }

        if (listlen > 0) {
             dbg(lvl_info, "Speaking text: '%s'", text);

             speak_index_start = 0;
             speak_index_end = 0; 
             sample_index = 0;
             missing = 0;
             argl = g_list_first(argl);

             while (argl) {

                 if (samplesmode) {
                     if (!g_ascii_strncasecmp((char *)argl->data, missing_text, strlen(missing_text))) {
                          missing = 1;
                          speak_text = 1;
                       }
                       if (sample_index == listlen - 1) {
                           speak_text = 1;
                       }
                 } else {
                     speak_text = 1;
                     sample_count = listlen;
                 }

                  if (speak_text) {
                      speak_text = 0;
                     if (samplesmode) {
                         if (missing) {
                             speak_index_end = sample_index - 1;
                         } else {
                             speak_index_end = sample_index;
                         }
                         sample_count = speak_index_end-speak_index_start + 1;
                     }

                     if (sample_count > 0) { 

                         for (index = speak_index_start; index <= speak_index_end; index++) {
                              dbg(lvl_debug, "samplesmode: %i", samplesmode);
                              if (samplesmode) {
                                  new_arg = g_strdup_printf("%s/%s", this->sample_dir,
                                                            (char *)g_list_nth_data(samples,index));
                                  dbg(lvl_info, "Speaking (sample): %s", new_arg);
                                  cli_command[cli_text_argument_position] = g_strdup(new_arg);
                                  g_free(new_arg);

                                  // Execute cli speech command for one sample file
                                  dbg(lvl_debug, "executing (sample): %s", g_strdup(cli_command[0]));
                                  dbg(lvl_debug, "sample: %s",
                                       g_strdup(cli_command[cli_text_argument_position]));
                                  if (this->spi) {
                                      // Block until previous spawned speech process is terminated.
                                      spawn_process_check_status(this->spi, 1);
                                      spawn_process_info_free(this->spi);
                                  }
                                  this->spi = spawn_process(cli_command);
                                  cli_command[cli_text_argument_position] = g_strdup("%s");

                              } else {
                                  new_arg = g_strdup_printf("\"%s\"", text);
                                  dbg(lvl_info, "Speaking (TTS): %s", new_arg);
                                  cli_tts_command[cli_tts_text_argument_position] = g_strdup(new_arg);
                                  g_free(new_arg);

                                  // Execute cli speech command for TTS text
                                  dbg(lvl_debug, "executing (TTS): %s", g_strdup(cli_tts_command[0]));
                                  dbg(lvl_debug, "text: %s",
                                       g_strdup(cli_tts_command[cli_tts_text_argument_position]));
                                  if (this->spi) {
                                      // Block until previous spawned speech process is terminated.
                                      spawn_process_check_status(this->spi, 1);
                                      spawn_process_info_free(this->spi);
                                  }
                                  this->spi = spawn_process(cli_tts_command);
                                  cli_tts_command[cli_tts_text_argument_position] = g_strdup("%s");

                             } // samplesmode
                         } // for sample index
                         g_free(new_arg);
                         g_strfreev(cli_command);
                         g_strfreev(cli_tts_command);
                     } // if sample_count > 0

                     if (samplesmode && missing) {
                         missing = 0;
                         speak_index_start = sample_index + 1;

                         text_tts = g_strdup_printf("%s", (char *)g_list_nth_data(samples, sample_index));
                         text_tts = g_strdup(&text_tts[strlen(missing_text)]);
                         cli_tts_command[cli_tts_text_argument_position] = g_strdup_printf("\"%s\"", text_tts);
                         dbg(lvl_info, "Speaking (TTS): %s", g_strdup(text_tts));
                         g_free(text_tts);

                         // Execute cli speech command for TTS text
                         dbg(lvl_debug, "executing (TTS): %s", g_strdup(cli_tts_command[0]));
                         dbg(lvl_debug, "text: %s", g_strdup(cli_tts_command[cli_tts_text_argument_position]));
                         if (this->spi) {
                             // Block until previous spawned speech process is terminated.
                             spawn_process_check_status(this->spi, 1);
                             spawn_process_info_free(this->spi);
                         }
                         this->spi = spawn_process(cli_tts_command);
                         cli_tts_command[cli_tts_text_argument_position] = g_strdup("%s");

                     } // samplesmode && missing
                  } // speak_text
                  sample_index++;
                  argl = g_list_next(argl);
              } // while argl
          } // if listlen
    } else {
        dbg(lvl_error, "Speech tag 'data' attribute is missing an '%s' argument in navit.xml.");
    } // if cli_text_argument_position

    if (argl) {
        // No need to free data elements here as they are
        // still referenced from this->samples
        g_list_free(argl);
        g_list_free(samples);
    }

    g_strfreev(cli);
    g_strfreev(cli_tts);

    return 0;
}

static void speechd_destroy(struct speech_priv *this) {
    GList *l = this->samples;
    g_free(this->name);
    g_free(this->cmdline);
    g_free(this->cmdline_tts);
    g_free(this->sample_dir);
    g_free(this->sample_suffix);
    while (l) {
        g_free(l->data);
    }
    g_list_free(this->samples);
    if (this->spi)
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
    attr = attr_search(attrs, attr_data);
    if (! attr)
        return NULL;
    this = g_new0(struct speech_priv, 1);
    this->cmdline = g_strdup(attr->u.str);
    attr = attr_search(attrs, attr_name);
    if (! attr)
        return NULL;
    this->name = g_strdup(attr->u.str);
    attr = attr_search(attrs, attr_data_tts);
    this->cmdline_tts = g_strdup(attr->u.str);
    if ((attr = attr_search(attrs, attr_sample_dir)))
        this->sample_dir = g_strdup(attr->u.str);
    if ((attr = attr_search(attrs, attr_sample_suffix)))
        this->sample_suffix = g_strdup(attr->u.str);
    if ((attr = attr_search(attrs, attr_flags)))
        this->flags = attr->u.num;
    if (this->sample_dir && this->sample_suffix) {
        void *handle = file_opendir(this->sample_dir);
        if (!handle) {
            dbg(lvl_error, "Cannot read sample directory contents: %s", this->sample_dir);
            return NULL;
        }
        char *name;
        int suffix_len = strlen(this->sample_suffix);
        this->sample_count = 0;
        while ((name = file_readdir(handle))) {
            int len = strlen(name);
            if (len > suffix_len) {
                if (!strcmp(name + len - suffix_len, this->sample_suffix)) {
                    dbg(lvl_debug, "found %s", name);
                    this->samples = g_list_prepend(this->samples, g_strdup(name));
                    this->sample_count++;
                }
            }
        }
        file_closedir(handle);
    }
    *meth = speechd_meth;
    return this;
}

void plugin_init(void) {
    plugin_register_category_speech("cmdline", speechd_new);
}

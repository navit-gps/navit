/**
 * Navit, a modular navigation system.
 * Copyright (C) 2017-2017 Navit Team
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
// style with: clang-format -style=WebKit -i *
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <glib.h>
extern "C" {
#include "item.h"
#include "attr.h"
#include "debug.h"
#include "file.h"
#include "plugin.h"
#include "speech.h"
#include "util.h"
}
#include <espeak/speak_lib.h>

#include "Qt5EspeakAudioOut.h"

#ifndef INTERNAL_ESPEAK
#define INTERNAL_ESPEAK 0
#endif
#define BUFFERLENGTH 1000
#define SAMPLE_SIZE 16

// --------------------------------------------------------------------

/* private context handle type */
struct speech_priv {
    gchar* path_home;
    int sample_rate;

    bool espeak_ok;
    bool audio_ok;
    Qt5EspeakAudioOut* audio;
};

/* callback from espeak to transfer generated samples */
int qt5_espeak_SynthCallback(short* wav, int numsamples, espeak_EVENT* events) {
    struct speech_priv* pr = NULL;
    dbg(lvl_debug, "Callback %d samples", numsamples);
    if (events != NULL)
        pr = (struct speech_priv*)events->user_data;
    if ((pr != NULL) && (pr->audio != NULL)) {
        pr->audio->addSamples(wav, numsamples);
    }

    return 0;
    /* 0 - continue synthesis */
    /* 1 - abort synthesis */
}

/* set up espeak */
static bool qt5_espeak_init_espeak(struct speech_priv* sr, struct attr** attrs) {
    struct attr* path;

    /* prepare espeak library path home */
    path = attr_search(attrs, attr_path);
    if (path) {
        sr->path_home = g_strdup(path->u.str);
    } else {
#if INTERNAL_ESPEAK
        sr->path_home = g_strdup_printf("%s", getenv("NAVIT_SHAREDIR"));
#else
        /* since no path was given by config, we don't know the path to external
        * espeak data.
        * so give NULL to use default path */
        sr->path_home = NULL;
#endif
    }
    dbg(lvl_debug, "path_home set to %s", sr->path_home);
#if INTERNAL_ESPEAK
    /*internal espeak is configured to support only synchronous modes */
    sr->sample_rate = espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, BUFFERLENGTH, sr->path_home, 0);
#else
    // sr->sample_rate = espeak_Initialize(AUDIO_OUTPUT_RETRIEVAL, BUFFERLENGTH,
    // sr->path_home, 0);
    sr->sample_rate = espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, BUFFERLENGTH, sr->path_home, 0);
#endif
    if (sr->sample_rate == EE_INTERNAL_ERROR) {
        dbg(lvl_error, "Init failed %d", sr->sample_rate);
        return 1;
    }
    dbg(lvl_error, "Sample rate is %d", sr->sample_rate);

    espeak_SetSynthCallback(qt5_espeak_SynthCallback);
    return TRUE;
}

/* set language to espeak */
static bool qt5_espeak_init_language(struct speech_priv* pr, struct attr** attrs) {
    struct attr* language;
    gchar* lang_str = NULL;
    espeak_ERROR error;
    espeak_VOICE voice_spec;

    language = attr_search(attrs, attr_language);
    if (language) {
        lang_str = g_strdup(language->u.str);
    } else {
        char* lang_env = getenv("LANG");
        if (lang_env != NULL) {
            char* country;

            lang_str = g_strdup(lang_env);
            /* convert to all lower */
            strtolower(lang_str, lang_env);

            /* extract language code from LANG environment */
            dbg(lvl_debug, "%s", lang_str);
            country = strchr(lang_str, '_');
            dbg(lvl_debug, "%s", country);
            if (country) {
                lang_str[country - lang_str] = '\0';
            }
            dbg(lvl_debug, "espeak lang: %s", lang_str);
        }
    }
    /*TODO (golden water tap): expose all those values as attrs */
    voice_spec.name = NULL; /* here we could select a specific voice */
    voice_spec.languages = lang_str;
    voice_spec.gender = 0; /* 0 auto, 1 male, 2 female */
    voice_spec.age = 20;
    voice_spec.variant = 0;

    error = espeak_SetVoiceByProperties(&voice_spec);
    if (lang_str != NULL)
        g_free(lang_str);
    if (error != EE_OK) {
        dbg(lvl_error, "Unable to set Language");
        return false;
    }
    return true;
}

/* init audio system */
static bool qt5_espeak_init_audio(struct speech_priv* sr, const char* category) {
    try {
        sr->audio = new Qt5EspeakAudioOut(sr->sample_rate, category);
    } catch (void* exception) {
        dbg(lvl_error, "Exception on Qt5EspeakAudioOut creation");
        return false;
    }
    return true;
}

/* will be called to say new text.
 * sr - private handle
 * text - new (utf8) text
 */
static int qt5_espeak_say(struct speech_priv* sr, const char* text) {
    espeak_ERROR error;
    dbg(lvl_debug, "Say \"%s\"", text);
    error = espeak_Synth(text, strlen(text), 0, POS_CHARACTER, 0,
                         espeakCHARS_UTF8, NULL, sr);
    if (error != EE_OK)
        dbg(lvl_error, "Unable to speak! error == %d", error);

    return 0;
}

/* destructor */
static void qt5_espeak_destroy(struct speech_priv* sr) {
    dbg(lvl_debug, "Enter");

    if (sr->path_home != NULL)
        g_free(sr->path_home);
    /* destroy handle */
    g_free(sr);
}

/* the plugin interface for speech */
static struct speech_methods qt5_espeak_meth = {
    qt5_espeak_destroy, qt5_espeak_say,
};

/* create new instance of this.
 * meth - return plugin interface
 * attr - get decoded attributes from config
 * parent - get parent attributes from config
 */
static struct speech_priv* qt5_espeak_new(struct speech_methods* meth, struct attr** attrs,
        struct attr* parent) {
    struct speech_priv* sr = NULL;
    dbg(lvl_debug, "Enter");

    /* allocate handle */
    sr = g_new0(struct speech_priv, 1);
    sr->path_home = NULL;
    sr->espeak_ok = false;
    sr->audio_ok = false;
    sr->sample_rate = 0;
    sr->audio = NULL;
    /* register our methods */
    *meth = qt5_espeak_meth;

    /* init espeak library */
    if (!(sr->espeak_ok = qt5_espeak_init_espeak(sr, attrs))) {
        dbg(lvl_error, "Unable to initialize espeak library");
    }

    /* init espeak voice and language */
    if (!(sr->espeak_ok = qt5_espeak_init_language(sr, attrs))) {
        dbg(lvl_error, "Unable to initialize espeak language");
    }

    /* init qt5 audio using default category*/
    if (!(sr->audio_ok = qt5_espeak_init_audio(sr, NULL))) {
        dbg(lvl_error, "Unable to initialize audio");
    }
    return sr;
}

/* initialize this as plugin */
void plugin_init(void) {
    dbg(lvl_debug, "Enter");
    plugin_register_category_speech("qt5_espeak", qt5_espeak_new);
}

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
#include <glib.h>
#include "config.h"
#include "item.h"
#include "debug.h"
#include "plugin.h"
#include "android.h"
#include "speech.h"

struct speech_priv {
    jclass NavitSpeechClass;
    jobject NavitSpeech;
    jmethodID NavitSpeech_say;
    int flags;
};

static int speech_android_say(struct speech_priv *this, const char *text) {
    char *str=g_strdup(text);
    jstring string;
    char *tok = str;

    /* Replace hyphens with white spaces, or some Android speech SDK will pronounce "hyphen" */
    while (*tok) {
        if (*tok=='-')
            *tok=' ';
        tok++;
    }

    string = (*jnienv)->NewStringUTF(jnienv, str);
    dbg(lvl_debug,"enter %s",str);
    (*jnienv)->CallVoidMethod(jnienv, this->NavitSpeech, this->NavitSpeech_say, string);
    (*jnienv)->DeleteLocalRef(jnienv, string);
    g_free(str);

    return 1;
}

static void speech_android_destroy(struct speech_priv *this) {
    g_free(this);
}

static struct speech_methods speech_android_meth = {
    speech_android_destroy,
    speech_android_say,
};

static int speech_android_init(struct speech_priv *ret) {
    jmethodID cid;
    char *class="org/navitproject/navit/NavitSpeech2";

    if (!android_find_class_global(class, &ret->NavitSpeechClass)) {
        dbg(lvl_error,"No class found");
        return 0;
    }
    dbg(lvl_debug,"at 3");
    cid = (*jnienv)->GetMethodID(jnienv, ret->NavitSpeechClass, "<init>", "(Lorg/navitproject/navit/Navit;)V");
    if (cid == NULL) {
        dbg(lvl_error,"no method found");
        return 0; /* exception thrown */
    }
    if (!android_find_method(ret->NavitSpeechClass, "say", "(Ljava/lang/String;)V", &ret->NavitSpeech_say))
        return 0;
    dbg(lvl_debug,"at 4 android_activity=%p",android_activity);
    ret->NavitSpeech=(*jnienv)->NewObject(jnienv, ret->NavitSpeechClass, cid, android_activity);
    dbg(lvl_debug,"result=%p",ret->NavitSpeech);
    if (!ret->NavitSpeech)
        return 0;
    if (ret->NavitSpeech)
        ret->NavitSpeech = (*jnienv)->NewGlobalRef(jnienv, ret->NavitSpeech);
    return 1;
}

static struct speech_priv *speech_android_new(struct speech_methods *meth, struct attr **attrs, struct attr *parent) {
    struct speech_priv *this;
    struct attr *flags;
    *meth=speech_android_meth;
    this=g_new0(struct speech_priv,1);
    if (!speech_android_init(this)) {
        dbg(lvl_error,"Failed to init speech %p",this->NavitSpeechClass);
        g_free(this);
        this=NULL;
    }
    if ((flags = attr_search(attrs, NULL, attr_flags)))
        this->flags=flags->u.num;

    return this;
}


void plugin_init(void) {
    plugin_register_category_speech("android", speech_android_new);
}

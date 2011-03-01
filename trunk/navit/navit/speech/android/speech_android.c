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

static int 
speech_android_say(struct speech_priv *this, const char *text)
{
	char *str=g_strdup(text);
	jstring string;
	int i;

	if (this->flags & 2) {
		for (i = 0 ; i < strlen(str) ; i++) {
			if (str[i] == 0xc3 && str[i+1] == 0x84) {
				str[i]='A';
				str[i+1]='e';
			}
			if (str[i] == 0xc3 && str[i+1] == 0x96) {
				str[i]='O';
				str[i+1]='e';
			}
			if (str[i] == 0xc3 && str[i+1] == 0x9c) {
				str[i]='U';
				str[i+1]='e';
			}
			if (str[i] == 0xc3 && str[i+1] == 0xa4) {
				str[i]='a';
				str[i+1]='e';
			}
			if (str[i] == 0xc3 && str[i+1] == 0xb6) {
				str[i]='o';
				str[i+1]='e';
			}
			if (str[i] == 0xc3 && str[i+1] == 0xbc) {
				str[i]='u';
				str[i+1]='e';
			}
			if (str[i] == 0xc3 && str[i+1] == 0x9f) {
				str[i]='s';
				str[i+1]='s';
			}
		}
	}
	string = (*jnienv)->NewStringUTF(jnienv, str);
	dbg(0,"enter %s\n",str);
        (*jnienv)->CallVoidMethod(jnienv, this->NavitSpeech, this->NavitSpeech_say, string);
        (*jnienv)->DeleteLocalRef(jnienv, string);
	g_free(str);

	return 1;
}

static void 
speech_android_destroy(struct speech_priv *this) {
	g_free(this);
}

static struct speech_methods speech_android_meth = {
	speech_android_destroy,
	speech_android_say,
};

static int
speech_android_init(struct speech_priv *ret)
{
	jmethodID cid;
	char *class="org/navitproject/navit/NavitSpeech2";

	if (ret->flags & 1) 
		class="org/navitproject/navit/NavitSpeech";

	if (!android_find_class_global(class, &ret->NavitSpeechClass)) {
		dbg(0,"No class found\n");
                return 0;
	}
        dbg(0,"at 3\n");
        cid = (*jnienv)->GetMethodID(jnienv, ret->NavitSpeechClass, "<init>", "(Lorg/navitproject/navit/Navit;)V");
        if (cid == NULL) {
                dbg(0,"no method found\n");
                return 0; /* exception thrown */
        }
	if (!android_find_method(ret->NavitSpeechClass, "say", "(Ljava/lang/String;)V", &ret->NavitSpeech_say))
                return 0;
        dbg(0,"at 4 android_activity=%p\n",android_activity);
        ret->NavitSpeech=(*jnienv)->NewObject(jnienv, ret->NavitSpeechClass, cid, android_activity);
        dbg(0,"result=%p\n",ret->NavitSpeech);
	if (!ret->NavitSpeech)
		return 0;
        if (ret->NavitSpeech)
                (*jnienv)->NewGlobalRef(jnienv, ret->NavitSpeech);
	return 1;
}

static struct speech_priv *
speech_android_new(struct speech_methods *meth, struct attr **attrs, struct attr *parent) {
	struct speech_priv *this;
	struct attr *flags;
	*meth=speech_android_meth;
	this=g_new0(struct speech_priv,1);
	if (!speech_android_init(this)) {
		g_free(this);
		this=NULL;
	}
	if (android_version < 4)
		this->flags=3;
	if ((flags = attr_search(attrs, NULL, attr_flags)))
		this->flags=flags->u.num;
	
	return this;
}


void
plugin_init(void)
{
	plugin_register_speech_type("android", speech_android_new);
}

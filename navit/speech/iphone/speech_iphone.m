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
#include "speech.h"
#include "attr.h"
#import "VSSpeechSynthesizer.h"
#import <UIKit/UIKit.h>

struct speech_priv {
    VSSpeechSynthesizer *speech;
};

static int speech_iphone_say(struct speech_priv *this, const char *text) {
    dbg(lvl_debug,"enter %s",text);
    NSString *s=[[NSString alloc]initWithUTF8String: text];
    [this->speech startSpeakingString:s];
    [s release];
    dbg(lvl_debug,"ok");
    return 1;
}

static void speech_iphone_destroy(struct speech_priv *this) {
    [this->speech release];
    g_free(this);
}

static struct speech_methods speech_iphone_meth = {
    speech_iphone_destroy,
    speech_iphone_say,
};

static struct speech_priv *speech_iphone_new(struct speech_methods *meth, struct attr **attrs, struct attr *parent) {
    struct speech_priv *this;
    *meth=speech_iphone_meth;
    this=g_new0(struct speech_priv,1);
    this->speech= [VSSpeechSynthesizer alloc];
    [this->speech init];
    dbg(lvl_debug,"this->speech=%p",this->speech);

    if([[[UIDevice currentDevice] systemVersion] floatValue] >=10) {
        [this->speech setRate:0.5];
        NSLog(@"iOS is >= 10: %f", [[[UIDevice currentDevice] systemVersion] floatValue]);
    } else {
        [this->speech setRate:0.15];
        NSLog(@"iOS is < 10, %f", [[[UIDevice currentDevice] systemVersion] floatValue]);
    }

    [this->speech setVolume:100.0];
    [this->speech setPitch:0.8];

    struct attr *attr;

    // With the attribute speech_use_hfp="1" user can force to use HFP always.
    // This will force background music to HFP as well.
    // If not set we set HFP, but will automatically switch to A2DP when background music is being played
    // A2DP will not stop your radio from playing when an announcement is spoken
    if ((attr=attr_search(attrs, attr_speech_use_hfp)))
        [this->speech useHFP:(int)attr->u.num force:YES];
    else
        [this->speech useHFP:YES force:NO]; // AUTO

    return this;
}

void plugin_init(void) {
    dbg(lvl_debug,"enter");
    plugin_register_category_speech("iphone", speech_iphone_new);
}

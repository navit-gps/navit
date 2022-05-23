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
#import "VSSpeechSynthesizer.h"

struct speech_priv {
	VSSpeechSynthesizer *speech;
};

static int
speech_iphone_say(struct speech_priv *this, const char *text)
{
	dbg(0,"enter %s",text);
	NSString *s=[[NSString alloc]initWithUTF8String: text];
	[this->speech startSpeakingString:s toURL:nil];
	[s release];
	dbg(0,"ok");
	return 1;
}

static void
speech_iphone_destroy(struct speech_priv *this)
{
	[this->speech release];
	g_free(this);
}

static struct speech_methods speech_iphone_meth = {
	speech_iphone_destroy,
	speech_iphone_say,
};

static struct speech_priv *
speech_iphone_new(struct speech_methods *meth, struct attr **attrs, struct attr *parent)
{
	struct speech_priv *this;
	*meth=speech_iphone_meth;
	this=g_new0(struct speech_priv,1);
	this->speech=[[NSClassFromString(@"VSSpeechSynthesizer") alloc] init];
	dbg(0,"this->speech=%p",this->speech);
	[this->speech setRate:(float)1.0];
	return this;
}


void
plugin_init(void)
{
	dbg(0,"enter");
	plugin_register_category_speech("iphone", speech_iphone_new);
}

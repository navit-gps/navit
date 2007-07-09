#include <glib.h>
#include "debug.h"
#include "speech.h"
#include "plugin.h"

struct speech {
	struct speech_priv *priv;
	struct speech_methods meth;
};

struct speech *
speech_new(const char *type, const char *data) 
{
	struct speech *this_;
	struct speech_priv *(*speech_new)(const char *data, struct speech_methods *meth);

	dbg("enter type=%s data=%s\n", type, data);
        speech_new=plugin_get_speech_type(type);
	dbg(1,"new=%p\n", speech_new);
        if (! speech_new) {
                return NULL;
	}
	this_=g_new0(struct speech, 1);
	this_->priv=speech_new(data, &this_->meth);
	dbg(1, "say=%p\n", this_->meth.say);
	dbg(1,"priv=%p\n", this_->priv);
	if (! this_->priv) {
		g_free(this_);
		return NULL;
	}
	dbg(1,"return %p\n", this_);
	return this_;
}

int
speech_say(struct speech *this_, const char *text)
{
	dbg(1, "this_=%p text='%s' calling %p\n", this_, text, this_->meth.say);
	return (this_->meth.say)(this_->priv, text);
}

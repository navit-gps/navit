#include <stdlib.h>
#include <glib.h>
#include "plugin.h"
#include "speech.h"

struct speech_priv {
	char *cmdline;
};

static int 
speechd_say(struct speech_priv *this, const char *text)
{
	char *cmdline;

	cmdline=g_strdup_printf(this->cmdline, text);
	return system(cmdline);
}

static void 
speechd_destroy(struct speech_priv *this) {
	g_free(this->cmdline);
	g_free(this);
}

static struct speech_methods speechd_meth = {
	speechd_destroy,
	speechd_say,
};

static struct speech_priv *
speechd_new(char *data, struct speech_methods *meth) {
	struct speech_priv *this;
	if (! data)
		return NULL;
	this=g_new(struct speech_priv,1);
	if (this) {
		this->cmdline=g_strdup(data);
		*meth=speechd_meth;
	}
	return this;
}


void
plugin_init(void)
{
	plugin_register_speech_type("cmdline", speechd_new);
}

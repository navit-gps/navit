#ifndef NAVIT_SPEECH_H
#define NAVIT_SPEECH_H

struct speech_priv;

struct speech_methods {
	void (*destroy)(struct speech_priv *this_);
	int (*say)(struct speech_priv *this_, const char *text);
};

/* prototypes */
struct speech * speech_new(const char *type, const char *data);
int speech_say(struct speech *this_, const char *text);
int speech_sayf(struct speech *this_, const char *format, ...);
void speech_destroy(struct speech *this_);
/* end of prototypes */

#endif


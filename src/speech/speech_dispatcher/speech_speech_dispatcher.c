
/* speechd simple client program
 * CVS revision: $Id: speech_speech_dispatcher.c,v 1.2 2007-07-08 16:42:16 martin-s Exp $
 * Author: Tomas Cerha <cerha@brailcom.cz> */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <stdarg.h>
#include "config.h"
#include <libspeechd.h>
#include "plugin.h"
#include "speech.h"

struct speech {
	SPDConnection *conn;
};

struct speech *
speech_new(void) {
	struct speech *this;
	SPDConnection *conn;

	conn = spd_open("navit","main",NULL,SPD_MODE_SINGLE);
	if (! conn) 
		return NULL;
	this=g_new(struct speech,1);
	if (this) {
		this->conn=conn;
		spd_set_punctuation(conn, SPD_PUNCT_NONE);
	}
	return this;
}

static int 
speech_say(struct speech *this, char *text) {
	int err;

	err = spd_sayf(this->conn, SPD_MESSAGE, text);
	if (err != 1)
		return 1;
	return 0;
}

static int
speech_sayf(struct speech *this, char *format, ...) {
	char buffer[8192];
	va_list ap;
	va_start(ap,format);
	vsnprintf(buffer, 8192, format, ap);
	return speech_say(this, buffer);
}

static void 
speech_destroy(struct speech *this) {
	spd_close(this->conn);
	g_free(this);
}
void
plugin_init(void)
{
}

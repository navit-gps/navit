/* speechd simple client program
 * CVS revision: $Id: speech.c,v 1.4 2006-01-03 21:37:48 martin-s Exp $
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
#ifdef HAVE_LIBSPEECHD
#include <libspeechd.h>
#endif
#include "speech.h"

struct speech {
#ifdef HAVE_LIBSPEECHD
	SPDConnection *conn;
#endif
};

struct speech *
speech_new(void) {
#ifdef HAVE_LIBSPEECHD
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
#else
	return NULL;
#endif
}

int 
speech_say(struct speech *this, char *text) {
#ifdef HAVE_LIBSPEECHD
	int err;

	err = spd_sayf(this->conn, SPD_MESSAGE, text);
	if (err != 1)
		return 1;
#endif
	return 0;
}

int
speech_sayf(struct speech *this, char *format, ...) {
#ifdef HAVE_LIBSPEECHD
	char buffer[8192];
	va_list ap;
	va_start(ap,format);
	vsnprintf(buffer, 8192, format, ap);
	return speech_say(this, buffer);
#else
	return 0;
#endif
}

int 
speech_destroy(struct speech *this) {
#ifdef HAVE_LIBSPEECHD
	spd_close(this->conn);
	g_free(this);
#endif
	return 0;
}

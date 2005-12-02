/* speechd simple client program
 * CVS revision: $Id: speech.c,v 1.1 2005-12-02 10:41:56 martin-s Exp $
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

#include <libspeechd.h>
#include "speech.h"

struct speech {
	int sockfd;
};

struct speech *
speech_new(void) {
	struct speech *this;
	int sockfd;

	sockfd = spd_init("map","main");
	if (sockfd == 0) 
		return NULL;
	this=g_new(struct speech,1);
	if (this) {
		this->sockfd=sockfd;
	}
	return this;
}

int 
speech_say(struct speech *this, char *text) {
	int err;

	err = spd_sayf(this->sockfd, 2, text);
	if (err != 1)
		return 1;
	return 0;
}

int
speech_sayf(struct speech *this, char *format, ...) {
	char buffer[8192];
	va_list ap;
	va_start(ap,format);
	vsnprintf(buffer, 8192, format, ap);
	return speech_say(this, buffer);
}

int 
speech_destroy(struct speech *this) {
	spd_close(this->sockfd);
	g_free(this);
	return 0;
}

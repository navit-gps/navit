#include <sys/types.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "wordexp.h"

int wordexp(const char * words, wordexp_t * we, int flags)
{
	int error=0;

	assert(we != NULL);
	assert(words != NULL);

    we->we_wordc = 1;
	we->we_wordv = NULL;
	we->we_strings = NULL;
	we->we_nbytes = 0;

    we->we_wordv = malloc( we->we_wordc * sizeof( char* ) );

    we->we_nbytes = strlen( words ) + 1;
    we->we_strings = malloc( we->we_nbytes );

    we->we_wordv[0] = &we->we_strings[0];

    // copy string & terminate
    memcpy( we->we_strings, words, we->we_nbytes -1 );
    we->we_strings[ we->we_nbytes -1 ] = '\0';

	return error;
}

void wordfree(wordexp_t *we)
{
	assert(we != NULL);

	if ( we->we_wordv )
	{
        free(we->we_wordv);
	}
	if ( we->we_strings )
	{
        free(we->we_strings);
	}

	we->we_wordv = NULL;
	we->we_strings = NULL;
	we->we_nbytes = 0;
	we->we_wordc = 0;
}

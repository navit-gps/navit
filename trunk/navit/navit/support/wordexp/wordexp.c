#include <sys/types.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "wordexp.h"


static int
is_valid_variable_char(char c, int pos)
{
	if ( (pos && c >= '0' && c <= '9') ||
	      c == '_' ||
	     (c >= 'a' && c <= 'z') ||
	     (c >= 'A' && c <= 'Z'))
		return 1;
	return 0;
}

static char *
expand_variables(const char *in)
{
	char *var,*pos,*ret=strdup(in);
	char *val,*str;
	pos=ret;
	while ((var=strchr(pos, '$'))) {
		char *name,*begin=var+1;
		int npos=0,bpos=0,slen,elen;
		*var='\0';
		if (var[1] == '{') {
			begin++;
			while (begin[npos]) {
				bpos=npos;
				if (begin[npos++] == '}') 
					break;
			}
		} else {
			while (is_valid_variable_char(begin[npos],npos)) 
				npos++;	
			bpos=npos;
		}
		name=strdup(begin);
		name[bpos]='\0';		
		val=getenv(name);
		free(name);
		if (! val)
			val="";
		slen=strlen(ret)+strlen(val);
		elen=strlen(begin+npos);
		str=malloc(slen+elen+1);
		strcpy(str,ret);
		strcat(str,val);
		strcat(str,begin+npos);
		free(ret);
		ret=str;
		pos=ret+slen;
	}
	return ret;
}

int wordexp(const char * words, wordexp_t * we, int flags)
{
	int error=0;
	char *words_expanded;

	assert(we != NULL);
	assert(words != NULL);

	words_expanded=expand_variables(words);

	we->we_wordc = 1;
	we->we_wordv = NULL;
	we->we_strings = NULL;
	we->we_nbytes = 0;

	we->we_wordv = malloc( we->we_wordc * sizeof( char* ) );

	we->we_nbytes = strlen( words_expanded ) + 1;
	we->we_strings = malloc( we->we_nbytes );

	we->we_wordv[0] = &we->we_strings[0];

	// copy string & terminate
	memcpy( we->we_strings, words_expanded, we->we_nbytes -1 );
	we->we_strings[ we->we_nbytes -1 ] = '\0';
	free(words_expanded);

	return error;
}

void wordfree(wordexp_t *we)
{
	assert(we != NULL);

	if ( we->we_wordv )
        	free(we->we_wordv);
	if ( we->we_strings )
        	free(we->we_strings);

	we->we_wordv = NULL;
	we->we_strings = NULL;
	we->we_nbytes = 0;
	we->we_wordc = 0;
}

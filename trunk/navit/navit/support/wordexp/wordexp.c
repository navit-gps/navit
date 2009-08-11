
#include <config.h>

#include <sys/types.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "wordexp.h"
#include "glob.h"


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

/*
 * @brief replace all names of $NAME ${NAME}
 * with the corresponding environment variable 
 * @ param in: the string to be checked
 * @ return the expanded string or a copy of the existing string
 * must be free() by the calling function 
*/
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

/*
 * @brief minimal realization of wordexp according to IEEE standard
 * shall perform word expansion as described in the Shell
 * expansion of ´$NAME´ or ´${NAME}´
 * expansion of ´*´ and ´?´
 * @param words: pointer to a string containing one or more words to be expanded
 * but here only one word supported
 */
int 
wordexp(const char *words, wordexp_t *we, int flags)
{	
	int     i;
	int     error;	
	char   *words_expanded;
#ifdef HAVE_API_WIN32_BASE
	glob_t  pglob;
#endif

	assert(we != NULL);
	assert(words != NULL);
 
	/* expansion of ´$NAME´ or ´${NAME}´ */
	words_expanded=expand_variables(words);

#ifdef HAVE_API_WIN32_BASE
	/* expansion of ´*´, ´?´ */
	error=glob(words_expanded, 0, NULL, &pglob);
	if (!error)
	{
		/* copy the content of struct of glob into struct of wordexp */
		we->we_wordc = pglob.gl_pathc;
		we->we_offs = pglob.gl_offs;
		we->we_wordv = malloc(we->we_wordc * sizeof(char*));
		for (i=0; i<we->we_wordc; i++)
		{
			we->we_wordv[i] = strdup(pglob.gl_pathv[i]);
		}		
		globfree(&pglob);
		free(words_expanded);
	}
	else
	{
#endif
		we->we_wordc = 1;		
		we->we_wordv = malloc(sizeof(char*));	
		we->we_wordv[0] = words_expanded;
#ifdef HAVE_API_WIN32_BASE
	}
#endif


	return error;	
}


void wordfree(wordexp_t *we)
{
	int i;

	for (i=0; i < we->we_wordc; i++)
	{
		free (we->we_wordv[i]);
	}
	
	free (we->we_wordv);
	we->we_wordc = 0;
}

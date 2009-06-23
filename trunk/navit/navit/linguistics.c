#include <string.h>
#include <stdio.h>
#include <glib.h>
#include "debug.h"
#include "linguistics.h"

static const char *special[][3]={
{"Ä","A","AE"},
{"Ö","O","OE"},
{"Ü","U","UE"},
{"Ő","O"},
{"Ű","U"},
{"Á","A"},
{"É","E"},
{"Í","I"},
{"Ó","O"},
{"Ú","U"},
{"ä","a","ae"},
{"ö","o","oe"},
{"ü","u","ue"},
{"ő","o"},
{"ű","u"},
{"á","a"},
{"é","e"},
{"í","i"},
{"ó","o"},
{"ú","u"},
{"ß","s","ss"},
};

char *
linguistics_expand_special(char *str, int mode)
{
	char *in=str;
	char *out,*ret;
	int found=0;
	out=ret=g_strdup(str);
	mode++;
	while (*in) {
		char *next=g_utf8_find_next_char(in, NULL);
		int i,len=next-in;
		int match=0;
		if (len > 1) {
			for (i = 0 ; i < sizeof(special)/sizeof(special[0]); i++) {
				const char *search=special[i][0];
				if (!strncmp(in,search,len)) {
					const char *replace=special[i][mode];
					if (replace) {
						int replace_len=strlen(replace);
						dbg_assert(replace_len <= len);
						dbg(1,"found %s %s %s\n",in,search,replace);
						strcpy(out, replace);
						out+=replace_len;
						match=1;
						break;
					}
				}
			}
			in=next;
		}
		if (match) 
			found=1;
		else {	
			while (len-- > 0) 
				*out++=*in++;
		}
	}
	*out++='\0';
	if (!found) {
		g_free(ret);
		ret=NULL;
	}
	return ret;
}

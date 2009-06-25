#include <string.h>
#include <stdio.h>
#include <glib.h>
#include "debug.h"
#include "linguistics.h"

static const char *special[][3]={
/* Capital Diacritics */
/* ¨ Diaresis */
{"Ä","A","AE"},
{"Ö","O","OE"},
{"Ü","U","UE"},
/* ˝ Double Acute Accent */
{"Ő","O"},
{"Ű","U"},
/* ´ Acute Accent */
{"Á","A"},
{"Ć","C"},
{"É","E"},
{"Í","I"},
{"Ń","N"},
{"Ó","O"},
{"Ś","S"},
{"Ú","U"},
{"Ý","Y"},
{"Ź","Z"},
/* ˛ Ogonek */
{"Ą","A"},
{"Ę","E"},
/* ˙ Dot */
{"Ż","Z"},
/* – Stroke */
{"Ł","L"},
/* ˚ Ring */
{"Å","A","AA"},
{"Ů","U"},
/* ˇ Caron */
{"Č","C"},
{"Ď","D"},
{"Ě","E"},
{"Ň","N"},
{"Ř","R"},
{"Š","S"},
{"Ť","T"},
{"Ž","Z"},
/* / Slash */
{"Ø","O","OE"},
/* ligatures */
{"Æ","A","AE"},
/* Small Diacritics */
/* ¨ Diaresis */
{"ä","a","ae"},
{"ö","o","oe"},
{"ü","u","ue"},
/* ˝ Double Acute Accent */
{"ő","o"},
{"ű","u"},
/* ´ Acute Accent */
{"á","a"},
{"ć","c"},
{"é","e"},
{"í","i"},
{"ń","n"},
{"ó","o"},
{"ś","s"},
{"ú","u"},
{"ý","y"},
{"ź","z"},
/* ˛ Ogonek */
{"ą","a"},
{"ę","e"},
/* ˙ Dot */
{"ż","z"},
/* – Stroke */
{"ł","l"},
/* ˚ Ring */
{"ů","u"},
{"å","a", "aa"},
/* ˇ Caron */
{"č","c"},
{"ď","d"},
{"ě","e"},
{"Ň","N"},
{"ř","r"},
{"š","s"},
{"ť","t"},
{"ž","z"},
/* / Slash */
{"ø","o", "oe"},
/* ligatures */
{"æ","a","ae"},
{"ß","s","ss"},
};

char *
linguistics_expand_special(char *str, int mode)
{
	char *in=str;
	char *out,*ret;
	int found=0;
	out=ret=g_strdup(str);
	if (!mode) 
		return ret;
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
						dbg(1,"found %s %s %d %s %d\n",in,search,len,replace,replace_len);
						strcpy(out, replace);
						out+=replace_len;
						match=1;
						break;
					}
				}
			}
		}
		if (match) {
			found=1;
			in=next;
		} else {
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

char *
linguistics_next_word(char *str)
{
	int len=strcspn(str, " -");
	if (!str[len] || !str[len+1])
		return NULL;
	return str+len+1;
}

void
linguistics_init(void)
{
}

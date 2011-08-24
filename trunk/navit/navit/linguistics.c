#include <string.h>
#include <stdio.h>
#include <glib.h>
#include "debug.h"
#include "linguistics.h"

static const char *special[][3]={
/* Capital Diacritics */
/* ¨ Diaresis */
{"Ä","A","AE"},
{"Ë","E"},
{"Ï","I"},
{"Ö","O","OE"},
{"Ü","U","UE"},
{"Ÿ","Y"},
/* ˝ Double Acute Accent */
{"Ő","O","Ö"},
{"Ű","U","Ü"},
/* ´ Acute Accent */
{"Á","A"},
{"Ć","C"},
{"É","E"},
{"Í","I"},
{"Ĺ","L"},
{"Ń","N"},
{"Ó","O"},
{"Ŕ","R"},
{"Ś","S"},
{"Ú","U"},
{"Ý","Y"},
{"Ź","Z"},
/* ˛ Ogonek (nosinė) */
{"Ą","A"},
{"Ę","E"},
{"Į","I"},
{"Ų","U"},
/* ˙ Dot */
{"Ċ","C"},
{"Ė","E"},
{"Ġ","G"},
{"İ","I"},
{"Ŀ","L"},
{"Ż","Z"},
/* – Stroke */
{"Đ","D","DJ"}, /* Croatian Dj, not to be confused with the similar-looking Icelandic Eth */
{"Ħ","H"},
{"Ł","L"},
{"Ŧ","T"},
/* ˚ Ring */
{"Å","A","AA"},
{"Ů","U"},
/* ˇ Caron (haček, paukščiukas) */
{"Č","C"},
{"Ď","D"},
{"Ě","E"},
{"Ľ","L"},
{"Ň","N"},
{"Ř","R"},
{"Š","S"},
{"Ť","T"},
{"Ž","Z"},
/* / Slash */
{"Ø","O","OE"},
/* ¯ Macron */
{"Ā","A","AA"},
{"Ē","E","EE"},
{"Ī","I","II"},
{"Ō","O","OO"},
{"Ū","U","UU"},
/* ˘ Brevis */
{"Ă","A"},
{"Ĕ","E"},
{"Ğ","G"},
{"Ĭ","I"},
{"Ŏ","O"},
{"Ŭ","U"},
/* ^ Circumflex */
{"Â","A"},
{"Ĉ","C"},
{"Ê","E"},
{"Ĝ","G"},
{"Ĥ","H"},
{"Î","I"},
{"Ĵ","J"},
{"Ô","O"},
{"Ŝ","S"},
{"Û","U"},
{"Ŵ","W"},
{"Ŷ","Y"},
/* ¸ Cedilla */
{"Ç","C"},
{"Ģ","G","GJ"},
{"Ķ","K","KJ"},
{"Ļ","L","LJ"},
{"Ņ","N","NJ"},
{"Ŗ","R"},
{"Ş","S"},
{"Ţ","T"},
/* ~ Tilde */
{"Ã","A"},
{"Ĩ","I"},
{"Ñ","N"},
{"Õ","O"},
{"Ũ","U"},
/* ` Grave */
{"À","A"},
{"È","E"},
{"Ì","I"},
{"Ò","O"},
{"Ù","U"},
/* ligatures */
{"Æ","A","AE"},
{"Ĳ","IJ"},
{"Œ","O","OE"},
/* special letters */
{"Ð","D","DH"}, /* Icelandic Eth, not to be confused with the similar-looking Croatian Dj */
{"Ŋ","N","NG"},
{"Þ","T","TH"},
/* Small Diacritics */
/* ¨ Diaresis */
{"ä","a","ae"},
{"ë","e"},
{"ï","i"},
{"ö","o","oe"},
{"ü","u","ue"},
{"ÿ","y"},
/* ˝ Double Acute Accent */
{"ő","o","ö"},
{"ű","u","ü"},
/* ´ Acute Accent */
{"á","a"},
{"ć","c"},
{"é","e"},
{"í","i"},
{"ĺ","l"},
{"ń","n"},
{"ó","o"},
{"ŕ","r"},
{"ś","s"},
{"ú","u"},
{"ý","y"},
{"ź","z"},
/* ˛ Ogonek (nosinė) */
{"ą","a"},
{"ę","e"},
{"į","i"},
{"ų","u"},
/* ˙ Dot (and dotless i) */
{"ċ","c"},
{"ė","e"},
{"ġ","g"},
{"ı","i"},
{"ŀ","l"},
{"ż","z"},
/* – Stroke */
{"đ","d","dj"},
{"ħ","h"},
{"ł","l"},
{"ŧ","t"},
/* ˚ Ring */
{"å","a", "aa"},
{"ů","u"},
/* ˇ Caron (haček, paukščiukas) */
{"č","c"},
{"ď","d"},
{"ě","e"},
{"ľ","l"},
{"ň","n"},
{"ř","r"},
{"š","s"},
{"ť","t"},
{"ž","z"},
/* / Slash */
{"ø","o", "oe"},
/* Macron */
{"ā","a","aa"},
{"ē","e","ee"},
{"ī","i","ii"},
{"ō","o","oo"},
{"ū","u","uu"},
/* ˘ Brevis */
{"ă","a"},
{"ĕ","e"},
{"ğ","g"},
{"ĭ","i"},
{"ŏ","o"},
{"ŭ","u"},
/* ^ Circumflex */
{"â","a"},
{"ĉ","c"},
{"ê","e"},
{"ĝ","g"},
{"ĥ","h"},
{"î","i"},
{"ĵ","j"},
{"ô","o"},
{"ŝ","s"},
{"û","u"},
{"ŵ","w"},
{"ŷ","y"},
/* ¸ Cedilla */
{"ç","c"},
{"ģ","g","gj"},
{"ķ","k","kj"},
{"ļ","l","lj"},
{"ņ","n","nj"},
{"ŗ","r"},
{"ş","s"},
{"ţ","t"},
/* ~ Tilde */
{"ã","a"},
{"ĩ","i"},
{"õ","o"},
{"ñ","n"},
{"ũ","u"},
/* ` Grave */
{"à","a"},
{"è","e"},
{"ì","i"},
{"ò","o"},
{"ù","u"},
/* ligatures */
{"æ","a","ae"},
{"ĳ","ij"},
{"œ","o","oe"},
{"ß","s","ss"},
/* special letters */
{"ð","d","dh"},
{"ŋ","n","ng"},
{"þ","t","th"},
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
	int len=strcspn(str, " -/()");
	if (!str[len] || !str[len+1])
		return NULL;
	return str+len+1;
}

int
linguistics_search(char *str)
{
	if (!g_strcasecmp(str,"str"))
		return 0;
	if (!g_strcasecmp(str,"str."))
		return 0;
	if (!g_strcasecmp(str,"strasse"))
		return 0;
	if (!g_strcasecmp(str,"weg"))
		return 0;
	return 1;
}

void
linguistics_init(void)
{
}

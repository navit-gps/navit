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

/* Cyrillic capital */
{"Ё","Е"},
{"І","I"},
{"Ї","I"},
{"Ў","У"},
{"Є","Е","Э"},
{"Ґ","Г"},
{"Ѓ","Г"},
{"Ђ","Д"},
{"Ќ","К"},
{"Љ","Л","ЛЬ"},
{"Њ","Н","НЬ"},
{"Џ","Ц"},

/* Cyrillic small */
{"ё","е"},
{"і","i"},
{"ї","i"},
{"ў","у"},
{"є","е","э"},
{"ґ","г"},
{"ѓ","г"},
{"ђ","д"},
{"ќ","к"},
{"љ","л","ль"},
{"њ","н","нь"},
{"џ","ц"},

};

/* Array of strings for case conversion
 * Even elements of array are strings of upper-case letters
 * Odd elements of array are strings of lower-case letters, in the order corresponding to directly preceeding even element.
 * Last element of array should be NULL.
 */
static const char *upperlower[]={
/*Latin diacritics*/
"ÄËÏÖÜŸŐŰÁĆÉÍĹŃÓŔŚÚÝŹĄĘĮŲĊĖĠİĿŻĐĦŁŦÅŮČĎĚĽŇŘŠŤŽØĀĒĪŌŪĂĔĞĬŎŬÂĈÊĜĤÎĴÔŜÛŴŶÇĢĶĻŅŖŞŢÃĨÑÕŨÀÈÌÒÙÆĲŒÐŊÞ",
"äëïöüÿőűáćéíĺńóŕśúýźąęįųċėġıŀżđħłŧåůčďěľňřšťžøāēīōūăĕğĭŏŭâĉêĝĥîĵôŝûŵŷçģķļņŗşţãĩõñũàèìòùæĳœðŋþ",
/*Cyrillic*/
"АБВГҐЃДЂЕЄЁЖЗИЙКЌЛЉМНЊОПРСТУФХЦЏЧШЩЪЫЬЭЮЯІЇЎ",
"абвгґѓдђеєёжзийкќлљмнњопрстуфхцџчшщъыьэюяіїў",

NULL
};

static GHashTable *casefold_hash;


/*
 * @brief Prepare an utf-8 string for case insensitive comparison.
 * @param in String to prepeare.
 * @return String prepared for case insensitive search. Result shoud be g_free()d after use.
 */
char*
linguistics_casefold(char *in)
{
	int len=strlen(in);
	char *src=in;
	char *ret=g_new(char,len+1);
	char *dest=ret;
	char buf[10];
	while(*src && dest-ret<len){
		if(*src>='A' && *src<='Z') {
			*dest++=*src++ - 'A' + 'a';
		} else if (!(*src&128)) {
			*dest++=*src++;
		} else {
			int charlen;
			char *tmp, *folded;
			tmp=g_utf8_find_next_char(src,NULL);
			charlen=tmp-src+1;
			g_strlcpy(buf,src,charlen>10?10:charlen);
			folded=g_hash_table_lookup(casefold_hash,buf);
			if(folded) {
				while(*folded && dest-ret<len)
					*dest++=*folded++;
				src=tmp;
			} else {
				while(src<tmp && dest-ret<len)
					*dest++=*src++;
			}
		}
	}
	*dest=0;
	if(*src)
		dbg(0,"Casefolded string for '%s' needs extra space, result is trucated to '%s'.\n",in,ret);
	return ret;
}


/**
 * @brief Replace special characters in string (e.g. umlauts) with plain letters.
 * This is useful e.g. to canonicalize a string for comparison.
 *
 * @param str string to process
 * @param mode Replacement mode. 0=do nothing, 1=replace with single
 * UTF character, 2=replace with multiple letters if the commonly used
 * replacement has multitple letter (e.g. a-umlaut -> ae)
 * @returns copy of string, with characters replaced
 */
char *
linguistics_expand_special(char *str, int mode)
{
	char *in=str;
	char *out,*ret;
	int found=0;
	int ret_len=strlen(str);
	int in_rest=ret_len;
	out=ret=g_strdup(str);
	if (!mode) 
		return ret;
	while (*in) {
		char *next=g_utf8_find_next_char(in, NULL);
		int i,len;
		int match=0;

		if(next)
			len=next-in;
		else
			len=strlen(in);

		in_rest-=len;
		
		if (len > 1) {
			for (i = 0 ; i < sizeof(special)/sizeof(special[0]); i++) {
				const char *search=special[i][0];
				if (!strncmp(in,search,len)) {
					const char *replace=special[i][mode];
					if (replace) {
						int replace_len=strlen(replace);
						if(out-ret+replace_len+in_rest>ret_len) {
							char *new_ret;
							ret_len+=(replace_len-len)*10;
							new_ret=g_realloc(ret,ret_len+1);
							out=new_ret+(out-ret);
							ret=new_ret;
						}
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
			in+=len;
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

/**
 * @brief Copy one utf8 encoded char to newly allocated buffer.
 *
 * @param s pointer to the beginning of the char.
 * @return  newly allocated nul-terminated string containing one utf8 encoded character.
 */
static char 
*linguistics_dup_utf8_char(const char *s)
{
	char *ret, *next;
	next=g_utf8_find_next_char(s,NULL);
	ret=g_new(char, next-s+1);
	g_strlcpy(ret,s,next-s+1);
	return ret;
}

void
linguistics_init(void)
{
	int i;

	casefold_hash=g_hash_table_new_full(g_str_hash, g_str_equal,g_free,g_free);

	for (i = 0 ; upperlower[i]; i+=2) {
		int j,k;
		for(j=0,k=0;upperlower[i][j] && upperlower[i+1][k];) {
			char *s1=linguistics_dup_utf8_char(upperlower[i]+j);
			char *s2=linguistics_dup_utf8_char(upperlower[i+1]+k);
			g_hash_table_insert(casefold_hash,s1,s2);
			j+=strlen(s1);
			k+=strlen(s2);
		}
	}
}

void
linguistics_free(void)
{
	g_hash_table_destroy(casefold_hash);
	casefold_hash=NULL;
}


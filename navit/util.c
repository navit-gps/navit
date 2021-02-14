/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <stdlib.h>
#include <glib.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

#ifdef _POSIX_C_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif
#ifdef _MSC_VER
typedef int ssize_t ;
#endif
#include "util.h"
#include "debug.h"
#include "config.h"

void strtoupper(char *dest, const char *src) {
    while (*src)
        *dest++=toupper(*src++);
    *dest='\0';
}

void strtolower(char *dest, const char *src) {
    while (*src)
        *dest++=tolower(*src++);
    *dest='\0';
}

/**
 * @brief Fast compute of square root for unsigned ints
 *
 * @param n The input number
 * @return sqrt(n)
 */
unsigned int uint_sqrt(unsigned int n) {
    unsigned int h, p= 0, q= 1, r= n;

    /* avoid q rollover */
    if(n >= (1<<(sizeof(n)*8-2))) {
        q = 1<<(sizeof(n)*8-2);
    } else {
        while ( q <= n ) {
            q <<= 2;
        }
        q >>= 2;
    }

    while ( q != 0 ) {
        h = p + q;
        p >>= 1;
        if ( r >= h ) {
            p += q;
            r -= h;
        }
        q >>= 2;
    }
    return p;
}

int navit_utf8_strcasecmp(const char *s1, const char *s2) {
    char *s1_folded,*s2_folded;
    int cmpres;
    s1_folded=g_utf8_casefold(s1,-1);
    s2_folded=g_utf8_casefold(s2,-1);
    cmpres=strcmp(s1_folded,s2_folded);
    dbg(lvl_debug,"Compared %s with %s, got %d",s1_folded,s2_folded,cmpres);
    g_free(s1_folded);
    g_free(s2_folded);
    return cmpres;
}

/**
 * @brief Trims all leading and trailing whitespace characters from a string.
 *
 * Whitespace characters are all up to and including 0x20.
 *
 * This function operates in-place, i.e. `s` will be modified.
 *
 * @param s The string to trim
 */
static void strtrim(char *s) {
    char *tmp = g_strdup(s);
    char *in = tmp;
    while (strlen(in) && (in[0] <= 0x20))
        in++;
    while (strlen(in) && (in[strlen(in) - 1] <= 0x20))
        in[strlen(in) - 1] = 0;
    strcpy(s, in);
    g_free(tmp);
}

/**
 * @brief Escape special characters from a string
 *
 * @param mode The escape mode that needs to be enabled (see enum escape_mode)
 * @param in The string to escape
 *
 * @return The escaped string
 *
 * @note In html escape mode (escape_mode_html), we will only process HTML escape sequence, and string quoting, but we won't escape backslashes or double quotes
 * @warning The returned string has been allocated and g_free() must thus be called on this string
 */
char *str_escape(enum escape_mode mode, const char *in) {
    int len=mode & escape_mode_string ? 2:0;	/* Add 2 characters to the length of the buffer if quoting is enabled */
    char *dst,*out;
    const char *src=in;
    static const char *quot="&quot;";
    static const char *apos="&apos;";
    static const char *amp="&amp;";
    static const char *lt="&lt;";
    static const char *gt="&gt;";

    dbg(lvl_debug, "Will escape string=\"%s\", escape mode %d", in, mode);
    while (*src) {
        if ((*src == '"' || *src == '\\') && (mode & (escape_mode_string | escape_mode_quote)))
            len++;
        if (*src == '"' && mode == escape_mode_html_quote)
            len+=strlen(quot);
        else if (*src == '\'' && mode == escape_mode_html_apos)
            len+=strlen(apos);
        else if (*src == '&' && mode == escape_mode_html_amp)
            len+=strlen(amp);
        else if (*src == '<' && mode == escape_mode_html_lt)
            len+=strlen(lt);
        else if (*src == '>' && mode == escape_mode_html_gt)
            len+=strlen(gt);
        else
            len++;
        src++;
    }
    src=in;
    out=dst=g_malloc(len+1); /* +1 character for NUL termination */

    /* In string quoting mode (escape_mode_string), prepend the whole string with a double quote */
    if (mode & escape_mode_string)
        *dst++='"';

    while (*src) {
        if (mode & escape_mode_html) {	/* In html escape mode, only process HTML escape sequence, not backslashes or quotes */
            if (*src == '"' && (mode & escape_mode_html_quote)) {
                strcpy(dst,quot);
                src++;
                dst+=strlen(quot);
            } else if (*src == '\'' && (mode & escape_mode_html_apos)) {
                strcpy(dst,apos);
                src++;
                dst+=strlen(apos);
            } else if (*src == '&' && (mode & escape_mode_html_amp)) {
                strcpy(dst,amp);
                src++;
                dst+=strlen(amp);
            } else if (*src == '<' && (mode & escape_mode_html_lt)) {
                strcpy(dst,lt);
                src++;
                dst+=strlen(lt);
            } else if (*src == '>' && (mode & escape_mode_html_gt)) {
                strcpy(dst,gt);
                src++;
                dst+=strlen(gt);
            } else
                *dst++=*src++;
        } else {
            if ((*src == '"' || *src == '\\') && (mode & (escape_mode_string | escape_mode_quote))) {
                *dst++='\\';
            }
            *dst++=*src++;
        }
    }

    /* In string quoting mode (escape_mode_string), append a double quote to the whole string */
    if (mode & escape_mode_string)
        *dst++='"';

    *dst++='\0';
    dbg(lvl_debug, "Result of escaped string=\"%s\"", out);
    return out;
}

/**
 * @brief Copy a string from @p src to @p dest, unescaping characters
 *
 * @note Escaped characters are "\\\\" (double backslash) resulting in '\\' (single backslash)
 *       and "\\\"" (backslash followed by double quote), resulting in '"' (double quote)
 *       but we will escape any other character, for example "\\ " will result in ' ' (space)
 *       This is the reverse of function str_escape, except that we assume (and only support) unescaping mode escape_mode_quote here
 *
 * @param[out] dest The location where to store the unescaped string
 * @param[in] src The source string to copy (and to unescape)
 * @param n The maximum amount of bytes copied into dest. Warning: If there is no null byte among the n bytes written to dest, the string placed in dest will not be null-terminated.
 *
 * @return A pointer to the destination string @p dest
 */
char *strncpy_unescape(char *dest, const char *src, size_t n) {
    char *dest_ptr;	/* A pointer to the currently parsed character inside string dest */

    for (dest_ptr=dest; (dest_ptr-dest) < n && (*src != '\0'); src++, dest_ptr++) {
        if (*src == '\\') {
            src++;
        }
        *dest_ptr = *src;
        if (*dest_ptr == '\0') {
            /* This is only possible if we just parsed an escaped sequence '\\' followed by a NUL termination, which is not really sane, but we will silently accept this case */
            return dest;
        }
    }
    if ((dest_ptr-dest) < n)
        *dest_ptr='\0';	/* Add a trailing '\0' if any room is remaining */
    else {
        // strncpy_unescape will return a non NUL-terminated string. Trouble ahead if this is not handled properly
    }

    return dest;
}

/**
 * @brief Parser states for `parse_for_systematic_comparison()`.
 */
enum parse_state {
    parse_state_whitespace,
    parse_state_numeric,
    parse_state_alpha,
};

/**
 * @brief Parses a string for systematic comparison.
 *
 * This is a helper function for `compare_name_systematic()`.
 *
 * The string is broken down into numeric and non-numeric parts. Whitespace characters are discarded
 * unless they are surrounded by string characters, in which case the whole unit is treated as one
 * string part. All strings are converted to lowercase and leading zeroes stripped from numbers.
 *
 * @param s The string to parse
 *
 * @return A buffer containing the parsed string, parts delimited by a null character, the last part
 * followed by a double null character.
 */
static char * parse_for_systematic_comparison(const char *s) {
    char *ret = g_malloc0(strlen(s) * 2 + 1);
    const char *in = s;
    char *out = ret;
    char *part;
    enum parse_state state = parse_state_whitespace;
    int i = 0;
    char c;

    dbg(lvl_debug, "enter\n");

    while (i < strlen(in)) {
        c = in[i];
        if ((c <= 0x20) || (c == ',') || (c == '-') || (c == '.') || (c == '/')) {
            /* whitespace */
            if (state == parse_state_numeric) {
                part = g_malloc0(i + 1);
                strncpy(part, in, i);
                sprintf(part, "%d", atoi(part));
                strcpy(out, part);
                out += strlen(part) + 1;
                dbg(lvl_debug, "part='%s'\n", part);
                g_free(part);
                in += i;
                i = 1;
                state = parse_state_whitespace;
            } else
                i++;
        } else if ((c >= '0') && (c <= '9')) {
            /* numeric */
            if (state == parse_state_alpha) {
                part = g_malloc0(i + 1);
                strncpy(part, in, i);
                strtrim(part);
                strcpy(out, part);
                out += strlen(part) + 1;
                dbg(lvl_debug, "part='%s'\n", part);
                g_free(part);
                in += i;
                i = 1;
            } else
                i++;
            state = parse_state_numeric;
        } else {
            /* alpha */
            if (state == parse_state_numeric) {
                part = g_malloc0(i + 1);
                strncpy(part, in, i);
                sprintf(part, "%d", atoi(part));
                strcpy(out, part);
                out += strlen(part) + 1;
                dbg(lvl_debug, "part='%s'\n", part);
                g_free(part);
                in += i;
                i = 1;
            } else
                i++;
            state = parse_state_alpha;
        }
    }

    if (strlen(in) > 0) {
        if (state == parse_state_numeric) {
            part = g_malloc0(strlen(in) + 1);
            strcpy(part, in);
            sprintf(part, "%d", atoi(part));
            strcpy(out, part);
            dbg(lvl_debug, "part='%s'\n", part);
            g_free(part);
        } else if (state == parse_state_alpha) {
            part = g_malloc0(strlen(in) + 1);
            strcpy(part, in);
            strtrim(part);
            strcpy(out, part);
            dbg(lvl_debug, "part='%s'\n", part);
            g_free(part);
        }
    }

    return ret;
}

/**
 * @brief Compares two name_systematic strings.
 *
 * A name_systematic string is typically used for road reference numbers (A 4, I-51, SP526). This
 * function performs a fuzzy comparison: Each string is broken down into numeric and non-numeric parts.
 * Then both strings are compared part by part. The following rules apply:
 *
 * \li Semicolons denote sequences of strings, and the best match between any pair of strings from `s1` and `s2` is
 * returned.
 * \li Whitespace bordering on a number is discarded.
 * \li Whitespace surrounded by string characters is treated as one string with the surrounding characters.
 * \li If one string has more parts than the other, the shorter string is padded with null parts.
 * \li null equals null.
 * \li null does not equal non-null.
 * \li Numeric parts are compared as integers, hence `'042'` equals `'42'`.
 * \li Comparison of string parts is case-insensitive.
 *
 * Partial matches are currently determined by determining each part of one string with each part of the other. Each
 * part of one string that is matched by at least one part of the other increases the score. Order is currently not
 * taken into account, i.e. `'42A'` and `'A-42A'` are both considered full (not partial) matches for `'A42'`. Future
 * versions may change this.
 *
 * @param s1 The first string
 * @param s2 The second string
 *
 * @return 0 if both strings match, nonzero if they do not. `MAX_MISMATCH` indicates a complete mismatch; values in
 * between indicate partial matches (lower values correspond to better matches).
 */
int compare_name_systematic(const char *s1, const char *s2) {
    int ret = MAX_MISMATCH;
    int tmp;
    int elements = 0, matches = 0;
    char *l = NULL, *r = NULL, *l0, *r0;

    if (!s1 || !s1[0]) {
        if (!s2 || !s2[0])
            return 0;
        else
            return MAX_MISMATCH;
    } else if (!s2 || !s2[0])
        return MAX_MISMATCH;

    /* break up strings at semicolons and parse each separately, return 0 if any two match */
    if (strchr(s1, ';')) {
        l = g_strdup(s1);
        for (l0 = strtok(l, ";"); l0; l0 = strtok(NULL, ";")) {
            tmp = compare_name_systematic(l0, s2);
            if (tmp < ret)
                ret = tmp;
            if (!ret)
                break;
        }
        g_free(l);
        return ret;
    } else if (strchr(s2, ';')) {
        r = g_strdup(s2);
        for (r0 = strtok(r, ";"); r0; r0 = strtok(NULL, ";")) {
            tmp = compare_name_systematic(s1, r0);
            if (tmp < ret)
                ret = tmp;
            if (!ret)
                break;
        }
        g_free(r);
        return ret;
    }

    /* s1 and s2 are single strings (no semicolons) */
    l0 = parse_for_systematic_comparison(s1);
    r0 = parse_for_systematic_comparison(s2);

    /* count left-hand elements and all left-hand elements matched by a right-hand element */
    for (l = l0; l[0]; l += strlen(l) + 1) {
        elements++;
        for (r = r0; r[0]; r += strlen(r) + 1) {
            if (atoi(l) || (l[0] == '0')) {
                if ((atoi(r) || (r[0] == '0')) && (atoi(l) == atoi(r))) {
                    matches++;
                    break;
                }
            } else if (!strcasecmp(l, r)) {
                matches++;
                break;
            }
        }
    }

    /* same in the opposite direction */
    for (r = r0; r[0]; r += strlen(r) + 1) {
        elements++;
        for (l = l0; l[0]; l += strlen(l) + 1) {
            if (atoi(l) || (l[0] == '0')) {
                if ((atoi(r) || (r[0] == '0')) && (atoi(l) == atoi(r))) {
                    matches++;
                    break;
                }
            } else if (!strcasecmp(l, r)) {
                matches++;
                break;
            }
        }
    }

    g_free(l0);
    g_free(r0);

    ret = ((elements - matches) * MAX_MISMATCH) / elements;

    dbg(lvl_debug, "'%s' %s '%s', ret=%d",
        s1, ret ? (ret == MAX_MISMATCH ? "does NOT match" : "PARTIALLY matches") : "matches", s2, ret);

    return ret;
}

static void hash_callback(gpointer key, gpointer value, gpointer user_data) {
    GList **l=user_data;
    *l=g_list_prepend(*l, value);
}

GList *g_hash_to_list(GHashTable *h) {
    GList *ret=NULL;
    g_hash_table_foreach(h, hash_callback, &ret);

    return ret;
}

static void hash_callback_key(gpointer key, gpointer value, gpointer user_data) {
    GList **l=user_data;
    *l=g_list_prepend(*l, key);
}

GList *g_hash_to_list_keys(GHashTable *h) {
    GList *ret=NULL;
    g_hash_table_foreach(h, hash_callback_key, &ret);

    return ret;
}

/**
 * @brief Appends a formatted string and appends it to an existing one.
 *
 * Usage is similar to the familiar C functions that take a format string and a variable argument list.
 *
 * Return value is a concatenation of `buffer` (unless it is NULL) and `fmt`, with the remaining arguments
 * inserted into `fmt`.
 *
 * @param buffer An existing string, can be null and will be freed by this function
 * @param fmt A format string (will not be altered)
 *
 * @return A newly allocated string, see description. The caller is responsible for freeing the returned string.
 */
gchar *g_strconcat_printf(gchar *buffer, gchar *fmt, ...) {
    gchar *str,*ret;
    va_list ap;

    va_start(ap, fmt);
    str=g_strdup_vprintf(fmt, ap);
    va_end(ap);
    if (! buffer)
        return str;
    ret=g_strconcat(buffer, str, NULL);
    g_free(buffer);
    g_free(str);
    return ret;
}

#ifndef HAVE_GLIB
int g_utf8_strlen_force_link(gchar *buffer, int max);
int g_utf8_strlen_force_link(gchar *buffer, int max) {
    return g_utf8_strlen(buffer, max);
}
#endif

#if defined(_WIN32) || defined(__CEGCC__)
#include <windows.h>
#include <sys/types.h>
#endif

#if defined(_WIN32) || defined(__CEGCC__) || defined (__APPLE__) || defined(HAVE_API_ANDROID)
char *stristr(const char *String, const char *Pattern) {
    char *pptr, *sptr, *start;

    for (start = (char *)String; *start != (int)NULL; start++) {
        /* find start of pattern in string */
        for ( ; ((*start!=(int)NULL) && (toupper(*start) != toupper(*Pattern))); start++)
            ;
        if ((int)NULL == *start)
            return NULL;

        pptr = (char *)Pattern;
        sptr = (char *)start;

        while (toupper(*sptr) == toupper(*pptr)) {
            sptr++;
            pptr++;

            /* if end of pattern then pattern was found */

            if ((int)NULL == *pptr)
                return (start);
        }
    }
    return NULL;
}

#ifndef SIZE_MAX
# define SIZE_MAX ((size_t) -1)
#endif
#ifndef SSIZE_MAX
# define SSIZE_MAX ((ssize_t) (SIZE_MAX / 2))
#endif
#if !HAVE_FLOCKFILE
# undef flockfile
# define flockfile(x) ((void) 0)
#endif
#if !HAVE_FUNLOCKFILE
# undef funlockfile
# define funlockfile(x) ((void) 0)
#endif

/* Some systems, like OSF/1 4.0 and Woe32, don't have EOVERFLOW.  */
#ifndef EOVERFLOW
# define EOVERFLOW E2BIG
#endif


#ifndef HAVE_GETDELIM
/**
 * @brief Reads the part of a file up to a delimiter to a string.
 * <p>
 * Read up to (and including) a DELIMITER from FP into *LINEPTR (and NUL-terminate it).
 *
 * @param lineptr Pointer to a pointer returned from malloc (or NULL), pointing to a buffer. It is
 * realloc'ed as necessary and will receive the data read.
 * @param n Size of the buffer.
 *
 * @return Number of characters read (not including the null terminator), or -1 on error or EOF.
*/
ssize_t getdelim (char **lineptr, size_t *n, int delimiter, FILE *fp) {
    int result;
    size_t cur_len = 0;

    if (lineptr == NULL || n == NULL || fp == NULL) {
        return -1;
    }

    flockfile (fp);

    if (*lineptr == NULL || *n == 0) {
        *n = 120;
        *lineptr = (char *) realloc (*lineptr, *n);
        if (*lineptr == NULL) {
            result = -1;
            goto unlock_return;
        }
    }

    for (;;) {
        int i;

        i = getc (fp);
        if (i == EOF) {
            result = -1;
            break;
        }

        /* Make enough space for len+1 (for final NUL) bytes.  */
        if (cur_len + 1 >= *n) {
            size_t needed_max=SIZE_MAX;
            size_t needed = 2 * *n + 1;   /* Be generous. */
            char *new_lineptr;
            if (needed_max < needed)
                needed = needed_max;
            if (cur_len + 1 >= needed) {
                result = -1;
                goto unlock_return;
            }

            new_lineptr = (char *) realloc (*lineptr, needed);
            if (new_lineptr == NULL) {
                result = -1;
                goto unlock_return;
            }

            *lineptr = new_lineptr;
            *n = needed;
        }

        (*lineptr)[cur_len] = i;
        cur_len++;

        if (i == delimiter)
            break;
    }
    (*lineptr)[cur_len] = '\0';
    result = cur_len ? cur_len : result;

unlock_return:
    funlockfile (fp); /* doesn't set errno */

    return result;
}
#endif

#ifndef HAVE_GETLINE
ssize_t getline (char **lineptr, size_t *n, FILE *stream) {
    return getdelim (lineptr, n, '\n', stream);
}
#endif

#if defined(_UNICODE)
wchar_t* newSysString(const char *toconvert) {
    int newstrlen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, toconvert, -1, 0, 0);
    wchar_t *newstring = g_new(wchar_t,newstrlen);
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, toconvert, -1, newstring, newstrlen) ;
    return newstring;
}
#else
char * newSysString(const char *toconvert) {
    return g_strdup(toconvert);
}
#endif
#endif

/**
 * @brief Optimizes the format of a string, adding carriage returns so that when displayed, the result text zone is roughly as wide as high
 *
 * @param[in,out] s The string to proces (will be modified by this function, but length will be unchanged)
 */
void square_shape_str(char *s) {
    char *c;
    char *last_break;
    unsigned int max_cols = 0;
    unsigned int cur_cols = 0;
    unsigned int max_rows = 0;
    unsigned int surface;
    unsigned int target_cols;

    if (!s)
        return;
    for (c=s; *c!='\0'; c++) {
        if (*c==' ') {
            if (max_cols < cur_cols)
                max_cols = cur_cols;
            cur_cols = 0;
            max_rows++;
        } else
            cur_cols++;
    }
    if (max_cols < cur_cols)
        max_cols = cur_cols;
    if (cur_cols)	/* If last line does not end with CR, add it to line numbers anyway */
        max_rows++;
    /* Give twice more room for rows (hence the factor 2 below)
     * This will render as a rectangular shape, taking more horizontal space than vertical */
    surface = max_rows * 2 * max_cols;
    target_cols = uint_sqrt(surface);

    if (target_cols < max_cols)
        target_cols = max_cols;

    target_cols = target_cols + target_cols/10;	/* Allow 10% extra on columns */
    dbg(lvl_debug, "square_shape_str(): analyzing input text=\"%s\". max_rows=%u, max_cols=%u, surface=%u, target_cols=%u",
        s, max_rows, max_cols, max_rows * 2 * max_cols, target_cols);

    cur_cols = 0;
    last_break = NULL;
    for (c=s; *c!='\0'; c++) {
        if (*c==' ') {
            if (cur_cols>=target_cols) {	/* This line is too long, break at the previous non alnum character */
                if (last_break) {
                    *last_break =
                        '\n';	/* Replace the previous non alnum character with a line break, this creates a new line and prevents the previous line from being too long */
                    cur_cols = c-last_break;
                }
            }
            last_break = c;	/* Record this position as a candidate to insert a line break */
        }
        cur_cols++;
    }
    if (cur_cols>=target_cols && last_break) {
        *last_break =
            '\n';	/* Replace the previous non alnum character with a line break, this creates a new line and prevents the previous line from being too long */
    }

    dbg(lvl_debug, "square_shape_str(): output text=\"%s\"", s);
}

#if defined(_MSC_VER) || (!defined(HAVE_GETTIMEOFDAY) && defined(HAVE_API_WIN32_BASE))
/**
 * Impements a simple incomplete version of gettimeofday. Only usefull for messuring
 * time spans, not the real time of day.
 */
int gettimeofday(struct timeval *time, void *local) {
    int milliseconds = GetTickCount();

    time->tv_sec = milliseconds/1000;
    time->tv_usec = (milliseconds - (time->tv_sec * 1000)) * 1000;

    return 0;
}
#endif
/**
 * @brief Converts an ISO 8601-style time string into epoch time.
 *
 * @param iso8601 Time in ISO 8601 format.
 *
 * @return The number of seconds elapsed since January 1, 1970, 00:00:00 UTC.
 */
unsigned int iso8601_to_secs(char *iso8601) {
    int a,b,d,val[6],i=0;
    char *start=iso8601,*pos=iso8601;
    while (*pos && i < 6) {
        if (*pos < '0' || *pos > '9') {
            val[i++]=atoi(start);
            pos++;
            start=pos;
        }
        if(*pos)
            pos++;
    }

    a=val[0]/100;
    b=2-a+a/4;

    if (val[1] < 2) {
        val[0]--;
        val[1]+=12;
    }

    d=1461*(val[0]+4716)/4+306001*(val[1]+1)/10000+val[2]+b-2442112;

    return ((d*24+val[3])*60+val[4])*60+val[5];
}

/**
 * @brief Converts a `tm` structure to `time_t`
 *
 * Returns the value of type `time_t` that represents the UTC time described by the `tm` structure
 * pointed to by `pt` (which may be modified).
 *
 * This function performs the reverse translation that `gmtime()` does. As this functionality is absent
 * in the standard library, it is emulated by calling `mktime()`, converting its output into both GMT
 * and local time, comparing the results and calling `mktime()` again with an input adjusted for the
 * offset in the opposite direction. This ensures maximum portability.
 *
 * The values of the `tm_wday` and `tm_yday` members of `pt` are ignored, and the values of the other
 * members are interpreted even if out of their valid ranges (see `struct tm`). For example, `tm_mday`
 * may contain values above 31, which are interpreted accordingly as the days that follow the last day
 * of the selected month.
 *
 * A call to this function automatically adjusts the values of the members of `pt` if they are off-range
 * or—in the case of `tm_wday` and `tm_yday`—if their values are inconsistent with the other members.
 *
 */
time_t mkgmtime(struct tm * pt) {
    time_t ret;

    /* Input, GMT and local time */
    struct tm * pti, * pgt, * plt;

    pti = g_memdup(pt, sizeof(struct tm));

    ret = mktime(pti);

    pgt = g_memdup(gmtime(&ret), sizeof(struct tm));
    plt = g_memdup(localtime(&ret), sizeof(struct tm));

    pti->tm_year = pt->tm_year - pgt->tm_year + plt->tm_year;
    pti->tm_mon = pt->tm_mon - pgt->tm_mon + plt->tm_mon;
    pti->tm_mday = pt->tm_mday - pgt->tm_mday + plt->tm_mday;
    pti->tm_hour = pt->tm_hour - pgt->tm_hour + plt->tm_hour;
    pti->tm_min = pt->tm_min - pgt->tm_min + plt->tm_min;
    pti->tm_sec = pt->tm_sec - pgt->tm_sec + plt->tm_sec;

    ret = mktime(pti);

    dbg(lvl_debug, "time %ld (%02d-%02d-%02d %02d:%02d:%02d)\n", ret, pti->tm_year, pti->tm_mon, pti->tm_mday,
        pti->tm_hour, pti->tm_min, pti->tm_sec);

    g_free(pti);
    g_free(pgt);
    g_free(plt);

    return ret;
}

/**
 * @brief Converts an ISO 8601-style time string into `time_t`.
 */
time_t iso8601_to_time(char * iso8601) {
    /* Date/time fields (YYYY-MM-DD-hh-mm-ss) */
    int val[8];

    int i = 0;

    /* Start of next integer portion and current position */
    char *start = iso8601, *pos = iso8601;

    /* Time struct */
    struct tm tm;

    memset(&tm, 0, sizeof(struct tm));

    while (*pos && i < 6) {
        if (*pos < '0' || *pos > '9') {
            val[i++] = atoi(start);
            if (i == 6)
                break;
            pos++;
            start = pos;
        }
        if (*pos)
            pos++;
    }
    val[6] = 0;
    val[7] = 0;
    if (*pos && i == 6) {
        if (pos[1] && pos[2] && (!pos[3] || pos[3] == ':')) {
            val[6] = atoi(pos);
            if (pos[3] == ':') {
                pos += 3;
                val[7] = (val[6] < 0) ? -atoi(pos) : atoi(pos);
            }
        } else if (pos[1] && pos[2] && pos[3] && pos[4]) {
            val[6] = atoi(pos) / 100;
            val[7] = atoi(pos) % 100;
        }
    }

    tm.tm_year = val[0] - 1900;
    tm.tm_mon = val[1] - 1;
    tm.tm_mday = val[2];
    tm.tm_hour = val[3] - val[6];
    tm.tm_min = val[4] - val[7];
    tm.tm_sec = val[5];

    dbg(lvl_debug, "time %s (%02d-%02d-%02d %02d:%02d:%02d)\n", iso8601, tm.tm_year, tm.tm_mon, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec);

    return mkgmtime(&tm);
}

/**
 * @brief Converts time to ISO8601 format.
 *
 * The caller is responsible for freeing the return value of this function when it is no longer needed.
 *
 * @param time The time, as returned by `time()` and related functions
 *
 * @return Time in ISO8601 format
 */
char * time_to_iso8601(time_t time) {
    char *timep=NULL;
    char buffer[32];
    struct tm *tm;

    tm = gmtime(&time);
    if (tm) {
        strftime(buffer, sizeof(buffer), "%Y-%m-%dT%TZ", tm);
        timep=g_strdup(buffer);
    }
    return timep;
}

/**
 * @brief Outputs local system time in ISO 8601 format.
 *
 * @return Time in ISO 8601 format
 */
char *current_to_iso8601(void) {
#ifdef HAVE_API_WIN32_BASE
    char *timep=NULL;
    SYSTEMTIME ST;
    GetSystemTime(&ST);
    timep=g_strdup_printf("%d-%02d-%02dT%02d:%02d:%02dZ",ST.wYear,ST.wMonth,ST.wDay,ST.wHour,ST.wMinute,ST.wSecond);
    return timep;
#else
    time_t tnow;
    tnow = time(0);
    return time_to_iso8601(tnow);
#endif
}


struct spawn_process_info {
#ifdef HAVE_API_WIN32_BASE
    PROCESS_INFORMATION pr;
#else
    pid_t pid; // = -1 if non-blocking spawn isn't supported
    int status; // exit status if non-blocking spawn isn't supported
#endif
};


/**
 * Escape and quote string for shell
 *
 * @param in arg string to escape
 * @returns escaped string
 */
char *shell_escape(char *arg) {
    char *r;
    int arglen=strlen(arg);
    int i,j,rlen;
#ifdef HAVE_API_WIN32_BASE
    {
        int bscount=0;
        rlen=arglen+3;
        r=g_new(char,rlen);
        r[0]='"';
        for(i=0,j=1; i<arglen; i++) {
            if(arg[i]=='\\') {
                bscount++;
                if(i==(arglen-1)) {
                    // Most special case - last char is
                    // backslash. We can't escape it inside
                    // quoted string due to Win unescaping
                    // rules so quote should be closed
                    // before backslashes and these
                    // backslashes shouldn't be doubled
                    rlen+=bscount;
                    r=g_realloc(r,rlen);
                    r[j++]='"';
                    memset(r+j,'\\',bscount);
                    j+=bscount;
                }
            } else {
                //Any preceeding backslashes will be doubled.
                bscount*=2;
                // Double quote needs to be preceeded by
                // at least one backslash
                if(arg[i]=='"')
                    bscount++;
                if(bscount>0) {
                    rlen+=bscount;
                    r=g_realloc(r,rlen);
                    memset(r+j,'\\',bscount);
                    j+=bscount;
                    bscount=0;
                }
                r[j++]=arg[i];
                if(i==(arglen-1)) {
                    r[j++]='"';
                }
            }
        }
        r[j++]=0;
    }
#else
    {
        // Will use hard quoting for the whole string
        // and replace each singular quote found with a '\'' sequence.
        rlen=arglen+3;
        r=g_new(char,rlen);
        r[0]='\'';
        for(i=0,j=1; i<arglen; i++) {
            if(arg[i]=='\'') {
                rlen+=3;
                r=g_realloc(r,rlen);
                g_strlcpy(r+j,"'\\''",rlen-j);
            } else {
                r[j++]=arg[i];
            }
        }
        r[j++]='\'';
        r[j++]=0;
    }
#endif
    return r;
}

#ifndef _POSIX_C_SOURCE
static char* spawn_process_compose_cmdline(char **argv) {
    int i,j;
    char *cmdline=shell_escape(argv[0]);
    for(i=1,j=strlen(cmdline); argv[i]; i++) {
        char *arg=shell_escape(argv[i]);
        int arglen=strlen(arg);
        cmdline[j]=' ';
        cmdline=g_realloc(cmdline,j+1+arglen+1);
        memcpy(cmdline+j+1,arg,arglen+1);
        g_free(arg);
        j=j+1+arglen;
    }
    return cmdline;
}
#endif

#ifdef _POSIX_C_SOURCE

#if 0 /* def _POSIX_THREADS */
#define spawn_process_sigmask(how,set,old) pthread_sigmask(how,set,old)
#else
#define spawn_process_sigmask(how,set,old) sigprocmask(how,set,old)
#endif

GList *spawn_process_children=NULL;

#endif


/**
 * Call external program
 *
 * @param in argv NULL terminated list of parameters,
 *    zeroeth argument is program name
 * @returns 0 - success, >0 - return code, -1 - error
 */
struct spawn_process_info*
spawn_process(char **argv) {
    struct spawn_process_info*r=g_new(struct spawn_process_info,1);
#ifdef _POSIX_C_SOURCE
    {
        pid_t pid;

        sigset_t set, old;
        dbg(lvl_debug,"spawning process for '%s'", argv[0]);
        sigemptyset(&set);
        sigaddset(&set,SIGCHLD);
        spawn_process_sigmask(SIG_BLOCK,&set,&old);
        pid=fork();
        if(pid==0) {
            execvp(argv[0], argv);
            /*Shouldn't reach here*/
            exit(1);
        } else if(pid>0) {
            r->status=-1;
            r->pid=pid;
            spawn_process_children=g_list_prepend(spawn_process_children,r);
        } else {
            dbg(lvl_error,"fork() returned error.");
            g_free(r);
            r=NULL;
        }
        spawn_process_sigmask(SIG_SETMASK,&old,NULL);
        return r;
    }
#else
#ifdef HAVE_API_WIN32_BASE
    {
        char *cmdline;
        DWORD dwRet;

        // For [desktop] Windows it's adviceable not to use
        // first CreateProcess parameter because PATH is not used
        // if it is defined.
        //
        // On WinCE 6.0 I was unable to launch anything
        // without first CreateProcess parameter, also it seems that
        // no WinCE program has support for quoted strings in arguments.
        // So...
#ifdef HAVE_API_WIN32_CE
        LPWSTR cmd,args;
        cmdline=g_strjoinv(" ",argv+1);
        args=newSysString(cmdline);
        cmd = newSysString(argv[0]);
        dwRet=CreateProcess(cmd, args, NULL, NULL, 0, 0, NULL, NULL, NULL, &(r->pr));
        dbg(lvl_debug, "CreateProcess(%s,%s), PID=%i",argv[0],cmdline,r->pr.dwProcessId);
        g_free(cmd);
#else
        TCHAR* args;
        STARTUPINFO startupInfo;
        memset(&startupInfo, 0, sizeof(startupInfo));
        startupInfo.cb = sizeof(startupInfo);
        cmdline=spawn_process_compose_cmdline(argv);
        args=newSysString(cmdline);
        dwRet=CreateProcess(NULL, args, NULL, NULL, 0, 0, NULL, NULL, &startupInfo, &(r->pr));
        dbg(lvl_debug, "CreateProcess(%s), PID=%i",cmdline,r->pr.dwProcessId);
#endif
        g_free(cmdline);
        g_free(args);
        return r;
    }
#else
    {
        char *cmdline=spawn_process_compose_cmdline(argv);
        int status;
        dbg(lvl_error,"Unblocked spawn_process isn't availiable on this platform.");
        status=system(cmdline);
        g_free(cmdline);
        r->status=status;
        r->pid=0;
        return r;
    }
#endif
#endif
}

/**
 * Check external program status
 *
 * @param in *pi pointer to spawn_process_info structure
 * @param in block =0 do not block =1 block until child terminated
 * @returns -1 - still running, >=0 program exited,
 *     =255 trminated abnormally or wasn't run at all.
 *
 */
int spawn_process_check_status(struct spawn_process_info *pi, int block) {
    if(pi==NULL) {
        dbg(lvl_error,"Trying to get process status of NULL, assuming process is terminated.");
        return 255;
    }
#ifdef HAVE_API_WIN32_BASE
    {
        int failcount=0;
        while(1) {
            DWORD dw;
            if(GetExitCodeProcess(pi->pr.hProcess,&dw)) {
                if(dw!=STILL_ACTIVE) {
                    return dw;
                    break;
                }
            } else {
                dbg(lvl_error,"GetExitCodeProcess failed. Assuming the process is terminated.");
                return 255;
            }
            if(!block)
                return -1;

            dw=WaitForSingleObject(pi->pr.hProcess,INFINITE);
            if(dw==WAIT_FAILED && failcount++==1) {
                dbg(lvl_error,"WaitForSingleObject failed twice. Assuming the process is terminated.");
                return 0;
                break;
            }
        }
    }
#else
#ifdef _POSIX_C_SOURCE
    if(pi->status!=-1) {
        return pi->status;
    }
    while(1) {
        int status;
        pid_t w=waitpid(pi->pid,&status,block?0:WNOHANG);
        if(w>0) {
            if(WIFEXITED(status))
                pi->status=WEXITSTATUS(status);
            return pi->status;
            if(WIFSTOPPED(status)) {
                dbg(lvl_debug,"child is stopped by %i signal",WSTOPSIG(status));
            } else if (WIFSIGNALED(status)) {
                dbg(lvl_debug,"child terminated by signal %i",WEXITSTATUS(status));
                pi->status=255;
                return 255;
            }
            if(!block)
                return -1;
        } else if(w==0) {
            if(!block)
                return -1;
        } else {
            if(pi->status!=-1) // Signal handler has changed pi->status while in this function
                return pi->status;
            dbg(lvl_error,"waitpid() indicated error, reporting process termination.");
            return 255;
        }
    }
#else
    dbg(lvl_error, "Non-blocking spawn_process isn't availiable for this platform, repoting process exit status.");
    return pi->status;
#endif
#endif
}

void spawn_process_info_free(struct spawn_process_info *pi) {
    if(pi==NULL)
        return;
#ifdef HAVE_API_WIN32_BASE
    CloseHandle(pi->pr.hProcess);
    CloseHandle(pi->pr.hThread);
#endif
#ifdef _POSIX_C_SOURCE
    {
        sigset_t set, old;
        sigemptyset(&set);
        sigaddset(&set,SIGCHLD);
        spawn_process_sigmask(SIG_BLOCK,&set,&old);
        spawn_process_children=g_list_remove(spawn_process_children,pi);
        spawn_process_sigmask(SIG_SETMASK,&old,NULL);
    }
#endif
    g_free(pi);
}

#ifdef _POSIX_C_SOURCE
static void spawn_process_sigchld(int sig) {
    int status;
    pid_t pid;
    while ((pid=waitpid(-1, &status, WNOHANG)) > 0) {
        GList *el=g_list_first(spawn_process_children);
        while(el) {
            struct spawn_process_info *p=el->data;
            if(p->pid==pid) {
                p->status=status;
            }
            el=g_list_next(el);
        }
    }
}
#endif

void spawn_process_init() {
#ifdef _POSIX_C_SOURCE
    struct sigaction act;
    act.sa_handler=spawn_process_sigchld;
    act.sa_flags=0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGCHLD, &act, NULL);
#endif
    return;
}

/**
 * @brief Get printable compass direction from an angle.
 *
 * This function supports three different modes:
 *
 * In mode 0, the angle in degrees is output as a string.
 *
 * In mode 1, the angle is output as a cardinal direction (N, SE etc.).
 *
 * In mode 2, the angle is output in analog clock notation (6 o'clock).
 *
 * @param buffer Buffer to hold the result string (up to 5 characters, including the terminating null
 * character, may be required)
 * @param angle The angle to convert
 * @param mode The conversion mode, see description
 */
void get_compass_direction(char *buffer, int angle, int mode) {
    angle=angle%360;
    switch (mode) {
    case 0:
        sprintf(buffer,"%d",angle);
        break;
    case 1:
        if (angle < 69 || angle > 291)
            *buffer++='N';
        if (angle > 111 && angle < 249)
            *buffer++='S';
        if (angle > 22 && angle < 158)
            *buffer++='E';
        if (angle > 202 && angle < 338)
            *buffer++='W';
        *buffer++='\0';
        break;
    case 2:
        angle=(angle+15)/30;
        if (! angle)
            angle=12;
        sprintf(buffer,"%d H", angle);
        break;
    }
}

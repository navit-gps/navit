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

#include <glib.h>
#include <ctype.h>
#include <stdarg.h>
#include "util.h"

void
strtoupper(char *dest, const char *src)
{
	while (*src)
		*dest++=toupper(*src++);
	*dest='\0';
}

void
strtolower(char *dest, const char *src)
{
	while (*src)
		*dest++=tolower(*src++);
	*dest='\0';
}


static void
hash_callback(gpointer key, gpointer value, gpointer user_data)
{
	GList **l=user_data;
	*l=g_list_prepend(*l, value);
}

GList *
g_hash_to_list(GHashTable *h)
{
	GList *ret=NULL;
	g_hash_table_foreach(h, hash_callback, &ret);

	return ret;
}

gchar *
g_strconcat_printf(gchar *buffer, gchar *fmt, ...)
{
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
int
g_utf8_strlen_force_link(gchar *buffer, int max)
{
	return g_utf8_strlen(buffer, max);
}
#endif

#if defined(_WIN32) || defined(__CEGCC__)
#include <windows.h>
#include <stdio.h>
char *stristr(const char *String, const char *Pattern)
{
      char *pptr, *sptr, *start;

      for (start = (char *)String; *start != (int)NULL; start++)
      {
            /* find start of pattern in string */
            for ( ; ((*start!=(int)NULL) && (toupper(*start) != toupper(*Pattern))); start++)
                  ;
            if ((int)NULL == *start)
                  return NULL;

            pptr = (char *)Pattern;
            sptr = (char *)start;

            while (toupper(*sptr) == toupper(*pptr))
            {
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

/* Read up to (and including) a DELIMITER from FP into *LINEPTR (and
   NUL-terminate it).  *LINEPTR is a pointer returned from malloc (or
   NULL), pointing to *N characters of space.  It is realloc'ed as
   necessary.  Returns the number of characters read (not including
   the null terminator), or -1 on error or EOF.  */

int
getdelim (char **lineptr, size_t *n, int delimiter, FILE *fp)
{
  int result;
  size_t cur_len = 0;

  if (lineptr == NULL || n == NULL || fp == NULL)
    {
      return -1;
    }

  flockfile (fp);

  if (*lineptr == NULL || *n == 0)
    {
      *n = 120;
      *lineptr = (char *) realloc (*lineptr, *n);
      if (*lineptr == NULL)
	{
	  result = -1;
	  goto unlock_return;
	}
    }

  for (;;)
    {
      int i;

      i = getc (fp);
      if (i == EOF)
	{
	  result = -1;
	  break;
	}

      /* Make enough space for len+1 (for final NUL) bytes.  */
      if (cur_len + 1 >= *n)
	{
	  size_t needed_max =
	    SSIZE_MAX < SIZE_MAX ? (size_t) SSIZE_MAX + 1 : SIZE_MAX;
	  size_t needed = 2 * *n + 1;   /* Be generous. */
	  char *new_lineptr;

	  if (needed_max < needed)
	    needed = needed_max;
	  if (cur_len + 1 >= needed)
	    {
	      result = -1;
	      goto unlock_return;
	    }

	  new_lineptr = (char *) realloc (*lineptr, needed);
	  if (new_lineptr == NULL)
	    {
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

int
getline (char **lineptr, size_t *n, FILE *stream)
{
  return getdelim (lineptr, n, '\n', stream);
}

#if defined(_UNICODE)
wchar_t* newSysString(const char *toconvert)
{
	int newstrlen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, toconvert, -1, 0, 0);
	wchar_t *newstring = g_new(wchar_t,newstrlen);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, toconvert, -1, newstring, newstrlen) ;
	return newstring;
}
#else
char * newSysString(const char *toconvert)
{
	return g_strdup(toconvert);
}
#endif
#endif

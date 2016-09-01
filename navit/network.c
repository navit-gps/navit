#include "curl/curl.h"
#include <navit/debug.h>
#include "unistd.h"
#include "stddef.h"

struct string
{
  char *ptr;
  size_t len;
};

void
init_string (struct string *s)
{
  s->len = 0;
  s->ptr = malloc (s->len + 1);
  if (s->ptr == NULL)
    {
      dbg(lvl_error, "malloc() failed\n");
    }
  s->ptr[0] = '\0';
}

size_t
writefunc (void *ptr, size_t size, size_t nmemb, struct string *s)
{
  size_t new_len = s->len + size * nmemb;
  s->ptr = realloc (s->ptr, new_len + 1);
  if (s->ptr == NULL)
    {
      dbg (lvl_error, "realloc() failed\n");
    }
  memcpy (s->ptr + s->len, ptr, size * nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size * nmemb;
}

size_t
write_data (void *ptr, size_t size, size_t nmemb, FILE * stream)
{
  size_t written = fwrite (ptr, size, nmemb, stream);
  return written;
}

// From http://creativeandcritical.net/str-replace-c/
char *replace_str(const char *str, const char *old, const char *new)
{
	char *ret, *r;
	const char *p, *q;
	size_t oldlen = strlen(old);
	size_t count, retlen, newlen = strlen(new);

	if (oldlen != newlen) {
		for (count = 0, p = str; (q = strstr(p, old)) != NULL; p = q + oldlen)
			count++;
		/* this is undefined if p - str > PTRDIFF_MAX */
		retlen = p - str + strlen(p) + count * (newlen - oldlen);
	} else
		retlen = strlen(str);

	if ((ret = malloc(retlen + 1)) == NULL)
		return NULL;

	for (r = ret, p = str; (q = strstr(p, old)) != NULL; p = q + oldlen) {
		/* this is undefined if q - p > PTRDIFF_MAX */
		ptrdiff_t l = q - p;
		memcpy(r, p, l);
		r += l;
		memcpy(r, new, newlen);
		r += newlen;
	}
	strcpy(r, p);

	return ret;
}


char *
fetch_url_to_string(char * url)
{
  struct string s;
  char * sanitized_url=replace_str(url," ","%20");
  init_string (&s);
  CURL *curl;
  curl_global_init (CURL_GLOBAL_ALL);
  curl = curl_easy_init ();
  curl_easy_setopt (curl, CURLOPT_URL, sanitized_url);
  curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, writefunc);
  curl_easy_setopt (curl, CURLOPT_WRITEDATA, &s);
  curl_easy_perform (curl);
  curl_easy_cleanup (curl);
  dbg (lvl_debug, "url %s gave %s\n", sanitized_url, s.ptr);
  dbg (lvl_info, "url %s gave js of size %i\n", sanitized_url, strlen(s.ptr));
  free(sanitized_url);
  return strdup(s.ptr);
  // FIXME : we have a memleak here
  // js = malloc(sizeof(char)*(strlen(s.ptr)+1));
  // js="{}";
  // dbg(0,"copied js is %s\n",js);
  // free (s.ptr);
  // return 0;
}

char *
post_to_string(char * url)
{
  struct string s;
  init_string (&s);
  CURL *curl;
  curl_global_init (CURL_GLOBAL_ALL);
  curl = curl_easy_init ();
  curl_easy_setopt (curl, CURLOPT_VERBOSE, 1);
  curl_easy_setopt (curl, CURLOPT_URL, url);
  curl_easy_setopt (curl, CURLOPT_POST, 1);
  curl_easy_setopt (curl, CURLOPT_POSTFIELDS, url);
  curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, writefunc);
  curl_easy_setopt (curl, CURLOPT_WRITEDATA, &s);
  curl_easy_perform (curl);
  // FIXME : memleak
  free (s.ptr);
  curl_easy_cleanup (curl);
  return g_strdup(s.ptr);
}

int
download_icon (char *prefix, char *img_size, char *suffix)
{
  CURL *curl;
  FILE *fp;
  CURLcode res;
  strcat (prefix, img_size);
  char filename[512];

  static char *navit_sharedir;
  navit_sharedir = getenv ("NAVIT_SHAREDIR");

  // FIXME : use g_strdup_printf more widely

  strcpy (filename, g_strdup_printf ("%s/xpm/", navit_sharedir));
  strcat (filename, strrchr (prefix, '/') + 1);

  strcat (filename, "_");
  strcat (filename, img_size);
  strcat (filename, suffix);

  char icon_url[512];
  strcpy (icon_url, prefix);
  strcat (icon_url, suffix);

  dbg (lvl_info, "About to download %s to %s\n", icon_url, filename);
  // FIXME : switch to navit's code to check for icon presence
  if (access (filename, F_OK) != -1)
    {
      dbg (lvl_info, "We have %s in cache already, skipping download\n", filename);
    }
  else
    {
      curl = curl_easy_init ();
      if (curl)
        {
          fp = fopen (filename, "wb");
          curl_easy_setopt (curl, CURLOPT_URL, icon_url);
          curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, write_data);
          curl_easy_setopt (curl, CURLOPT_WRITEDATA, fp);
          res = curl_easy_perform (curl);
          curl_easy_cleanup (curl);
          fclose (fp);
        }
    }
  return 0;
}

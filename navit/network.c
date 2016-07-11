#include "curl/curl.h"
#include <navit/debug.h>
#include "unistd.h"
#include "stddef.h"

struct string {
	char *ptr;
	size_t len;
};

 /**
 * @brief Initialize a "string" object
 *
 * @param string the struct string pointer
 * @returns nothing
 */
void
init_string(struct string *s)
{
	s->len = 0;
	s->ptr = malloc(s->len + 1);
	if (s->ptr == NULL) {
		dbg(lvl_error, "malloc() failed\n");
	}
	s->ptr[0] = '\0';
}

 /**
 * @brief store the incoming data into a dynamically
 *  growing allocated buffer. Typical when using libcurl.
 *
 * @param ptr pointer to the delivered data
 * @param size size of the delivered data
 * @param nmemb number of mem blocks
 * @param s CURLOPT_WRITEDATA option
 * @returns the size of the buffer after expansion
 */
size_t
writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
	size_t new_len = s->len + size * nmemb;
	s->ptr = realloc(s->ptr, new_len + 1);
	if (s->ptr == NULL) {
		dbg(lvl_error, "realloc() failed\n");
	}
	memcpy(s->ptr + s->len, ptr, size * nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;

	return size * nmemb;
}

 /**
 * @brief Replace all occurrences of the search string with the replacement string
 * From http://creativeandcritical.net/str-replace-c/
 *
 * @param str the string to replace in
 * @param old the content to replace
 * @param new the new content
 * @returns the updated string
 */
char *
replace_str(const char *str, const char *old, const char *new)
{
	char *ret, *r;
	const char *p, *q;
	size_t oldlen = strlen(old);
	size_t count, retlen, newlen = strlen(new);

	if (oldlen != newlen) {
		for (count = 0, p = str; (q = strstr(p, old)) != NULL;
		     p = q + oldlen)
			count++;
		/* this is undefined if p - str > PTRDIFF_MAX */
		retlen = p - str + strlen(p) + count * (newlen - oldlen);
	} else
		retlen = strlen(str);

	if ((ret = malloc(retlen + 1)) == NULL)
		return NULL;

	for (r = ret, p = str; (q = strstr(p, old)) != NULL;
	     p = q + oldlen) {
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


 /**
 * @brief fetch the content from a url
 *
 * @param url the url to fetch content from
 * @returns the content of the webpage at the given url
 */
char *
fetch_url_to_string(char *url)
{
	struct string s;
	char *sanitized_url = replace_str(url, " ", "%20");
	init_string(&s);
	CURL *curl;
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, sanitized_url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	dbg(lvl_info, "url %s gave %s\n", sanitized_url, s.ptr);
	dbg(lvl_error, "url %s gave js of size %i\n", sanitized_url,
	    strlen(s.ptr));
	free(sanitized_url);
	return strdup(s.ptr);
}

 /**
 * @brief perform a http/post and returns the result
 *
 * @param url the url, including the encoded parameters
 * @returns the html response
 */
char *
post_to_string(char *url)
{
	struct string s;
	init_string(&s);
	CURL *curl;
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
	curl_easy_perform(curl);
	free(s.ptr);
	curl_easy_cleanup(curl);
	return g_strdup(s.ptr);
}

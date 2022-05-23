#ifndef _WORDEXP_H_
#define	_WORDEXP_H_


typedef struct {
	size_t	we_wordc;		/* count of words matched */
	char		**we_wordv;	/* pointer to list of words */
	size_t	we_offs;		/* slots to reserve in we_wordv */
					/* following are internals */
	char		*we_strings;	/* storage for wordv strings */
	size_t	we_nbytes;		/* size of we_strings */
} wordexp_t;

/*
 * Flags for wordexp().
 */
#define	WRDE_APPEND	0x1		/* append to previously generated */
#define	WRDE_DOOFFS	0x2		/* we_offs member is valid */
#define	WRDE_NOCMD	0x4		/* disallow command substitution */
#define	WRDE_REUSE	0x8		/* reuse wordexp_t */
#define	WRDE_SHOWERR	0x10		/* don't redirect stderr to /dev/null */
#define	WRDE_UNDEF	0x20		/* disallow undefined shell vars */

/*
 * Return values from wordexp().
 */
#define	WRDE_BADCHAR	1		/* unquoted special character */
#define	WRDE_BADVAL	2		/* undefined variable */
#define	WRDE_CMDSUB	3		/* command substitution not allowed */
#define	WRDE_NOSPACE	4		/* no memory for result */
#if (_XOPEN_SOURCE - 0) >= 4 || defined(_NETBSD_SOURCE)
#define	WRDE_NOSYS	5		/* obsolete, reserved */
#endif
#define	WRDE_SYNTAX	6		/* shell syntax error */
#define WRDE_ERRNO	7		/* other errors see errno */

void	wordfree(wordexp_t *);
int wordexp(const char * words, wordexp_t * we, int flags);


#endif /* !_WORDEXP_H_ */


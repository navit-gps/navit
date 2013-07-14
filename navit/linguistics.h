#ifdef __cplusplus
extern "C" {
#endif
#define LINGUISTICS_WORD_SEPARATORS_ASCII " -/()'`"
char *linguistics_expand_special(const char *str, int mode);
char *linguistics_next_word(char *str);
void linguistics_init(void);
void linguistics_free(void);
char *linguistics_casefold(const char *in);
int linguistics_search(const char *str);
enum linguistics_cmp_mode {
	linguistics_cmp_expand=1,
	linguistics_cmp_partial=2,
	linguistics_cmp_words=4
};
int linguistics_compare(const char *s1, const char *s2, enum linguistics_cmp_mode mode);
#ifdef __cplusplus
}
#endif



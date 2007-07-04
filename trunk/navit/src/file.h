#include "param.h"

struct file {
        unsigned char *begin;
        unsigned char *end;
        unsigned long size;
	char *name;
	void *private;
	int fd;
	struct file *next;
};
 
/* prototypes */
struct file;
struct file_wordexp;
struct param_list;
struct file *file_create(char *name);
int file_exists(char *name);
void file_remap_readonly(struct file *f);
void file_remap_readonly_all(void);
void file_unmap(struct file *f);
void file_unmap_all(void);
void *file_opendir(char *dir);
char *file_readdir(void *hnd);
void file_closedir(void *hnd);
struct file *file_create_caseinsensitive(char *name);
void file_destroy(struct file *f);
struct file_wordexp *file_wordexp_new(char *pattern);
int file_wordexp_get_count(struct file_wordexp *wexp);
char **file_wordexp_get_array(struct file_wordexp *wexp);
void file_wordexp_destroy(struct file_wordexp *wexp);
int file_get_param(struct file *file, struct param_list *param, int count);
/* end of prototypes */

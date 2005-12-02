#include "param.h"

struct file {
        unsigned char *begin;
        unsigned char *end;
        unsigned long size;
	char *name;
	void *private;
};
 
struct file *file_create(char *name);
void file_set_readonly(struct file *file);
struct file *file_create_caseinsensitive(char *name);
int file_get_param(struct file *file, struct param_list *param, int count);
void file_destroy(struct file *f);
void *file_opendir(char *dir);
char *file_readdir(void *hnd);
void file_closedir(void *hnd);

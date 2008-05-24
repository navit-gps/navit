#ifndef NAVIT_FILE_H
#define NAVIT_FILE_H

#include "param.h"

struct file {
	unsigned char *begin;
	unsigned char *end;
	long long size;
	char *name;
	int fd;
#ifdef _WIN32
    long map_handle;
    long map_file;
#endif
	struct file *next;
};

/* prototypes */
struct file;
struct file_wordexp;
struct param_list;
struct file *file_create(char *name);
int file_is_dir(char *name);
int file_mkdir(char *name, int pflag);
int file_mmap(struct file *file);
unsigned char *file_data_read(struct file *file, long long offset, int size);
unsigned char *file_data_read_compressed(struct file *file, long long offset, int size, int size_uncomp);
void file_data_free(struct file *file, unsigned char *data);
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
struct file_wordexp *file_wordexp_new(const char *pattern);
int file_wordexp_get_count(struct file_wordexp *wexp);
char **file_wordexp_get_array(struct file_wordexp *wexp);
void file_wordexp_destroy(struct file_wordexp *wexp);
int file_get_param(struct file *file, struct param_list *param, int count);
/* end of prototypes */

#endif


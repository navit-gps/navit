#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <stdio.h>
#include <glib.h>
#include "file.h"

static struct file *file_list;

struct file *
file_create(char *name)
{
        struct stat stat;
	struct file *file=malloc(sizeof(*file)+strlen(name)+1);

	if (! file)
		return file; 
        file->fd=open(name, O_RDONLY);
	if (file->fd < 0) {
		free(file);
		return NULL;
	}
        fstat(file->fd, &stat);
        file->size=stat.st_size;
	file->name=(char *)file+sizeof(*file);
	strcpy(file->name, name);
        file->begin=mmap(NULL, file->size, PROT_READ|PROT_WRITE, MAP_PRIVATE, file->fd, 0);
	g_assert(file->begin != NULL);
	if (file->begin == (void *)0xffffffff) {
		perror("mmap");
	}
	g_assert(file->begin != (void *)0xffffffff);
        file->end=file->begin+file->size;
	file->private=NULL;

	g_assert(file != NULL); 
	file->next=file_list;
	file_list=file;
        return file;
}

void
file_remap_readonly(struct file *f)
{
	void *begin;
	munmap(f->begin, f->size);
        begin=mmap(f->begin, f->size, PROT_READ, MAP_PRIVATE, f->fd, 0);
	if (f->begin != begin)
		printf("remap failed\n");
}

void
file_remap_readonly_all(void)
{
	struct file *f=file_list;

	while (f) {
		file_remap_readonly(f);
		f=f->next;
	}
}

void *
file_opendir(char *dir)
{
	return opendir(dir);
}

char *
file_readdir(void *hnd)
{
	struct dirent *ent;

	ent=readdir(hnd);
	if (! ent)
		return NULL;
	return ent->d_name;
}

void
file_closedir(void *hnd)
{
	closedir(hnd);
}

struct file *
file_create_caseinsensitive(char *name)
{
	char dirname[strlen(name)+1];
	char *filename;
	char *p;
	void *d;
	struct file *ret;

	ret=file_create(name);
	if (ret)
		return ret;

	strcpy(dirname, name);
	p=dirname+strlen(name);
	while (p > dirname) {
		if (*p == '/')
			break;
		p--;
	}
	*p=0;
	d=file_opendir(dirname);
	if (d) {
		*p++='/';
		while ((filename=file_readdir(d))) {
			if (!strcasecmp(filename, p)) {
				strcpy(p, filename);
				ret=file_create(dirname);
				if (ret)
					break;
			}
		}
		file_closedir(d);
	}
	return ret;
}

void
file_destroy(struct file *f)
{
        close(f->fd);
	munmap(f->begin, f->size);
	free(f);	
}

int
file_get_param(struct file *file, struct param_list *param, int count)
{
	int i=count;
	param_add_string("Filename", file->name, &param, &count);
	param_add_hex("Size", file->size, &param, &count);
	return i-count;
}

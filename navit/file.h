/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef NAVIT_FILE_H
#define NAVIT_FILE_H

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#ifndef __CEGCC__
#include <time.h>
#endif
#include "param.h"

struct file {
	unsigned char *begin;
	unsigned char *end;
	long long size;
	char *name;
	int name_id;
	int fd;
#ifndef __CEGCC__
	time_t mtime;
	time_t ctime;
	int version;			
#endif
#if defined(_WIN32) || defined(__CEGCC__)
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
void file_unmap(struct file *f);
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
int file_version(struct file *file, int byname);
void *file_get_os_handle(struct file *file);
void file_init(void);
/* end of prototypes */

#endif


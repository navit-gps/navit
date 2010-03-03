/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2010 Navit Team
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
#include <stdlib.h>
#include <unistd.h>
#include "file.h"
#include "debug.h"
#include "projection.h"
#include "coord.h"
#include "transform.h"
#include "callback.h"
#include "map.h"
#include "bookmarks.h"

struct bookmarks {
	//data storage
	struct map *bookmark;
	GHashTable *bookmarks_hash;

	//Refs to other objects
	struct transformation *trans;
	struct attr **attrs;
	struct callback_list *attr_cbl;
	struct attr *parent;
};

struct bookmarks *bookmarks_new(struct attr *parent, /*struct attr **attrs,*/struct transformation *trans) {
	struct bookmarks *this_;

	this_ = g_new0(struct bookmarks,1);
	this_->attr_cbl=callback_list_new();
	this_->parent=parent;
	//this_->attrs=attr_list_dup(attrs);
	this_->trans=trans;

	this_->bookmarks_hash=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	return this_;
}

void bookmarks_destroy(struct bookmarks *this_) {
	map_destroy(this_->bookmark);
	g_hash_table_destroy(this_->bookmarks_hash);
	callback_list_destroy(this_->attr_cbl);
	g_free(this_);
}

struct map* bookmarks_get_map(struct bookmarks *this_) {
	return this_->bookmark;
}
/*
 * bookmarks_get_user_data_directory
 * 
 * returns the directory used to store user data files (center.txt,
 * destination.txt, bookmark.txt, ...)
 *
 * arg: gboolean create: create the directory if it does not exist
 */
char*
bookmarks_get_user_data_directory(gboolean create) {
	char *dir;
	dir = getenv("NAVIT_USER_DATADIR");
	if (create && !file_exists(dir)) {
		dbg(0,"creating dir %s\n", dir);
		if (file_mkdir(dir,0)) {
			dbg(0,"failed creating dir %s\n", dir);
			return NULL;
		}
	}

	return dir;
}

 /*
 * bookmarks_get_bookmark_file
 * 
 * returns the name of the file used to store bookmarks with its
 * full path
 *
 * arg: gboolean create: create the directory where the file is stored
 * if it does not exist
 */
char*
bookmarks_get_bookmark_file(gboolean create)
{
	return g_strjoin(NULL, bookmarks_get_user_data_directory(create), "/bookmark.txt", NULL);
}

/*
 * bookmarks_get_destination_file
 * 
 * returns the name of the file used to store destinations with its
 * full path
 *
 * arg: gboolean create: create the directory where the file is stored
 * if it does not exist
 */
char*
bookmarks_get_destination_file(gboolean create)
{
	return g_strjoin(NULL, bookmarks_get_user_data_directory(create), "/destination.txt", NULL);
}

/*
 * bookmarks_get_center_file
 * 
 * returns the name of the file used to store the center file  with its
 * full path
 *
 * arg: gboolean create: create the directory where the file is stored
 * if it does not exist
 */
char*
bookmarks_get_center_file(gboolean create)
{
	return g_strjoin(NULL, bookmarks_get_user_data_directory(create), "/center.txt", NULL);
}

void
bookmarks_set_center_from_file(struct bookmarks *this_, char *file)
{
#ifndef HAVE_API_ANDROID
	FILE *f;
	char *line = NULL;

	size_t line_size = 0;
	enum projection pro;
	struct coord *center;

	f = fopen(file, "r");
	if (! f)
		return;
	getline(&line, &line_size, f);
	fclose(f);
	if (line) {
		center = transform_center(this_->trans);
		pro = transform_get_projection(this_->trans);
		coord_parse(g_strchomp(line), pro, center);
		free(line);
	}
	return;
#endif
}
 
void
bookmarks_write_center_to_file(struct bookmarks *this_, char *file)
{
	FILE *f;
	enum projection pro;
	struct coord *center;

	f = fopen(file, "w+");
	if (f) {
		center = transform_center(this_->trans);
		pro = transform_get_projection(this_->trans);
		coord_print(pro, center, f);
		fclose(f);
	} else {
		perror(file);
	}
	return;
}

/**
 * Record the given set of coordinates as a bookmark
 *
 * @param navit The navit instance
 * @param c The coordinate to store
 * @param description A label which allows the user to later identify this bookmark
 * @returns nothing
 */
void
bookmarks_add_bookmark(struct bookmarks *this_, struct pcoord *c, const char *description)
{
	char *bookmark_file = bookmarks_get_bookmark_file(TRUE);
	bookmarks_append_coord(this_,bookmark_file, c, "bookmark", description, this_->bookmarks_hash,0);
	g_free(bookmark_file);

	callback_list_call_attr_0(this_->attr_cbl, attr_bookmark_map);
}

void
bookmarks_add_bookmarks_from_file(struct bookmarks *this_)
{
	char *bookmark_file = bookmarks_get_bookmark_file(FALSE);
	struct attr type={attr_type, {"textfile"}}, data={attr_data, {bookmark_file}};
	struct attr *attrs[]={&type, &data, NULL};

	this_->bookmark=map_new(this_->parent, attrs);
	g_free(bookmark_file);
}

/**
 * @param limit Limits the number of entries in the "backlog". Set to 0 for "infinite"
 */
void
bookmarks_append_coord(struct bookmarks *this_, char *file, struct pcoord *c, const char *type, const char *description, GHashTable *h, int limit)
{
	FILE *f;
	int offset=0;
	char *buffer;
	int ch,prev,lines=0;
	int numc,readc;
	int fd;
	const char *prostr;

	f=fopen(file, "r");
	if (!f)
		goto new_file;
	if (limit != 0) {
		prev = '\n';
		while ((ch = fgetc(f)) != EOF) {
			if ((ch == '\n') && (prev != '\n')) {
				lines++;
			}
			prev = ch;
		}

		if (prev != '\n') { // Last line did not end with a newline
			lines++;
		}

		fclose(f);
		f = fopen(file, "r+");
		fd = fileno(f);
		while (lines >= limit) { // We have to "scroll up"
			rewind(f);
			numc = 0; // Counts how many bytes we have in our line to scroll up
			while ((ch = fgetc(f)) != EOF) {
				numc++;
				if (ch == '\n') {
					break;
				}
			}

			buffer=g_malloc(numc);
			offset = numc; // Offset holds where we currently are
			
			do {
				fseek(f,offset,SEEK_SET);
				readc = fread(buffer,1,numc,f);
				
				fseek(f,-(numc+readc),SEEK_CUR);
				fwrite(buffer,1,readc,f);

				offset += readc;
			} while (readc == numc);

			g_free(buffer);
			fflush(f);
			ftruncate(fd,(offset-numc));
#ifdef HAVE_FSYNC
			fsync(fd);
#endif

			lines--;
		}
		fclose(f);
	}

new_file:
	f=fopen(file, "a");
	if (f) {
		if (c) {
			prostr = projection_to_name(c->pro,NULL);
			fprintf(f,"%s%s%s0x%x %s0x%x type=%s label=\"%s\"\n",
				 prostr, *prostr ? ":" : "", 
				 c->x >= 0 ? "":"-", c->x >= 0 ? c->x : -c->x, 
				 c->y >= 0 ? "":"-", c->y >= 0 ? c->y : -c->y, 
				 type, description);
		} else
			fprintf(f,"\n");
	}
	fclose(f);
}


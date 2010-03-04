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
	char* bookmark_file;
	char *working_file;

	//Refs to other objects
	struct transformation *trans;
	struct attr **attrs;
	struct callback_list *attr_cbl;
	struct attr *parent;
};

struct bookmarks *
bookmarks_new(struct attr *parent, /*struct attr **attrs,*/struct transformation *trans) {
	struct bookmarks *this_;

	this_ = g_new0(struct bookmarks,1);
	this_->attr_cbl=callback_list_new();
	this_->parent=parent;
	//this_->attrs=attr_list_dup(attrs);
	this_->trans=trans;

	this_->bookmarks_hash=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	this_->bookmark_file=g_strjoin(NULL, bookmarks_get_user_data_directory(TRUE), "/bookmark.txt", NULL);
	this_->working_file=g_strjoin(NULL, bookmarks_get_user_data_directory(TRUE), "/bookmark.txt.tmp", NULL);

	return this_;
}

void 
bookmarks_destroy(struct bookmarks *this_) {
	map_destroy(this_->bookmark);
	g_hash_table_destroy(this_->bookmarks_hash);
	callback_list_destroy(this_->attr_cbl);

	g_free(this_->bookmark_file);
	g_free(this_->working_file);

	g_free(this_);
}

struct map* 
bookmarks_get_map(struct bookmarks *this_) {
	return this_->bookmark;
}

void
bookmarks_add_callback(struct bookmarks *this_, struct callback *cb)
{
	callback_list_add(this_->attr_cbl, cb);
}

static int bookmarks_write_bookmark_to_file(FILE *f, struct coord c, const char *label) {
	const char *prostr;
	int result;

	prostr = projection_to_name(projection_mg,NULL);
	result=fprintf(f,"%s%s%s0x%x %s0x%x type=%s label=\"%s\"\n",
		 prostr, *prostr ? ":" : "", 
		 c.x >= 0 ? "":"-", c.x >= 0 ? c.x : -c.x, 
		 c.y >= 0 ? "":"-", c.y >= 0 ? c.y : -c.y, 
		 "bookmark", label);

	return result>0;
}

static int 
bookmarks_store_bookmarks_to_file(struct bookmarks *this_,  int limit,int replace) {
	FILE *f;
	struct item *item;
	struct map_rect *mr=NULL;
	struct attr attr;
	struct coord c;

	if (limit>0) {
		limit++; //We are 1 based, not zero
		//Calculate, how many entries we already have
		f=fopen(this_->working_file,"r");
		if (f!=NULL) {
			while (!feof(f) && limit>0) {
				char* tmpline=NULL;
				size_t tmpn;
				getline(&tmpline,&tmpn,f);
				limit--;
				free(tmpline);
			}
		}
		fclose(f);
	}

	mr=map_rect_new(this_->bookmark, NULL);
	if (mr==NULL) {
		return FALSE;
	}

	f=fopen(this_->working_file, replace ? "w+" : "a+");
	if (f==NULL) {
		return FALSE;
	}


	while ((item=map_rect_get_item(mr))) {
		if (item->type != type_bookmark) continue;
		item_attr_get(item, attr_label, &attr);
		item_coord_get(item, &c, 1);

		if (!bookmarks_write_bookmark_to_file(f,c,attr.u.str)) 
			break;

 		/* Limit could be zero, so we start decrementing it from zero and never reach 1
 		 or it was bigger and we decreased it earlier. So when this counter becomes 1, we know
		 that we have enough entries in bookmarks file */
		if (limit==1) {
			break;
		}
		limit--;
	}

	fclose(f);

	return rename(this_->working_file,this_->bookmark_file)==0;
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
int 
bookmarks_add_bookmark(struct bookmarks *this_, struct pcoord *pc, const char *description)
{
	FILE *f;
	int result;
	struct coord c;

	f=fopen(this_->working_file, "a+");
	if (f==NULL) {
		return FALSE;
	}
	c.x=pc->x;
	c.y=pc->y;
	result=bookmarks_write_bookmark_to_file(f, c, description);
	fclose(f);
	if (result) {
		bookmarks_store_bookmarks_to_file(this_,0,0);
	}

	callback_list_call_attr_0(this_->attr_cbl, attr_bookmark_map);

	return result;
}

void
bookmarks_add_bookmarks_from_file(struct bookmarks *this_)
{
	struct attr type={attr_type, {"textfile"}}, data={attr_data, {this_->bookmark_file}};
	struct attr *attrs[]={&type, &data, NULL};

	this_->bookmark=map_new(this_->parent, attrs);
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


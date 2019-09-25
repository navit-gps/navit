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

#include <stdlib.h>
#include <glib.h>
#include <stdio.h>
#include <errno.h>
#include "config.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "file.h"
#include "debug.h"
#include "projection.h"
#include "coord.h"
#include "transform.h"
#include "callback.h"
#include "map.h"
#include "command.h"
#include "bookmarks.h"
#include "navit.h"
#include "navit_nls.h"
#include "util.h"

/* FIXME: Move this to support directory */
#ifdef _MSC_VER
#include <windows.h>
static int ftruncate(int fd, __int64 length) {
    HANDLE fh = (HANDLE)_get_osfhandle(fd);
    if (!fh || _lseeki64(fd, length, SEEK_SET)) {
        return -1;
    }
    return SetEndOfFile(fh) ? 0 : -1;
}
#endif /* _MSC_VER */

struct bookmarks {
    //data storage
    struct map *bookmark;
    struct map_rect *mr;
    GHashTable *bookmarks_hash;
    GList *bookmarks_list;
    char* bookmark_file;
    char *working_file;
    struct bookmark_item_priv* clipboard;
    struct bookmark_item_priv* root;
    struct bookmark_item_priv* current;

    //Refs to other objects
    struct transformation *trans;
    struct attr **attrs;
    struct callback_list *attr_cbl;
    struct attr *parent;
};

struct bookmark_item_priv {
    char *label;
    enum item_type type;
    struct pcoord c;
    GList *children;
    GList *iter;
    struct bookmark_item_priv *parent;
    struct item item;
};

void bookmarks_move_root(struct bookmarks *this_) {
    this_->current=this_->root;
    this_->current->iter=g_list_first(this_->current->children);
    dbg(lvl_info,"Root list have %u entries",g_list_length(this_->current->children));
    return;
}
void bookmarks_move_up(struct bookmarks *this_) {
    if (this_->current->parent) {
        this_->current=this_->current->parent;
        this_->current->iter=this_->current->children;
    }
    return;
}
int bookmarks_move_down(struct bookmarks *this_,const char* name) {
    bookmarks_item_rewind(this_);
    if (this_->current->children==NULL) {
        return 0;
    }
    while (this_->current->iter!=NULL) {
        struct bookmark_item_priv* data=(struct bookmark_item_priv*)this_->current->iter->data;
        if (!strcmp(data->label,name)) {
            this_->current=(struct bookmark_item_priv*)this_->current->iter->data;
            this_->current->iter=g_list_first(this_->current->children);
            dbg(lvl_info,"%s list have %u entries",this_->current->label,g_list_length(this_->current->children));
            return 1;
        }
        this_->current->iter=g_list_next(this_->current->iter);
    }
    return 0;
}

void bookmarks_item_rewind(struct bookmarks* this_) {
    this_->current->children=g_list_first(this_->current->children);
    this_->current->iter=this_->current->children;
    this_->current->iter=this_->current->children;
}
struct item* bookmarks_get_item(struct bookmarks* this_) {
    struct item item,*ret;
    if (this_->current->iter==NULL) {
        return NULL;
    }

    item=((struct bookmark_item_priv*)this_->current->iter->data)->item;
    this_->current->iter=g_list_next(this_->current->iter);

    ret = map_rect_get_item_byid(this_->mr, item.id_hi, item.id_lo);

    return ret;
}

int bookmarks_get_bookmark_count(struct bookmarks* this_) {
    int ret=0;
    bookmarks_item_rewind(this_);
    while (this_->current->iter!=NULL) {
        struct bookmark_item_priv* data=(struct bookmark_item_priv*)this_->current->iter->data;
        if (data->type == type_bookmark) {
            ret++;
        }
        this_->current->iter=g_list_next(this_->current->iter);
    }
    return ret;
}

const char* bookmarks_item_cwd(struct bookmarks* this_) {
    return this_->current->label;
}

static void bookmarks_clear_item(struct bookmark_item_priv *b_item) {
    b_item->children=g_list_first(b_item->children);
    while(b_item->children) {
        bookmarks_clear_item((struct bookmark_item_priv*)b_item->children->data);
        b_item->children=g_list_next(b_item->children);
    }
    g_free(b_item->label);
    g_free(b_item);
}

static void bookmarks_clear_hash(struct bookmarks *this_) {
    if (this_->mr) {
        map_rect_destroy(this_->mr);
    }
    bookmarks_clear_item(this_->root);
    g_hash_table_destroy(this_->bookmarks_hash);
    g_list_free(this_->bookmarks_list);
}

static void bookmarks_load_hash(struct bookmarks *this_) {
    struct bookmark_item_priv *b_item;
    struct item *item;
    struct attr attr;
    struct coord c;
    char *pos,*finder;
    char *copy_helper;

    if (this_->mr) {
        map_rect_destroy(this_->mr);
    }
    this_->mr=map_rect_new(this_->bookmark, NULL);

    this_->bookmarks_hash=g_hash_table_new(g_str_hash, g_str_equal);
    this_->root=g_new0(struct bookmark_item_priv,1);
    this_->root->type=type_none;
    this_->root->parent=NULL;
    this_->root->children=NULL;
    bookmarks_move_root(this_);

    while ((item=map_rect_get_item(this_->mr))) {
        if (item->type != type_bookmark && item->type != type_bookmark_folder ) continue;
        if (!item_attr_get(item, attr_path, &attr)) {
            item_attr_get(item, attr_label, &attr);
        }
        item_coord_get(item, &c, 1);

        b_item=g_new0(struct bookmark_item_priv,1);
        b_item->c.x=c.x;
        b_item->c.y=c.y;
        b_item->label=g_strdup(attr.u.str);
        b_item->type=item->type;
        b_item->item=*item;

        //Prepare position
        bookmarks_move_root(this_);
        finder=b_item->label;
        while ((pos=strchr(finder,'/'))) {
            *pos=0x00;
            dbg(lvl_debug,"Found path entry: %s",finder);
            if (!bookmarks_move_down(this_,finder)) {
                struct bookmark_item_priv *path_item=g_new0(struct bookmark_item_priv,1);
                path_item->type=type_bookmark_folder;
                path_item->parent=this_->current;
                path_item->children=NULL;
                path_item->label=g_strdup(finder);

                this_->current->children=g_list_append(this_->current->children,path_item);
                this_->current=path_item;
                g_hash_table_insert(this_->bookmarks_hash,b_item->label,path_item);
                this_->bookmarks_list=g_list_append(this_->bookmarks_list,path_item);
            }
            finder+=strlen(finder)+1;
        }
        copy_helper=g_strdup(finder);
        free(b_item->label);
        b_item->label=copy_helper;
        b_item->parent=this_->current;

        g_hash_table_insert(this_->bookmarks_hash,b_item->label,b_item);
        this_->bookmarks_list=g_list_append(this_->bookmarks_list,b_item);
        this_->current->children=g_list_append(this_->current->children,b_item);
        this_->current->children=g_list_first(this_->current->children);
        dbg(lvl_debug,"Added %s to %s and current list now %u long",b_item->label,this_->current->label,
            g_list_length(this_->current->children));
    }
    bookmarks_move_root(this_);
}

struct bookmarks *
bookmarks_new(struct attr *parent, struct attr **attrs, struct transformation *trans) {
    struct bookmarks *this_;

    if (parent->type!=attr_navit) {
        return NULL;
    }

    this_ = g_new0(struct bookmarks,1);
    this_->attr_cbl=callback_list_new();
    this_->parent=parent;
    //this_->attrs=attr_list_dup(attrs);
    this_->trans=trans;

    this_->bookmark_file=g_strjoin(NULL, navit_get_user_data_directory(TRUE), "/bookmark.txt", NULL);
    this_->working_file=g_strjoin(NULL, navit_get_user_data_directory(TRUE), "/bookmark.txt.tmp", NULL);

    this_->clipboard=g_new0(struct bookmark_item_priv,1);

    {
        //Load map now
        struct attr type= {attr_type, {"textfile"}},
        data= {attr_data, {this_->bookmark_file}},
        no_warn= {attr_no_warning_if_map_file_missing, {(void *)1}};
        struct attr *attrs[]= {&type, &data, &no_warn, NULL};
        this_->bookmark=map_new(this_->parent, attrs);
        if (!this_->bookmark)
            return NULL;
        bookmarks_load_hash(this_);
    }

    return this_;
}

void bookmarks_destroy(struct bookmarks *this_) {

    bookmarks_clear_hash(this_);

    map_destroy(this_->bookmark);
    callback_list_destroy(this_->attr_cbl);

    g_free(this_->bookmark_file);
    g_free(this_->working_file);

    g_free(this_->clipboard);
    g_free(this_);
}

struct map*
bookmarks_get_map(struct bookmarks *this_) {
    return this_->bookmark;
}

enum projection bookmarks_get_projection(struct bookmarks *this_) {
    return map_projection(this_->bookmark);
}
void bookmarks_add_callback(struct bookmarks *this_, struct callback *cb) {
    callback_list_add(this_->attr_cbl, cb);
}

static int bookmarks_store_bookmarks_to_file(struct bookmarks *this_,  int limit,int replace) {
    FILE *f;
    struct bookmark_item_priv *item,*parent_item;
    char *fullname;
    const char *prostr;
    int result;
    GHashTable *dedup=g_hash_table_new_full(g_str_hash,g_str_equal,g_free,NULL);

    f=fopen(this_->working_file, replace ? "w+" : "a+");
    if (f==NULL) {
        navit_add_message(this_->parent->u.navit,_("Failed to write bookmarks file"));
        return FALSE;
    }

    this_->bookmarks_list=g_list_first(this_->bookmarks_list);
    while (this_->bookmarks_list) {
        item=(struct bookmark_item_priv*)this_->bookmarks_list->data;

        parent_item=item;
        fullname=g_strdup(item->label);
        while ((parent_item=parent_item->parent)) {
            char *pathHelper;
            if (parent_item->label) {
                pathHelper=g_strconcat(parent_item->label,"/",fullname,NULL);
                g_free(fullname);
                fullname=g_strdup(pathHelper);
                g_free(pathHelper);
                dbg(lvl_debug,"full name: %s",fullname);
            }
        }

        if (!g_hash_table_lookup(dedup,fullname)) {
            g_hash_table_insert(dedup,fullname,fullname);
            if (item->type == type_bookmark) {
                prostr = projection_to_name(projection_mg);
                if (fprintf(f,"%s%s%s0x%x %s0x%x type=%s label=\"%s\" path=\"%s\"\n",
                            prostr, *prostr ? ":" : "",
                            item->c.x >= 0 ? "":"-", item->c.x >= 0 ? item->c.x : -item->c.x,
                            item->c.y >= 0 ? "":"-", item->c.y >= 0 ? item->c.y : -item->c.y,
                            "bookmark", item->label,fullname)<1) {
                    g_free(fullname);
                    break;
                }
            }
            if (item->type == type_bookmark_folder) {
                prostr = projection_to_name(projection_mg);
                if (fprintf(f,"%s%s%s0x%x %s0x%x type=%s label=\"%s\" path=\"%s\"\n",
                            prostr, *prostr ? ":" : "",
                            "", 0,
                            "", 0,
                            "bookmark_folder", item->label,fullname)<1) {
                    g_free(fullname);
                    break;
                }
            }
        }

        /* Limit could be zero, so we start decrementing it from zero and never reach 1
         or it was bigger and we decreased it earlier. So when this counter becomes 1, we know
         that we have enough entries in bookmarks file */
        if (limit==1) {
            break;
        }
        limit--;

        this_->bookmarks_list=g_list_next(this_->bookmarks_list);
    }

#ifdef FlushFileBuffers
    FlushFileBuffers(f)
#endif FlushFileBuffers

    fclose(f);

    g_hash_table_destroy(dedup);

    if (this_->mr) {
        map_rect_destroy(this_->mr);
        this_->mr = 0;
    }

    unlink(this_->bookmark_file);
#ifdef _POSIX
    sync();
#endif _POSIX
    result=(rename(this_->working_file,this_->bookmark_file)==0);
    if (!result) {
        navit_add_message(this_->parent->u.navit,_("Failed to write bookmarks file"));
    }
    return result;
}

/**
 * @param create: create the directory where the file is stored
 * if it does not exist
 * @return The name of the file used to store destinations with its
 * full path. Should be freed using g_free.
 *
 */
char* bookmarks_get_destination_file(gboolean create) {
    return g_strjoin(NULL, navit_get_user_data_directory(create), "/destination.txt", NULL);
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
char* bookmarks_get_center_file(gboolean create) {
    return g_strjoin(NULL, navit_get_user_data_directory(create), "/center.txt", NULL);
}

void bookmarks_set_center_from_file(struct bookmarks *this_, char *file) {
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
}

void bookmarks_write_center_to_file(struct bookmarks *this_, char *file) {
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

static void bookmarks_emit_dbus_signal(struct bookmarks *this_, struct pcoord *c, const char *description,int create) {
    struct attr attr1,attr2,attr3,attr4,cb,*attr_list[5];
    int valid=0;
    attr1.type=attr_type;
    attr1.u.str="bookmark";
    attr2.type=attr_data;
    attr2.u.str=create ? "create" : "delete";
    attr3.type=attr_data;
    attr3.u.str=(char *)description;
    attr4.type=attr_coord;
    attr4.u.pcoord=c;
    attr_list[0]=&attr1;
    attr_list[1]=&attr2;
    attr_list[2]=&attr3;
    attr_list[3]=&attr4;
    attr_list[4]=NULL;
    if (navit_get_attr(this_->parent->u.navit, attr_callback_list, &cb, NULL))
        callback_list_call_attr_4(cb.u.callback_list, attr_command, "dbus_send_signal", attr_list, NULL, &valid);
}

/**
 * Record the given set of coordinates as a bookmark
 *
 * @param navit The navit instance
 * @param c The coordinate to store
 * @param description A label which allows the user to later identify this bookmark
 * @returns nothing
 */
int bookmarks_add_bookmark(struct bookmarks *this_, struct pcoord *pc, const char *description) {
    struct bookmark_item_priv *b_item=g_new0(struct bookmark_item_priv,1);
    int result;

    if (pc) {
        b_item->c.x=pc->x;
        b_item->c.y=pc->y;
        b_item->type=type_bookmark;
    } else {
        b_item->type=type_bookmark_folder;
    }
    b_item->label=g_strdup(description);
    b_item->parent=this_->current;
    b_item->children=NULL;

    this_->current->children=g_list_first(this_->current->children);
    this_->current->children=g_list_append(this_->current->children,b_item);
    this_->bookmarks_list=g_list_first(this_->bookmarks_list);
    this_->bookmarks_list=g_list_append(this_->bookmarks_list,b_item);

    result=bookmarks_store_bookmarks_to_file(this_,0,0);

    callback_list_call_attr_0(this_->attr_cbl, attr_bookmark_map);
    bookmarks_clear_hash(this_);
    bookmarks_load_hash(this_);

    bookmarks_emit_dbus_signal(this_,&(b_item->c),description,TRUE);

    return result;
}

int bookmarks_cut_bookmark(struct bookmarks *this_, const char *label) {
    if (bookmarks_copy_bookmark(this_,label)) {
        return bookmarks_delete_bookmark(this_,label);
    }

    return FALSE;
}
int bookmarks_copy_bookmark(struct bookmarks *this_, const char *label) {
    bookmarks_item_rewind(this_);
    if (this_->current->children==NULL) {
        return 0;
    }
    while (this_->current->iter!=NULL) {
        struct bookmark_item_priv* data=(struct bookmark_item_priv*)this_->current->iter->data;
        if (!strcmp(data->label,label)) {
            this_->clipboard->c=data->c;
            this_->clipboard->type=data->type;
            this_->clipboard->item=data->item;
            this_->clipboard->children=data->children;
            if (!this_->clipboard->label) {
                g_free(this_->clipboard->label);
            }
            this_->clipboard->label=g_strdup(data->label);
            return TRUE;
        }
        this_->current->iter=g_list_next(this_->current->iter);
    }
    return FALSE;
}
int bookmarks_paste_bookmark(struct bookmarks *this_) {
    int result;
    struct bookmark_item_priv* b_item;

    if (!this_->clipboard->label) {
        return FALSE;
    }

    b_item=g_new0(struct bookmark_item_priv,1);
    b_item->c.x=this_->clipboard->c.x;
    b_item->c.y=this_->clipboard->c.y;
    b_item->label=g_strdup(this_->clipboard->label);
    b_item->type=this_->clipboard->type;
    b_item->item=this_->clipboard->item;
    b_item->parent=this_->current;
    b_item->children=this_->clipboard->children;

    g_hash_table_insert(this_->bookmarks_hash,b_item->label,b_item);
    this_->bookmarks_list=g_list_append(this_->bookmarks_list,b_item);
    this_->current->children=g_list_append(this_->current->children,b_item);
    this_->current->children=g_list_first(this_->current->children);

    result=bookmarks_store_bookmarks_to_file(this_,0,0);

    callback_list_call_attr_0(this_->attr_cbl, attr_bookmark_map);
    bookmarks_clear_hash(this_);
    bookmarks_load_hash(this_);

    return result;
}


int bookmarks_delete_bookmark(struct bookmarks *this_, const char *label) {
    int result;

    bookmarks_item_rewind(this_);
    if (this_->current->children==NULL) {
        return 0;
    }
    while (this_->current->iter!=NULL) {
        struct bookmark_item_priv* data=(struct bookmark_item_priv*)this_->current->iter->data;
        if (!strcmp(data->label,label)) {
            this_->bookmarks_list=g_list_first(this_->bookmarks_list);
            this_->bookmarks_list=g_list_remove(this_->bookmarks_list,data);

            result=bookmarks_store_bookmarks_to_file(this_,0,0);

            callback_list_call_attr_0(this_->attr_cbl, attr_bookmark_map);
            bookmarks_clear_hash(this_);
            bookmarks_load_hash(this_);

            bookmarks_emit_dbus_signal(this_,&(data->c),label,FALSE);

            return result;
        }
        this_->current->iter=g_list_next(this_->current->iter);
    }

    return FALSE;
}

int bookmarks_rename_bookmark(struct bookmarks *this_, const char *oldName, const char* newName) {
    int result;

    bookmarks_item_rewind(this_);
    if (this_->current->children==NULL) {
        return 0;
    }
    while (this_->current->iter!=NULL) {
        struct bookmark_item_priv* data=(struct bookmark_item_priv*)this_->current->iter->data;
        if (!strcmp(data->label,oldName)) {
            g_free(data->label);
            data->label=g_strdup(newName);

            result=bookmarks_store_bookmarks_to_file(this_,0,0);

            callback_list_call_attr_0(this_->attr_cbl, attr_bookmark_map);
            bookmarks_clear_hash(this_);
            bookmarks_load_hash(this_);

            return result;
        }
        this_->current->iter=g_list_next(this_->current->iter);
    }

    return FALSE;
}

struct former_destination {
    enum item_type type;
    char* description;
    GList* c;
};

static void free_former_destination(struct former_destination* former_destination) {
    g_free(former_destination->description);
    g_list_foreach(former_destination->c, (GFunc)g_free, NULL);
    g_list_free(former_destination->c);
    g_free(former_destination);
}

/*
 * Doesn't read any items besides former_destination. So active waypoints items of type former_itinerary and former_itinerary_part are skipped.
 */
static GList* read_former_destination_map_as_list(struct map *map) {
    struct map_rect *mr;
    struct item *item;
    struct attr attr;
    struct former_destination *dest;
    struct coord c;
    int more;
    GList* list = NULL;
    if (map && (mr=map_rect_new(map, NULL))) {
        while ((item=map_rect_get_item(mr))) {
            if (item->type != type_former_destination) continue;
            dest = g_new0(struct former_destination, 1);
            dest->type=item->type;
            item_attr_get(item, attr_label, &attr);
            dest->description = g_strdup(attr.u.str);
            more = item_coord_get(item, &c, 1);
            while (more) {
                dest->c = g_list_append(dest->c, g_new(struct coord, 1));
                *(struct coord *)g_list_last(dest->c)->data = c;
                more = item_coord_get(item, &c, 1);
            }
            list = g_list_prepend(list, dest);
        }
        map_rect_destroy(mr);
    }
    return g_list_reverse(list);
}

static int destination_equal(struct former_destination* dest1, struct former_destination* dest2,
                             int ignore_descriptions) {
    if ((dest1->type == dest2->type) &&
            (ignore_descriptions || !strcmp(dest1->description, dest2->description)) &&
            (coord_equal((struct coord *)g_list_last(dest1->c)->data, (struct coord *)g_list_last(dest2->c)->data))) {
        return TRUE;
    }
    return FALSE;
}

/*
 * Find destination in given GList. If remove_found is non-zero, any matching items are removed and new beginning of the list is returned.
 * If remove_found is zero, last matching item is returned. In the latter case, description is ignored and can be NULL.
 */
static GList* find_destination_in_list(struct former_destination* dest_to_remove, GList* former_destinations,
                                       int remove_found) {
    GList* curr_el = former_destinations;
    GList* prev_el = NULL;
    GList* found_el = NULL;
    struct former_destination* curr_dest;
    while(curr_el) {
        curr_dest = curr_el->data;
        if (destination_equal(dest_to_remove, curr_dest, remove_found?0:1)) {
            if(remove_found) {
                free_former_destination(curr_dest);
                curr_el = g_list_remove(curr_el, curr_dest);
                continue;
            } else {
                found_el=curr_el;
            }
        }
        prev_el = curr_el;
        curr_el = g_list_next(curr_el);

    }
    if(remove_found)
        return g_list_first(prev_el);
    else
        return found_el;

}

/**
 * @brief Write all former destinations into a text file
 *
 * @param[in] former_destinations A GList of struct coord data elements containing the list of former destinations
 * @param[in] former_destination_file The name of the output text file
 * @param proj The projection used to represent coordinates in former_destinations' list elements
 */
static void write_former_destinations(const GList* former_destinations, const char *former_destination_file,
                                      enum projection proj) {
    FILE *f;
    const GList* currdest = NULL;
    GList* c_list = NULL;
    struct coord *c;
    struct former_destination *dest;
    const char* prostr = projection_to_name(proj);
    if (prostr == NULL)
        prostr = "";	/* Protect from NULL pointer dereference below */

    f=fopen(former_destination_file, "w");
    if (f) {
        for(currdest = former_destinations; currdest; currdest = g_list_next(currdest)) {
            dest = currdest->data;
            fprintf(f,"type=%s", item_to_name(dest->type));
            if (dest->description)
                fprintf(f," label=\"%s\"", str_escape(escape_mode_quote, dest->description));
            fputc('\n', f);
            c_list = dest->c;
            do {
                c = (struct coord *)c_list->data;
                fprintf(f,"%s%s%s0x%x %s0x%x\n",
                        prostr, *prostr ? ":" : "",
                        c->x >= 0 ? "":"-", c->x >= 0 ? c->x : -c->x,
                        c->y >= 0 ? "":"-", c->y >= 0 ? c->y : -c->y);
                c_list = g_list_next(c_list);
            } while (c_list);
        }
        fclose(f);
    } else {
        dbg(lvl_error, "Error updating destinations file %s: %s", former_destination_file, strerror(errno));
    }
}

/**
 * @brief Append recent destination(s) item to the former destionations map.
 * @param former_destination_map
 * @param former_destination_file
 * @param c coordinates of item point(s). Can be set to NULL when navigation is stopped to remove type_former_itinerary and
 *    type_former_itinerary_part items from the file.
 * @param count number of points in this item. Set to 0 when navigation is stopped.
 * @param type type_former_destination, type_former_itinerary and type_former_itinerary_part are meaningful here
 * @param description character string used to identify this destination. If NULL, most recent waypoint at these coordinates will be used
 *   to get description.
 * @param limit Limits the number of entries in the "backlog". Set to 0 for "infinite"
 */
void bookmarks_append_destinations(struct map *former_destination_map, char *former_destination_file, struct pcoord *c,
                                   int count, enum item_type type, const char *description, int limit) {
    struct former_destination *new_dest=NULL;
    GList* former_destinations = NULL;
    GList* former_destinations_shortened = NULL;
    struct coord* c_dup;
    int no_of_former_destinations;
    int i;

    former_destinations = read_former_destination_map_as_list(former_destination_map);

    if(c && count>0) {
        GList *older;
        struct coord ctmp;
        new_dest = g_new0(struct former_destination, 1);
        new_dest->type = type;
        for (i=0; i<count; i++) {
            ctmp.x=c[i].x;
            ctmp.y=c[i].y;
            c_dup = g_new(struct coord, 1);
            transform_from_to(&ctmp,c[i].pro, c_dup, map_projection(former_destination_map));
            new_dest->c = g_list_append(new_dest->c, c_dup);
        }
        older=find_destination_in_list(new_dest, former_destinations,0);
        if(!description && older)
            description=((struct former_destination *)older->data)->description;
        new_dest->description = g_strdup(description?description:_("Map Point"));

        former_destinations = find_destination_in_list(new_dest, former_destinations, 1);
        former_destinations = g_list_append(former_destinations, new_dest);
    }
    no_of_former_destinations = g_list_length(former_destinations);
    if (limit > 0 && no_of_former_destinations > limit)
        former_destinations_shortened = g_list_nth(former_destinations, no_of_former_destinations - limit);
    else
        former_destinations_shortened = former_destinations;

    write_former_destinations(former_destinations_shortened, former_destination_file,
                              map_projection(former_destination_map));
    g_list_foreach(former_destinations, (GFunc) free_former_destination, NULL);
    g_list_free(former_destinations);
}


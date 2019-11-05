/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
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
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include "config.h"
#include "debug.h"
#include "plugin.h"
#include "projection.h"
#include "item.h"
#include "map.h"
#include "maptype.h"
#include "attr.h"
#include "transform.h"
#include "file.h"

#include "textfile.h"

static int map_id;

static void remove_comment_line(char* line) {
    if (line[0]==TEXTFILE_COMMENT_CHAR) {
        line='\0';
    }
}

static void get_line(struct map_rect_priv *mr) {
    if(mr->f) {
        if (!mr->m->is_pipe)
            mr->pos=ftell(mr->f);
        else
            mr->pos+=mr->lastlen;
        if(fgets(mr->line, TEXTFILE_LINE_SIZE, mr->f) == NULL) {
            dbg(lvl_error, "Unable to get line (%s)", strerror(errno));
            mr->line[0]=0;
        }
        dbg(lvl_debug,"read textfile line: %s", mr->line);
        remove_comment_line(mr->line);
        mr->lastlen=strlen(mr->line)+1;
        if (strlen(mr->line) >= TEXTFILE_LINE_SIZE-1)
            dbg(lvl_error, "line too long: %s", mr->line);
    }
}

static void map_destroy_textfile(struct map_priv *m) {
    g_free(m->filename);
    if(m->charset) {
        g_free(m->charset);
    }
    g_free(m);
}

static void textfile_coord_rewind(void *priv_data) {
}

static int parse_line(struct map_rect_priv *mr, int attr) {
    int pos;

    pos=coord_parse(mr->line, projection_mg, &mr->c);
    if (pos < strlen(mr->line) && attr) {
        strcpy(mr->attrs, mr->line+pos);
    }
    return pos;
}

static int textfile_coord_get(void *priv_data, struct coord *c, int count) {
    struct map_rect_priv *mr=priv_data;
    int ret=0;
    dbg(lvl_warning,"enter, count: %d",count);
    while (count--) {
        if (mr->f && !feof(mr->f) && (!mr->item.id_hi || !mr->eoc) && parse_line(mr, mr->item.id_hi)) {
            if (c) {
                *c=mr->c;
                dbg(lvl_debug,"c=0x%x,0x%x", c->x, c->y);
                c++;
            }
            ret++;
            get_line(mr);
            if (mr->item.id_hi)
                mr->eoc=1;
        } else {
            mr->more=0;
            break;
        }
    }
    return ret;
}

static void textfile_attr_rewind(void *priv_data) {
    struct map_rect_priv *mr=priv_data;
    mr->attr_pos=0;
    mr->attr_last=attr_none;
}

static void textfile_encode_attr(char *attr_val, enum attr_type attr_type, struct attr *attr) {
    if (attr_type >= attr_type_int_begin && attr_type <= attr_type_int_end)
        attr->u.num=atoi(attr_val);
    else
        attr->u.str=attr_val;
}

static int textfile_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr) {
    struct map_rect_priv *mr=priv_data;
    char *str=NULL;
    dbg(lvl_debug,"mr=%p attrs='%s' ", mr, mr->attrs);
    if (attr_type != mr->attr_last) {
        dbg(lvl_debug,"reset attr_pos");
        mr->attr_pos=0;
        mr->attr_last=attr_type;
    }
    if (attr_type == attr_any) {
        dbg(lvl_debug,"attr_any");
        if (attr_from_line(mr->attrs,NULL,&mr->attr_pos,mr->attr, mr->attr_name)) {
            attr_type=attr_from_name(mr->attr_name);
            dbg(lvl_debug,"found attr '%s' 0x%x", mr->attr_name, attr_type);
            attr->type=attr_type;
            textfile_encode_attr(mr->attr, attr_type, attr);
            return 1;
        }
    } else {
        str=attr_to_name(attr_type);
        dbg(lvl_debug,"attr='%s' ",str);
        if (attr_from_line(mr->attrs,str,&mr->attr_pos,mr->attr, NULL)) {
            textfile_encode_attr(mr->attr, attr_type, attr);
            dbg(lvl_debug,"found");
            return 1;
        }
    }
    dbg(lvl_debug,"not found");
    return 0;
}

static struct item_methods methods_textfile = {
    textfile_coord_rewind,
    textfile_coord_get,
    textfile_attr_rewind,
    textfile_attr_get,
};

static struct map_rect_priv *map_rect_new_textfile(struct map_priv *map, struct map_selection *sel) {
    struct map_rect_priv *mr;

    dbg(lvl_debug,"enter");
    mr=g_new0(struct map_rect_priv, 1);
    mr->m=map;
    mr->sel=sel;
    if (map->flags & 1)
        mr->item.id_hi=1;
    else
        mr->item.id_hi=0;
    mr->item.id_lo=0;
    mr->item.meth=&methods_textfile;
    mr->item.priv_data=mr;
    if (map->is_pipe) {
#ifdef HAVE_POPEN
        char *oargs,*args=g_strdup(map->filename),*sep=" ";
        enum layer_type lay;
        g_free(mr->args);
        while (sel) {
            oargs=args;
            args=g_strdup_printf("%s 0x%x 0x%x 0x%x 0x%x", oargs, sel->u.c_rect.lu.x, sel->u.c_rect.lu.y, sel->u.c_rect.rl.x,
                                 sel->u.c_rect.rl.y);
            g_free(oargs);
            for (lay=layer_town ; lay < layer_end ; lay++) {
                oargs=args;
                args=g_strdup_printf("%s%s%d", oargs, sep, sel->order);
                g_free(oargs);
                sep=",";
            }
            sel=sel->next;
        }
        dbg(lvl_debug,"popen args %s", args);
        mr->args=args;
        mr->f=popen(mr->args, "r");
        mr->pos=0;
        mr->lastlen=0;
#else
        dbg(lvl_error,"unable to work with pipes %s",map->filename);
#endif
    } else {
        mr->f=fopen(map->filename, "r");
    }
    if(!mr->f) {
        if (!(errno == ENOENT && map->no_warning_if_map_file_missing)) {
            dbg(lvl_error, "error opening textfile %s: %s", map->filename, strerror(errno));
        }
    }
    get_line(mr);
    return mr;
}


static void map_rect_destroy_textfile(struct map_rect_priv *mr) {
    if (mr->f) {
        if (mr->m->is_pipe) {
#ifdef HAVE_POPEN
            pclose(mr->f);
#endif
        } else {
            fclose(mr->f);
        }
    }
    g_free(mr);
}

static struct item *map_rect_get_item_textfile(struct map_rect_priv *mr) {
    char *p,type[TEXTFILE_LINE_SIZE];
    dbg(lvl_debug,"map_rect_get_item_textfile id_hi=%d line=%s", mr->item.id_hi, mr->line);
    if (!mr->f) {
        return NULL;
    }
    while (mr->more) {
        struct coord c;
        textfile_coord_get(mr, &c, 1);
    }
    for(;;) {
        if (feof(mr->f)) {
            dbg(lvl_debug,"map_rect_get_item_textfile: eof %d",mr->item.id_hi);
            if (mr->m->flags & 1) {
                if (!mr->item.id_hi)
                    return NULL;
                mr->item.id_hi=0;
            } else {
                if (mr->item.id_hi)
                    return NULL;
                mr->item.id_hi=1;
            }
            if (mr->m->is_pipe) {
#ifdef HAVE_POPEN
                pclose(mr->f);
                mr->f=popen(mr->args, "r");
                mr->pos=0;
                mr->lastlen=0;
#endif
            } else {
                fseek(mr->f, 0, SEEK_SET);
                clearerr(mr->f);
            }
            get_line(mr);
        }
        if ((p=strchr(mr->line,'\n')))
            *p='\0';
        if (mr->item.id_hi) {
            mr->attrs[0]='\0';
            if (!parse_line(mr, 1)) {
                get_line(mr);
                continue;
            }
            dbg(lvl_debug,"map_rect_get_item_textfile: point found");
            mr->eoc=0;
            mr->item.id_lo=mr->pos;
        } else {
            if (parse_line(mr, 1)) {
                get_line(mr);
                continue;
            }
            dbg(lvl_debug,"map_rect_get_item_textfile: line found");
            if (! mr->line[0]) {
                get_line(mr);
                continue;
            }
            mr->item.id_lo=mr->pos;
            strcpy(mr->attrs, mr->line);
            get_line(mr);
            dbg(lvl_debug,"mr=%p attrs=%s", mr, mr->attrs);
        }
        dbg(lvl_debug,"get_attrs %s", mr->attrs);
        if (attr_from_line(mr->attrs,"type",NULL,type,NULL)) {
            dbg(lvl_debug,"type='%s'", type);
            mr->item.type=item_from_name(type);
            if (mr->item.type == type_none)
                dbg(lvl_error, "Warning: type '%s' unknown", type);
        } else {
            get_line(mr);
            continue;
        }
        mr->attr_last=attr_none;
        mr->more=1;
        dbg(lvl_debug,"return attr='%s'", mr->attrs);
        return &mr->item;
    }
}

static struct item *map_rect_get_item_byid_textfile(struct map_rect_priv *mr, int id_hi, int id_lo) {
    if (mr->m->is_pipe) {
#ifndef _MSC_VER
        pclose(mr->f);
        mr->f=popen(mr->args, "r");
        mr->pos=0;
        mr->lastlen=0;
#endif /* _MSC_VER */
    } else
        fseek(mr->f, id_lo, SEEK_SET);
    get_line(mr);
    mr->item.id_hi=id_hi;
    return map_rect_get_item_textfile(mr);
}

static struct map_methods map_methods_textfile = {
    projection_mg,
    "utf-8",
    map_destroy_textfile,
    map_rect_new_textfile,
    map_rect_destroy_textfile,
    map_rect_get_item_textfile,
    map_rect_get_item_byid_textfile,
};

static struct map_priv *map_new_textfile(struct map_methods *meth, struct attr **attrs, struct callback_list *cbl) {
    struct map_priv *m;
    struct attr *data=attr_search(attrs, NULL, attr_data);
    struct attr *charset=attr_search(attrs, NULL, attr_charset);
    struct attr *flags=attr_search(attrs, NULL, attr_flags);
    struct attr *no_warn=attr_search(attrs, NULL, attr_no_warning_if_map_file_missing);
    struct file_wordexp *wexp;
    int len,is_pipe=0;
    char *wdata;
    char **wexp_data;
    if (! data)
        return NULL;
    dbg(lvl_debug,"map_new_textfile %s", data->u.str);
    wdata=g_strdup(data->u.str);
    len=strlen(wdata);
    if (len && wdata[len-1] == '|') {
        wdata[len-1]='\0';
        is_pipe=1;
    }
    wexp=file_wordexp_new(wdata);
    wexp_data=file_wordexp_get_array(wexp);
    *meth=map_methods_textfile;

    m=g_new0(struct map_priv, 1);
    m->id=++map_id;
    m->filename=g_strdup(wexp_data[0]);
    m->is_pipe=is_pipe;
    m->no_warning_if_map_file_missing=(no_warn!=NULL) && (no_warn->u.num);
    if (flags)
        m->flags=flags->u.num;
    dbg(lvl_debug,"map_new_textfile %s %s", m->filename, wdata);
    if (charset) {
        m->charset=g_strdup(charset->u.str);
        meth->charset=m->charset;
    }
    file_wordexp_destroy(wexp);
    g_free(wdata);
    return m;
}

void plugin_init(void) {
    dbg(lvl_debug,"textfile: plugin_init");
    plugin_register_category_map("textfile", map_new_textfile);
}


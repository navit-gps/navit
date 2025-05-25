/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2009 Navit Team
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

#ifndef NAVIT_VOICE_H
#define NAVIT_VOICE_H

#ifdef __cplusplus
extern "C" {
#endif

struct point;
struct voice_priv;

struct voice_methods {
    void (*destroy)(struct voice_priv *priv);
    int (*position_attr_get)(struct voice_priv *priv, enum attr_type type, struct attr *attr);
    int (*set_attr)(struct voice_priv *priv, struct attr *attr);
};

/* prototypes */
enum attr_type;
struct attr;
struct attr_iter;
struct cursor;
struct graphics;
struct point;
struct voice;
struct voice *voice_new(struct attr *parent, struct attr **attrs);
void voice_destroy(struct voice *this_);
struct attr_iter *voice_attr_iter_new(void * unused);
void voice_attr_iter_destroy(struct attr_iter *iter);
int voice_get_attr(struct voice *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter);
int voice_set_attr(struct voice *this_, struct attr *attr);
int voice_add_attr(struct voice *this_, struct attr *attr);
int voice_remove_attr(struct voice *this_, struct attr *attr);
void voice_set_cursor(struct voice *this_, struct cursor *cursor, int overwrite);
void voice_draw(struct voice *this_, struct graphics *gra, struct point *pnt, int angle, int speed);
int voice_get_cursor_data(struct voice *this_, struct point *pnt, int *angle, int *speed);
void voice_log_gpx_add_tag(char *tag, char **logstr);
struct voice * voice_ref(struct voice *this_);
void voice_unref(struct voice *this_);
/* end of prototypes */

#ifdef __cplusplus
}
#endif

#endif


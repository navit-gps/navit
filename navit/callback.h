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

#ifndef NAVIT_CALLBACK_H
#define NAVIT_CALLBACK_H

#include "item.h"
#include "attr.h"

#ifdef __cplusplus
extern "C" {
#endif
/* prototypes */
enum attr_type;
struct callback;
struct callback_list;
typedef void (*callback_patch) (struct callback_list *l, enum attr_type type, int pcount, void **p, void * context);
struct callback_list *callback_list_new(void);
struct callback *callback_new_attr(void (*func)(void), enum attr_type type, int pcount, void **p);
struct callback *callback_new_attr_args(void (*func)(void), enum attr_type type, int count, ...);
struct callback *callback_new(void (*func)(void), int pcount, void **p);
struct callback *callback_new_args(void (*func)(void), int count, ...);
void callback_destroy(struct callback *cb);
void callback_set_arg(struct callback *cb, int arg, void *p);
void callback_list_add(struct callback_list *l, struct callback *cb);
struct callback *callback_list_add_new(struct callback_list *l, void (*func)(void), int pcount, void **p);
void callback_list_remove(struct callback_list *l, struct callback *cb);
void callback_list_remove_destroy(struct callback_list *l, struct callback *cb);
void callback_list_add_patch_function (struct callback_list *l, callback_patch patch, void * context);
void callback_call(struct callback *cb, int pcount, void **p);
void callback_call_args(struct callback *cb, int count, ...);
void callback_list_call_attr(struct callback_list *l, enum attr_type type, int pcount, void **p);
void callback_list_call_attr_args(struct callback_list *cbl, enum attr_type type, int count, ...);
void callback_list_call(struct callback_list *l, int pcount, void **p);
void callback_list_call_args(struct callback_list *cbl, int count, ...);
void callback_list_destroy(struct callback_list *l);
/* end of prototypes */

#define callback_new_0(func) callback_new_args(func, 0)
#define callback_new_1(func,p1) callback_new_args(func, 1, p1)
#define callback_new_2(func,p1,p2) callback_new_args(func, 2, p1, p2)
#define callback_new_3(func,p1,p2,p3) callback_new_args(func, 3, p1, p2, p3)
#define callback_new_4(func,p1,p2,p3,p4) callback_new_args(func, 4, p1, p2, p3, p4)

#define callback_new_attr_0(func,type) callback_new_attr_args(func, type, 0)
#define callback_new_attr_1(func,type,p1) callback_new_attr_args(func, type, 1, p1)
#define callback_new_attr_2(func,type,p1,p2) callback_new_attr_args(func, type, 2, p1, p2)
#define callback_new_attr_3(func,type,p1,p2,p3) callback_new_attr_args(func, type, 3, p1, p2, p3)
#define callback_new_attr_4(func,type,p1,p2,p3,p4) callback_new_attr_args(func, type, 4, p1, p2, p3, p4)

#define callback_call_0(cb) callback_call_args(cb, 0)
#define callback_call_1(cb,p1) callback_call_args(cb, 1, p1)
#define callback_call_2(cb,p1,p2) callback_call_args(cb, 2, p1, p2)
#define callback_call_3(cb,p1,p2,p3) callback_call_args(cb, 3, p1, p2, p3)
#define callback_call_4(cb,p1,p2,p3,p4) callback_call_args(cb, 4, p1, p2, p3, p4)

#define callback_list_call_0(cbl) callback_list_call_args(cbl, 0)
#define callback_list_call_1(cbl,p1) callback_list_call_args(cbl, 1, p1)
#define callback_list_call_2(cbl,p1,p2) callback_list_call_args(cbl, 2, p1, p2)
#define callback_list_call_3(cbl,p1,p2,p3) callback_list_call_args(cbl, 3, p1, p2, p3)
#define callback_list_call_4(cbl,p1,p2,p3,p4) callback_list_call_args(cbl, 4, p1, p2, p3, p4)

#define callback_list_call_attr_0(cbl,type) callback_list_call_attr_args(cbl,type, 0)
#define callback_list_call_attr_1(cbl,type,p1) callback_list_call_attr_args(cbl, type, 1, p1)
#define callback_list_call_attr_2(cbl,type,p1,p2) callback_list_call_attr_args(cbl, type, 2, p1, p2)
#define callback_list_call_attr_3(cbl,type,p1,p2,p3) callback_list_call_attr_args(cbl, type, 3, p1, p2, p3)
#define callback_list_call_attr_4(cbl,type,p1,p2,p3,p4) callback_list_call_attr_args(cbl, type, 4, p1, p2, p3, p4)

#define callback_cast(x) (void (*)(void))(x)
#ifdef __cplusplus
}
#endif

#endif


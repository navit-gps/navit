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
#include <string.h>
#include "item.h"
#include "debug.h"
#include "callback.h"


struct callback {
    /* func has variable number of arguments, not (void),
     * but we must declare something... */
    void (*func)(void);
    int pcount;
    enum attr_type type;
    void *p[0];

};

struct callback_list {
    callback_patch patch;
    void * patch_context;
    GList *list;
};

struct callback_list * callback_list_new(void) {
    struct callback_list *ret=g_new0(struct callback_list, 1);

    return ret;
}

struct callback * callback_new_attr(void (*func)(void), enum attr_type type, int pcount, void **p) {
    struct callback *ret;
    int i;

    ret=g_malloc0(sizeof(struct callback)+pcount*sizeof(void *));
    ret->func=func;
    ret->pcount=pcount;
    ret->type=type;
    for (i = 0 ; i < pcount ; i++) {
        ret->p[i]=p[i];
    }
    return ret;
}

struct callback * callback_new_attr_args(void (*func)(void), enum attr_type type, int count, ...) {
    int i;
    void **p=g_alloca(sizeof(void*)*count);
    va_list ap;
    va_start(ap, count);
    for (i = 0 ; i < count ; i++)
        p[i]=va_arg(ap, void *);
    va_end(ap);
    return callback_new_attr(func, type, count, p);
}

struct callback * callback_new(void (*func)(void), int pcount, void **p) {
    return callback_new_attr(func, attr_none, pcount, p);
}

struct callback * callback_new_args(void (*func)(void), int count, ...) {
    int i;
    void **p=g_alloca(sizeof(void*)*count);
    va_list ap;
    va_start(ap, count);
    for (i = 0 ; i < count ; i++)
        p[i]=va_arg(ap, void *);
    va_end(ap);
    return callback_new(func, count, p);
}

void callback_destroy(struct callback *cb) {
    g_free(cb);
}

void callback_set_arg(struct callback *cb, int arg, void *p) {
    if (arg < 0 || arg > cb->pcount)
        return;
    cb->p[arg]=p;
}

void callback_list_add(struct callback_list *l, struct callback *cb) {
    l->list=g_list_prepend(l->list, cb);
}


struct callback * callback_list_add_new(struct callback_list *l, void (*func)(void), int pcount, void **p) {
    struct callback *ret;

    ret=callback_new(func, pcount, p);
    callback_list_add(l, ret);
    return ret;
}

void callback_list_remove(struct callback_list *l, struct callback *cb) {
    l->list=g_list_remove(l->list, cb);
}

void callback_list_remove_destroy(struct callback_list *l, struct callback *cb) {
    callback_list_remove(l, cb);
    g_free(cb);
}

void callback_list_add_patch_function (struct callback_list *l, callback_patch patch, void * context) {
    if(!l)
        return;
    l->patch = patch;
    l->patch_context = context;
}

void callback_call(struct callback *cb, int pcount, void **p) {
    int i;
    void *pf[8];
    if (! cb)
        return;
    if (cb->pcount + pcount <= 8) {
        dbg(lvl_debug,"cb->pcount=%d", cb->pcount);
        if (cb->pcount && cb->p)
            dbg(lvl_debug,"cb->p[0]=%p", cb->p[0]);
        dbg(lvl_debug,"pcount=%d", pcount);
        if (pcount) {
            dbg_assert(p!=NULL);
            dbg(lvl_debug,"p[0]=%p", p[0]);
        }
        for (i = 0 ; i < cb->pcount ; i++)
            pf[i]=cb->p[i];
        for (i = 0 ; i < pcount ; i++)
            pf[i+cb->pcount]=p[i];
        switch (cb->pcount+pcount) {
        case 8:
            ((int (*)(void*,void*,void*,void*,void*,void*,void*,void*))cb->func)(
                pf[0],pf[1],pf[2],pf[3],pf[4],pf[5],pf[6],pf[7]);
            break;
        case 7:
            ((int (*)(void*,void*,void*,void*,void*,void*,void*))cb->func)(
                pf[0],pf[1],pf[2],pf[3],pf[4],pf[5],pf[6]);
            break;
        case 6:
            ((int (*)(void*,void*,void*,void*,void*,void*))cb->func)(
                pf[0],pf[1],pf[2],pf[3],pf[4],pf[5]);
            break;
        case 5:
            ((int (*)(void*,void*,void*,void*,void*))cb->func)(
                pf[0],pf[1],pf[2],pf[3],pf[4]);
            break;
        case 4:
            ((int (*)(void*,void*,void*,void*))cb->func)(
                pf[0],pf[1],pf[2],pf[3]);
            break;
        case 3:
            ((int (*)(void*,void*,void*))cb->func)(
                pf[0],pf[1],pf[2]);
            break;
        case 2:
            ((int (*)(void*,void*))cb->func)(
                pf[0],pf[1]);
            break;
        case 1:
            ((int (*)(void*))cb->func)(
                pf[0]);
            break;
        case 0:
            cb->func();
            break;
        }
    } else {
        dbg(lvl_error,"too many parameters for callback (%d+%d)", cb->pcount, pcount);
    }
}

void callback_call_args(struct callback *cb, int count, ...) {
    int i;
    void **p=g_alloca(sizeof(void*)*count);
    va_list ap;
    va_start(ap, count);
    for (i = 0 ; i < count ; i++)
        p[i]=va_arg(ap, void *);
    va_end(ap);
    callback_call(cb, count, p);
}

void callback_list_call_attr(struct callback_list *l, enum attr_type type, int pcount, void **p) {
    GList *cbi;
    struct callback *cb;

    if (!l) {
        return;
    }
    if(l->patch != NULL)
        l->patch(l, type, pcount, p, l->patch_context);

    cbi=l->list;
    while (cbi) {
        cb=cbi->data;
        if (type == attr_any || cb->type == attr_any || cb->type == type)
            callback_call(cb, pcount, p);
        cbi=g_list_next(cbi);
    }

}

void callback_list_call_attr_args(struct callback_list *cbl, enum attr_type type, int count, ...) {
    int i;
    void **p=g_alloca(sizeof(void*)*count);
    va_list ap;
    va_start(ap, count);
    for (i = 0 ; i < count ; i++)
        p[i]=va_arg(ap, void *);
    va_end(ap);
    callback_list_call_attr(cbl, type, count, p);
}

void callback_list_call(struct callback_list *l, int pcount, void **p) {
    callback_list_call_attr(l, attr_any, pcount, p);
}

void callback_list_call_args(struct callback_list *cbl, int count, ...) {
    int i;
    void **p=g_alloca(sizeof(void*)*count);
    va_list ap;
    va_start(ap, count);
    for (i = 0 ; i < count ; i++)
        p[i]=va_arg(ap, void *);
    va_end(ap);
    callback_list_call(cbl, count, p);
}

void callback_list_destroy(struct callback_list *l) {
    GList *cbi;
    cbi=l->list;
    while (cbi) {
        g_free(cbi->data);
        cbi=g_list_next(cbi);
    }
    g_list_free(l->list);
    g_free(l);
}

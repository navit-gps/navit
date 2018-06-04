#include <glib.h>
#include <stdlib.h>
#include "config.h"
#ifdef HAVE_API_WIN32_BASE
#include <windows.h>
#endif
#ifndef _MSC_VER
#include <sys/time.h>
#endif /* _MSC_VER */
#include "color.h"
#include "coord.h"
#include "point.h"
#include "callback.h"
#include "graphics.h"
#include "debug.h"
#include "navit.h"
#include "types.h"
#include "navit_nls.h"
#include "event.h"
#include "search.h"
#include "country.h"
#include "track.h"
#include "gui_internal.h"
#include "gui_internal_widget.h"
#include "gui_internal_priv.h"
#include "gui_internal_html.h"
#include "gui_internal_menu.h"
#include "gui_internal_gesture.h"

void gui_internal_gesture_ring_clear(struct gui_priv *this) {
    this->gesture_ring_last=this->gesture_ring_first=0;
};

static struct gesture_elem *gui_internal_gesture_ring_get(struct gui_priv *this, int i) {
    int n=(this->gesture_ring_last-i+GESTURE_RINGSIZE)%GESTURE_RINGSIZE;
    if(n==this->gesture_ring_first)
        return NULL;
    return this->gesture_ring+n;
};

void gui_internal_gesture_ring_add(struct gui_priv *this, struct point *p) {
    long long msec;
#ifndef HAVE_API_WIN32_CE
    struct timeval tv;
    gettimeofday(&tv,NULL);
    msec=((long long)tv.tv_sec)*1000+tv.tv_usec/1000;
#else
    msec=GetTickCount();
#endif
    this->gesture_ring_last++;
    this->gesture_ring_last%=GESTURE_RINGSIZE;
    if(this->gesture_ring_last==this->gesture_ring_first) {
        this->gesture_ring_first++;
        this->gesture_ring_first%=GESTURE_RINGSIZE;
    }
    this->gesture_ring[this->gesture_ring_last].p=*p;
    this->gesture_ring[this->gesture_ring_last].msec=msec;
    dbg(lvl_info,"msec="LONGLONG_FMT" x=%d y=%d",msec,p->x,p->y);
};

int gui_internal_gesture_get_vector(struct gui_priv *this, long long msec, struct point *p0, int *dx, int *dy) {
    struct gesture_elem *g;
    int x,y,dt;
    int i;

    dt=0;

    if(dx) *dx=0;
    if(dy) *dy=0;
    if(p0) {
        p0->x=-1;
        p0->y=-1;
    }

    g=gui_internal_gesture_ring_get(this,0);
    if(!g)
        return 0;
    x=g->p.x;
    y=g->p.y;
    if(p0) {
        *p0=g->p;
    }
    msec=g->msec;
    dbg(lvl_info,LONGLONG_FMT" %d %d",g->msec, g->p.x, g->p.y);
    for(i=1; (g=gui_internal_gesture_ring_get(this,i))!=NULL; i++) {
        if( msec-g->msec > 1000 )
            break;
        dt=msec-g->msec;
        if(dx) *dx=x-g->p.x;
        if(dy) *dy=y-g->p.y;
        if(p0) {
            *p0=g->p;
        }
        dbg(lvl_info,LONGLONG_FMT" %d %d",g->msec, g->p.x, g->p.y);
    }
    return dt;
}

int gui_internal_gesture_do(struct gui_priv *this) {
    int dt;
    int dx,dy;

    dt=gui_internal_gesture_get_vector(this, 1000, NULL, &dx, &dy);

    if( abs(dx) > this->icon_s*3 && abs(dy) < this->icon_s ) {
        struct widget *wt;
        dbg(lvl_debug,"horizontal dx=%d dy=%d",dx,dy);

        /* Prevent swiping if widget was scrolled beforehand */
        if(this->pressed==2)
            return 0;

        for(wt=this->highlighted; wt && wt->type!=widget_table; wt=wt->parent);
        if(!wt || wt->type!=widget_table || !wt->data)
            return 0;
        if(this->highlighted) {
            this->highlighted->state &= ~STATE_HIGHLIGHTED;
            this->highlighted=NULL;
        }
        if(dx<0)
            gui_internal_table_button_next(this,NULL,wt);
        else
            gui_internal_table_button_prev(this,NULL,wt);
        return 1;
    } else if( abs(dy) > this->icon_s*3 && abs(dx) < this->icon_s ) {
        dbg(lvl_debug,"vertical dx=%d dy=%d",dx,dy);
    } else if (dt>300 && abs(dx) <this->icon_s && abs(dy) <this->icon_s ) {
        dbg(lvl_debug,"longtap dx=%d dy=%d",dx,dy);
    } else {
        dbg(lvl_debug,"none dx=%d dy=%d",dx,dy);
    }

    return 0;

};

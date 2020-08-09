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

#include <math.h>
#include <stdio.h>
#include <glib.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "config.h"
#include <navit/item.h>
#include <navit/xmlconfig.h>
#include <navit/main.h>
#include <navit/debug.h>
#include <navit/point.h>
#include <navit/graphics.h>
#include <navit/transform.h>
#include <navit/map.h>
#include <navit/navit.h>
#include <navit/callback.h>
#include <navit/file.h>
#include <navit/color.h>
#include <navit/route.h>
#include <navit/plugin.h>
#include <navit/layout.h>
#include <navit/mapset.h>
#include <navit/osd.h>
#include <navit/event.h>
#include <navit/command.h>
#include <navit/config_.h>
#include <navit/vehicle.h>

/* #define DEMO 1 */

#ifdef HAVE_API_ANDROID

#include <navit/android.h>

#endif

#define ORIENTATION_UNKNOWN 0
#define ORIENTATION_PORTRAIT 1
#define ORIENTATION_LANDSCAPE 2
#define ORIENTATION_FLAT 3
#define TYPE_ACCELEROMETER 1
#define TYPE_MAGNETIC_FIELD 2

static struct map *global_map;

int orientation, orientation_old;

struct pedestrian {
    struct navit *nav;
    int w, h;
    int yaw;
} pedestrian_data;

int sensors_locked;

struct attr initial_layout, main_layout;


struct rocket {
    struct navit *navit;
    struct layout *layout;
    struct graphics *gra;
    struct transformation *trans;
    struct displaylist *dl;
    struct mapset *ms;
    int a, g, t, hog, v, vscale;
    struct callback *callback;
    struct event_idle *idle;
};

static void pedestrian_rocket_idle(struct rocket *rocket) {
    struct attr follow;
    follow.type = attr_follow;
    follow.u.num = 100;
    transform_set_hog(rocket->trans, rocket->hog);
    graphics_displaylist_draw(rocket->gra, rocket->dl, rocket->trans, rocket->layout, 0);
    rocket->v += rocket->a - rocket->g;
    dbg(lvl_debug, "enter v=%d", rocket->v);
    if (rocket->t > 0) {
        rocket->t--;
    } else {
        rocket->a = 0;
    }
    rocket->hog += rocket->v / rocket->vscale;
    dbg(lvl_debug, "hog=%d", rocket->hog);
    if (rocket->hog < 0) {
        transform_set_hog(rocket->trans, 0);
        transform_set_order_base(rocket->trans, 14);
        transform_set_scale(rocket->trans, transform_get_scale(rocket->trans));
        graphics_overlay_disable(rocket->gra, 0);
        navit_draw(rocket->navit);
        follow.u.num = 1;
        event_remove_idle(rocket->idle);
        rocket->idle = NULL;
        sensors_locked = 0;
    }
    navit_set_attr(rocket->navit, &follow);
}

static void pedestrian_cmd_pedestrian_rocket(struct rocket *rocket) {
    struct attr attr;
    int max = 0;
#if 0
    int i,step=10;
#endif
    rocket->a = 2;
    rocket->g = 1;
    rocket->t = 100;
    rocket->hog = 0;
    rocket->v = 0;
    rocket->vscale = 10;
    if (!navit_get_attr(rocket->navit, attr_graphics, &attr, NULL)) {
        return;
    }
    rocket->gra = attr.u.graphics;
    if (!navit_get_attr(rocket->navit, attr_transformation, &attr, NULL)) {
        return;
    }
    rocket->trans = attr.u.transformation;
    if (!navit_get_attr(rocket->navit, attr_displaylist, &attr, NULL)) {
        return;
    }
    rocket->dl = attr.u.displaylist;
    if (!navit_get_attr(rocket->navit, attr_mapset, &attr, NULL)) {
        return;
    }
    rocket->ms = attr.u.mapset;
    transform_set_hog(rocket->trans, max);
    transform_set_order_base(rocket->trans, 14);
    transform_set_scale(rocket->trans, transform_get_scale(rocket->trans));
    transform_setup_source_rect(rocket->trans);
    graphics_overlay_disable(rocket->gra, 1);
    graphics_draw(rocket->gra, rocket->dl, rocket->ms, rocket->trans, rocket->layout, 0, NULL, 0);
    sensors_locked = 1;
    if (!rocket->idle) {
        rocket->idle = event_add_idle(50, rocket->callback);
    }
#if 0
    while (hog >= 0) {
        transform_set_hog(trans, hog);
#if 0
        graphics_draw(gra, dl, ms, trans, layout, 0, NULL);
#else
        graphics_displaylist_draw(gra, dl, trans, layout, 0);
#endif
        v=v+a-g;
        if (t > 0)
            t--;
        else
            a=0;
        hog=hog+v/vscale;
    }
#if 0
    for (i = 0 ; i < max ; i+=step) {
        transform_set_hog(trans, i);
        graphics_displaylist_draw(gra, dl, trans, layout, 0);
    }
    for (i = max ; i >= 0 ; i-=step) {
        transform_set_hog(trans, i);
        graphics_displaylist_draw(gra, dl, trans, layout, 0);
    }
#endif
#endif
}

static struct command_table commands[] = {{"pedestrian_rocket", command_cast(pedestrian_cmd_pedestrian_rocket)},};


static void osd_rocket_init(struct navit *nav) {
    struct rocket *rocket = g_new0(struct rocket, 1);
    struct attr attr;
    rocket->navit = nav;
    rocket->callback = callback_new_1(callback_cast(pedestrian_rocket_idle), rocket);
    if (navit_get_attr(nav, attr_layout, &attr, NULL)) {
        rocket->layout = attr.u.layout;
    }
    if (navit_get_attr(nav, attr_callback_list, &attr, NULL)) {
        dbg(lvl_debug, "ok");
        command_add_table(attr.u.callback_list, commands,
                          sizeof(commands) / sizeof(struct command_table), rocket);
    }
}

struct marker {
    struct cursor *cursor;
};

static void osd_marker_draw(struct marker *this, struct navit *nav) {
#if 0
    struct attr graphics;
    struct point p;
    dbg(lvl_debug,"enter");
    if (!navit_get_attr(nav, attr_graphics, &graphics, NULL)) {
        return;
    }
    p.x=40;
    p.y=50;
    cursor_draw(this->cursor, graphics.u.graphics, &p, 0, 45, 0);
#endif

}

static void osd_marker_init(struct marker *this, struct navit *nav) {
    struct attr cursor;
    struct attr itemgra, polygon, polygoncoord1, polygoncoord2, polygoncoord3;
    struct attr *color = attr_new_from_text("color", "#ff0000");

    cursor = (struct attr) {
        attr_cursor, {(void *) cursor_new(NULL, (struct attr *[]) {
            &(struct attr) {
                attr_w, {(void *) 26}
            }, &(struct attr) {
                attr_h, {(void *) 26}
            }, NULL
        })
                     }
    };
    itemgra = (struct attr) {
        attr_itemgra, {(void *) itemgra_new(&cursor, (struct attr *[]) {
            NULL
        })
                      }
    };
    cursor_add_attr(cursor.u.cursor, &itemgra);
    polygoncoord1 = (struct attr) {
        attr_coord, {(void *) coord_new_from_attrs(&polygon, (struct attr *[]) {
            &(struct attr) {
                attr_x, {(void *) -7}
            }, &(struct attr) {
                attr_y, {(void *) -10}
            }, NULL
        })
                    }
    };
    itemgra_add_attr(itemgra.u.itemgra, &polygon);
    polygoncoord1 = (struct attr) {
        attr_coord, {(void *) coord_new_from_attrs(&polygon, (struct attr *[]) {
            &(struct attr) {
                attr_x, {(void *) -7}
            }, &(struct attr) {
                attr_y, {(void *) -10}
            }, NULL
        })
                    }
    };
    element_add_attr(polygon.u.element, &polygoncoord1);
    polygoncoord2 = (struct attr) {
        attr_coord, {
            (void *) coord_new_from_attrs(&polygon, (struct attr *[]) {
                &(struct attr) {
                    attr_x, {(void *) 0}
                },
                &(struct attr) {
                    attr_y, {(void *) 12}
                },
                NULL
            })
        }
    };
    element_add_attr(polygon.u.element, &polygoncoord2);
    polygoncoord3 = (struct attr) {
        attr_coord, {
            (void *) coord_new_from_attrs(&polygon, (struct attr *[]) {
                &(struct attr) {
                    attr_x, {(void *) 7}
                },
                &(struct attr) {
                    attr_y, {(void *) -10}
                },
                NULL
            })
        }
    };
    element_add_attr(polygon.u.element, &polygoncoord3);
    attr_free(color);
    this->cursor = cursor.u.cursor;
    osd_marker_draw(this, nav);
}

static struct osd_priv * osd_marker_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs) {
    struct marker *this = g_new0(struct marker, 1);
    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_marker_init), attr_navit, this));
    return (struct osd_priv *) this;
}


struct map_priv {
    struct navit *navit;
};

struct map_rect_priv {
    struct map_priv *mpriv;
    struct item item;
    struct map_rect *route_map_rect;
    struct item *route_item;
    struct street_data *sd;
    struct coord c0;
    struct coord c_base;
    int checked;
    int idx_base;
    int idx_next;
    int idx;
    int first, last;
    int route_item_done;
    struct coord c_out;
    struct coord c_next;
    struct coord c_next_base;
    struct coord lseg[2];
    struct coord lseg_base[2];
    int lseg_done;
    int lseg_done_base;
};

static int map_route_occluded_bbox(struct map *map, struct coord_rect *bbox) {
    struct coord c[128];
    struct coord_rect r;
    int i, first = 1, ccount;
    struct map_rect *map_rect = map_rect_new(map, NULL);
    struct item *item;
    while ((item = map_rect_get_item(map_rect))) {
        ccount = item_coord_get(item, c, 128);
        if (ccount && first) {
            r.lu = c[0];
            r.rl = c[0];
            first = 0;
        }
        for (i = 0; i < ccount; i++) {
            coord_rect_extend(&r, &c[i]);
        }
    }
    map_rect_destroy(map_rect);
    if (first)
        return 0;
    *bbox = r;
    return 1;
}

static struct building {
    struct street_data *sd;
    struct coord left, right;
    struct building *next;
} *buildings;

static void map_route_occluded_buildings_free(void) {
    struct building *next, *b = buildings;
    while (b) {
        street_data_free(b->sd);
        next = b->next;
        g_free(b);
        b = next;
    }
    buildings = NULL;
}

static void map_route_occluded_get_buildings(struct mapset *mapset, struct coord_rect *r) {
    struct mapset_handle *msh = mapset_open(mapset);
    struct map *map;
    struct map_selection sel;
    struct map_rect *mr;
    struct item *item;
    struct building *b;
    sel.next = NULL;
    sel.u.c_rect = *r;
    sel.order = 18;
    sel.range.min = type_poly_building;
    sel.range.max = type_poly_building;

    map_route_occluded_buildings_free();
    while ((map = mapset_next(msh, 1))) {
        mr = map_rect_new(map, &sel);
        while ((item = map_rect_get_item(mr))) {
            if (item->type == type_poly_building) {
#if 0
                if (item->id_hi != 0x1 || item->id_lo != 0x1f69)
                    continue;
#endif
#if 0
                if (item->id_hi != 0x8)
                    continue;
#if 1
                if (item->id_lo != 0x2b3e0 && item->id_lo != 0x2ae7a && item->id_lo != 0x2af1a && item->id_lo != 0x2b348
                        && item->id_lo != 0x18bb5 && item->id_lo != 0x18ce5 && item->id_lo != 0x18a85)
#else
                if (item->id_lo != 0x18bb5 && item->id_lo != 0x18ce5 && item->id_lo != 0x18a85)
#endif
                    continue;
#endif
                b = g_new(struct building, 1);
                b->sd = street_get_data(item);
                b->next = buildings;
                buildings = b;
            }
        }
        map_rect_destroy(mr);
    }
}

FILE *debug, *debug2;

/* < 0 left, > 0 right */

static int side(struct coord *l0, struct coord *l1, struct coord *p) {
    int dxl = l1->x - l0->x;
    int dyl = l1->y - l0->y;
    int dxp = p->x - l0->x;
    int dyp = p->y - l0->y;

    return dxp * dyl - dyp * dxl;
}

static void map_route_occluded_check_buildings(struct coord *c0) {
    struct building *b = buildings;
    dbg(lvl_debug, "enter");
    int i, count;
    struct coord *c;
#if 1
    FILE *bdebug, *bldebug;
    bdebug = fopen("tstb.txt", "w");
    bldebug = fopen("tstbl.txt", "w");
#endif
    while (b) {
        c = b->sd->c;
        count = b->sd->count;
        if (c[count - 1].x == c[0].x && c[count - 1].y == c[0].y) {
#if 0
            dbg(lvl_debug,"closed");
#endif
            count--;
        }
        for (i = 0; i < count; i++) {
            if (!i || side(c0, &b->left, &c[i]) < 0) {
                b->left = c[i];
            }
            if (!i || side(c0, &b->right, &c[i]) > 0) {
                b->right = c[i];
            }
        }
#if 1
        if (bdebug) {
            fprintf(bdebug, "0x%x 0x%x type=poi_hospital", b->left.x, b->left.y);
            fprintf(bdebug, "0x%x 0x%x type=poi_hospital", b->right.x, b->right.y);
        }
        if (bldebug) {
            fprintf(bldebug, "type=street_nopass");
            fprintf(bldebug, "0x%x 0x%x", c0->x, c0->y);
            fprintf(bldebug, "0x%x 0x%x", c0->x + (b->left.x - c0->x) * 10,
                    c0->y + (b->left.y - c0->y) * 10);
            fprintf(bldebug, "type=street_nopass");
            fprintf(bldebug, "0x%x 0x%x", c0->x, c0->y);
            fprintf(bldebug, "0x%x 0x%x", c0->x + (b->right.x - c0->x) * 10,
                    c0->y + (b->right.y - c0->y) * 10);
        }
#endif
        b = b->next;
    }
#if 1
    if (bdebug) {
        fclose(bdebug);
    }
    if (bldebug) {
        fclose(bldebug);
    }
#endif
}

static int intersect(struct coord *p1, struct coord *p2, struct coord *p3, struct coord *p4, struct coord *i) {
    double num = (p4->x - p3->x) * (p1->y - p3->y) - (p4->y - p3->y) * (p1->x - p3->x);
    double den = (p4->y - p3->y) * (p2->x - p1->x) - (p4->x - p3->x) * (p2->y - p1->y);
    if (num < 0 && den < 0) {
        num = -num;
        den = -den;
    }
    dbg(lvl_debug, "num=%f den=%f", num, den);
    if (i) {
        i->x = p1->x + (num / den) * (p2->x - p1->x) + 0.5;
        i->y = p1->y + (num / den) * (p2->y - p1->y) + 0.5;
        dbg(lvl_debug, "i=0x%x,0x%x", i->x, i->y);
        if (debug2)
            fprintf(debug2, "0x%x 0x%x type=town_label_5e3", i->x, i->y);
    }
    if (num < 0 || den < 0) {
        return -1;
    }
    if (num > den) {
        return 257;
    }
    return 256 * num / den;
}

/* return
    0=Not clipped
    1=Start clipped
    2=End clipped
    3=Both clipped
    4=Invisible
*/

/* #define DEBUG_VISIBLE */

static int is_visible_line(struct coord *c0, struct coord *c1, struct coord *c2) {
    int res, ret = 0;
    struct building *b = buildings;
    struct coord cn;
#ifdef DEBUG_VISIBLE
    dbg(lvl_debug,"enter");
#endif
    while (b) {
        if (side(&b->left, &b->right, c1) < 0 || side(&b->left, &b->right, c2) < 0) {
#ifdef DEBUG_VISIBLE
            dbg(lvl_debug,"sides left: start %d end %d right: start %d end %d", side(c0, &b->left, c1),
                side(c0, &b->left, c2), side(c0, &b->right, c1), side(c0, &b->right, c2));
#endif
            for (;;) {
                if (side(c0, &b->left, c1) <= 0) {
                    if (side(c0, &b->left, c2) > 0) {
#ifdef DEBUG_VISIBLE
                        dbg(lvl_debug,"visible: start is left of left corner and end is right of left corner, clipping end");
#endif
                        res = intersect(c0, &b->left, c1, c2, &cn);
                        if (res < 256) {
                            break;
                        }
                        if (c1->x != cn.x || c1->y != cn.y) {
                            *c2 = cn;
                            ret |= 2;
                            break;
                        }
                    } else
                        break;
                }
                if (side(c0, &b->right, c1) >= 0) {
                    if (side(c0, &b->right, c2) < 0) {
#ifdef DEBUG_VISIBLE
                        dbg(lvl_debug,"visible: start is right of right corner and end is left of right corner, clipping end");
#endif
                        res = intersect(c0, &b->right, c1, c2, &cn);
                        if (res < 256) {
                            break;
                        }
                        if (c1->x != cn.x || c1->y != cn.y) {
                            *c2 = cn;
                            ret |= 2;
                            break;
                        }
                    } else
                        break;
                }
                if (side(c0, &b->left, c2) <= 0) {
                    if (side(c0, &b->left, c1) > 0) {
#ifdef DEBUG_VISIBLE
                        dbg(lvl_debug,"visible: end is left of left corner and start is right of left corner, clipping start");
#endif
                        res = intersect(c0, &b->left, c1, c2, &cn);
                        if (res < 256) {
                            break;
                        }
                        if (c2->x != cn.x || c2->y != cn.y) {
                            *c1 = cn;
                            ret |= 1;
                            break;
                        }
                    } else
                        break;
                }
                if (side(c0, &b->right, c2) >= 0) {
                    if (side(c0, &b->right, c1) < 0) {
#ifdef DEBUG_VISIBLE
                        dbg(lvl_debug,"visible: end is right of right corner and start is left of right corner, clipping start");
#endif
                        res = intersect(c0, &b->right, c1, c2, &cn);
                        if (res < 256)
                            break;
                        if (c2->x != cn.x || c2->y != cn.y) {
                            *c1 = cn;
                            ret |= 1;
                            break;
                        }
                    } else
                        break;
                }
#ifdef DEBUG_VISIBLE
                dbg(lvl_debug,"visible: not visible");
#endif
                return 4;
            }
        }
        b = b->next;
    }
#ifdef DEBUG_VISIBLE
    dbg(lvl_debug,"return %d",ret);
#endif
    return ret;
}

static void map_route_occluded_coord_rewind(void *priv_data) {
    struct map_rect_priv *mr = priv_data;
    dbg(lvl_debug, "enter");
    mr->idx = mr->idx_base;
    mr->first = 1;
    mr->lseg_done = mr->lseg_done_base;
    mr->c_next = mr->c_next_base;
    mr->lseg[0] = mr->lseg_base[0];
    mr->lseg[1] = mr->lseg_base[1];
    mr->last = 0;
    item_coord_rewind(mr->route_item);
}

static void map_route_occluded_attr_rewind(void *priv_data) {
    struct map_rect_priv *mr = priv_data;
    dbg(lvl_debug, "enter\n");
    item_attr_rewind(mr->route_item);
}

static int map_route_occluded_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr) {
    struct map_rect_priv *mr = priv_data;
    dbg(lvl_debug, "enter\n");
    return item_attr_get(mr->route_item, attr_type, attr);
}

static int coord_next(struct map_rect_priv *mr, struct coord *c) {
    if (mr->idx >= mr->sd->count) {
        return 1;
    }
    *c = mr->sd->c[mr->idx++];
    return 0;
}

#define DEBUG_COORD_GET

static int map_route_occluded_coord_get(void *priv_data, struct coord *c, int count) {
    struct map_rect_priv *mr = priv_data;
    struct coord l0, l1;
    int vis, ret = 0;
    char buffer[4096];

#ifdef DEBUG_COORD_GET
    dbg(lvl_debug, "enter");
#endif
    dbg_assert(count >= 2);
    if (!mr->checked) {
        mr->c0 = mr->sd->c[0];
        map_route_occluded_check_buildings(&mr->c0);
        mr->checked = 1;
    }
    while (ret < count && !mr->last) {
#ifdef DEBUG_COORD_GET
        dbg(lvl_debug, "coord first %d lseg_done %d", mr->first, mr->lseg_done);
#endif
        if (mr->lseg_done) {
#ifdef DEBUG_COORD_GET
            dbg(lvl_debug, "loading %d of %d from id_lo %d", mr->idx, mr->sd->count,
                mr->sd->item.id_lo);
#endif
            if (!mr->idx) {
                if (coord_next(mr, &mr->lseg[0])) {
                    mr->route_item_done = 1;
                    mr->last = 1;
                    mr->idx_base = mr->idx = 0;
                    break;
                }
                mr->first = 1;
            } else
                mr->lseg[0] = mr->lseg[1];
            if (coord_next(mr, &mr->lseg[1])) {
                mr->route_item_done = 1;
                mr->last = 1;
                mr->idx_base = mr->idx = 0;
                break;
            }
            mr->c_next = mr->lseg[0];
            mr->lseg_done = 0;
        }
        l0 = mr->c_next;
        l1 = mr->lseg[1];
#ifdef DEBUG_COORD_GET
        dbg(lvl_debug, "line (0x%x,0x%x)-(0x%x,0x%x)", l0.x, l0.y, l1.x, l1.y);
#endif
        vis = is_visible_line(&mr->c0, &l0, &l1);
        if (debug) {
            fprintf(debug, "type=tracking_%d debug=\"%s\"\n", vis * 20, buffer);
            fprintf(debug, "0x%x 0x%x", l0.x, l0.y);
            fprintf(debug, "0x%x 0x%x", l1.x, l1.y);
        }
#ifdef DEBUG_COORD_GET
        dbg(lvl_debug, "vis=%d line (0x%x,0x%x)-(0x%x,0x%x)", vis, l0.x, l0.y, l1.x, l1.y);
#endif
        mr->idx_base = mr->idx;
        mr->c_next_base = mr->c_next;
        mr->lseg_base[0] = mr->lseg[0];
        mr->lseg_base[1] = mr->lseg[1];
        mr->lseg_done_base = mr->lseg_done;
        switch (vis) {
        case 0:
            mr->c_next_base = mr->c_next;
#ifdef DEBUG_COORD_GET
            dbg(lvl_debug, "out 0x%x,0x%x", l0.x, l1.y);
#endif
            c[ret++] = l0;
#ifdef DEBUG_COORD_GET
            dbg(lvl_debug, "out 0x%x,0x%x", l1.x, l1.y);
#endif
            c[ret++] = l1;
            mr->lseg_done_base = mr->lseg_done = 1;
            mr->last = 1;
            break;
        case 1:
#ifdef DEBUG_COORD_GET
            dbg(lvl_debug, "begin clipped");
            dbg(lvl_debug, "out 0x%x,0x%x", l0.x, l1.y);
#endif
            c[ret++] = l0;
#ifdef DEBUG_COORD_GET
            dbg(lvl_debug, "out 0x%x,0x%x", l1.x, l1.y);
#endif
            c[ret++] = l1;
            mr->c_next_base = mr->c_next = l1;
            mr->last = 1;
            break;
        case 2:
#ifdef DEBUG_COORD_GET
            dbg(lvl_debug, "end clipped");
#endif
        case 3:
#ifdef DEBUG_COORD_GET
            if (vis == 3) {
                dbg(lvl_debug, "both clipped");
            }
#endif
            mr->c_next_base = mr->c_next;
#ifdef DEBUG_COORD_GET
            dbg(lvl_debug, "out 0x%x,0x%x", l0.x, l1.y);
#endif
            c[ret++] = l0;
#ifdef DEBUG_COORD_GET
            dbg(lvl_debug, "out 0x%x,0x%x", l1.x, l1.y);
#endif
            c[ret++] = l1;
            mr->c_next_base = mr->c_next = l1;
            mr->last = 1;
            break;
        case 4:
            mr->last = 1;
            mr->lseg_done_base = mr->lseg_done = 1;
            break;

#if 0
        case 2:
            dbg(lvl_debug,"visible up to 0x%x,0x%x",l1.x,l1.y);
            if (mr->first) {
                mr->first=0;
                c[ret++]=mr->c_out;
                dbg(lvl_debug,"out 0x%x,0x%x", mr->c_out.x, mr->c_out.y);
            }
            c[ret++]=mr->c_out=l1;
            dbg(lvl_debug,"out 0x%x,0x%x", l1.x, l1.y);
            mr->last=1;
            mr->route_item_done=1;
            break;
        case 1:
        case 3:
        case 4:
            dbg(lvl_debug,"invisible");
            mr->c_out=l1;
            mr->idx++;
            mr->last=1;
            mr->route_item_done=1;
            break;
        }
        if (!vis)
            break;
#endif
    default:
        break;
    }
}
#ifdef DEBUG_COORD_GET
dbg(lvl_debug, "ret=%d last=%d", ret, mr->last);
#endif
return ret;
}

static struct item_methods methods_route_occluded_item = {
    map_route_occluded_coord_rewind,
    map_route_occluded_coord_get,
    map_route_occluded_attr_rewind,
    map_route_occluded_attr_get,
};

static void map_route_occluded_destroy(struct map_priv *priv) {
    g_free(priv);
}

static int no_recurse;

static struct map_rect_priv * map_route_occluded_rect_new(struct map_priv *priv, struct map_selection *sel) {
    struct map_rect_priv *mr;
    struct attr route;
    struct attr route_map;
    struct map_rect *route_map_rect;
    struct coord_rect r;
    if (!navit_get_attr(priv->navit, attr_route, &route, NULL)) {
        dbg(lvl_debug, "no route in navit");
        return NULL;
    }
    if (!route_get_attr(route.u.route, attr_map, &route_map, NULL)) {
        dbg(lvl_debug, "no map in route");
        return NULL;
    }
    route_map_rect = map_rect_new(route_map.u.map, sel);
    if (!route_map_rect) {
        dbg(lvl_debug, "no route map rect");
        return NULL;
    }
    map_dump_file(route_map.u.map, "route.txt");
    mr = g_new0(struct map_rect_priv, 1);
    mr->route_map_rect = route_map_rect;
    mr->mpriv = priv;
    mr->item.priv_data = mr;
    mr->item.meth = &methods_route_occluded_item;
    mr->item.id_lo = -1;
    mr->route_item_done = 1;
    mr->lseg_done_base = 1;
    mr->last = 1;
    if (!no_recurse && map_route_occluded_bbox(route_map.u.map, &r)) {
        struct attr mapset;
        no_recurse++;
        if (navit_get_attr(mr->mpriv->navit, attr_mapset, &mapset, NULL)) {
            map_route_occluded_get_buildings(mapset.u.mapset, &r);
        }
        debug = fopen("tst.txt", "w");
        debug2 = fopen("tstp.txt", "w");
        no_recurse--;
    }
    return mr;
}


static void map_route_occluded_rect_destroy(struct map_rect_priv *mr) {
    map_rect_destroy(mr->route_map_rect);
    street_data_free(mr->sd);
    g_free(mr);
    if (!no_recurse) {
        if (debug) {
            fclose(debug);
            debug = NULL;
        }
        if (debug2) {
            fclose(debug2);
            debug2 = NULL;
        }
    }
#if 0
    static int in_dump;
    if (! in_dump) {
        in_dump=1;
        map_dump_file(global_map,"route.txt");
        in_dump=0;
    }
#endif
}

static struct item *map_route_occluded_get_item(struct map_rect_priv *mr) {
    dbg(lvl_debug, "enter last=%d", mr->last);
    while (!mr->last) {
        struct coord c[128];
        map_route_occluded_coord_get(mr, c, 128);
    }
    if (mr->route_item_done) {
        dbg(lvl_debug, "next route item");
        do {
            mr->route_item = map_rect_get_item(mr->route_map_rect);
        } while (mr->route_item && mr->route_item->type != type_street_route);
        dbg(lvl_debug, "item %p", mr->route_item);
        if (!mr->route_item) {
            return NULL;
        }
        mr->item.type = type_street_route_occluded;
        street_data_free(mr->sd);
        mr->sd = street_get_data(mr->route_item);
        mr->route_item_done = 0;
    }
    mr->item.id_lo++;
#if 0
    if (mr->item.id_lo > 20)
        return NULL;
#endif
    map_route_occluded_coord_rewind(mr);
    dbg(lvl_debug, "type %s", item_to_name(mr->route_item->type));
    return &mr->item;
}

static struct item * map_route_occluded_get_item_byid(struct map_rect_priv *mr, int id_hi, int id_lo) {
    struct item *ret = NULL;
    while (id_lo-- > 0) {
        ret = map_route_occluded_get_item(mr);
    }
    return ret;
}

static struct map_methods map_route_occluded_methods = {
    projection_mg,
    "utf-8",
    map_route_occluded_destroy,
    map_route_occluded_rect_new,
    map_route_occluded_rect_destroy,
    map_route_occluded_get_item,
    map_route_occluded_get_item_byid,
    NULL,
    NULL,
    NULL,
};


static struct map_priv *map_route_occluded_new(struct map_methods *meth, struct attr **attrs) {
    struct map_priv *ret;
    struct attr *navit;
    dbg(lvl_debug, "enter\n");
    navit = attr_search(attrs, attr_navit);
    if (!navit) {
        return NULL;
    }
    ret = g_new0(struct map_priv, 1);
    *meth = map_route_occluded_methods;
    ret->navit = navit->u.navit;
    dbg(lvl_debug, "m = %p navit = %p", ret, ret->navit);
    return ret;
}

static void pedestrian_graphics_resize(struct graphics *gra, int w, int h) {
#ifndef HAVE_API_ANDROID
    static int done;
    if (!done) {
        int id=(int)graphics_get_data(gra, "xwindow_id");
        char buffer[1024];
        sprintf(buffer,"testxv %d &",id);
        system(buffer);
        done=1;
    }
#endif
    pedestrian_data.w = w;
    pedestrian_data.h = h;
}


static void pedestrian_draw_arrow(struct graphics *gra, char *name, int x, int y) {
    char *src = graphics_icon_path(name);
    struct graphics_image *img = graphics_image_new(gra, src);
    struct graphics_gc *gc = graphics_gc_new(gra);
    struct color col = {0xffff, 0xffff, 0xffff, 0xffff};
    struct point p;
    graphics_gc_set_foreground(gc, &col);
    p.x = x;
    p.y = y;
    graphics_draw_image(gra, gc, &p, img);
    graphics_image_free(gra, img);
    graphics_gc_destroy(gc);
    g_free(src);
}

static void pedestrian_draw_arrows(struct graphics *gra) {
    struct attr route, route_map;
    struct map_rect *route_map_rect;
    struct item *item;

    if (orientation == ORIENTATION_FLAT) {
        return;
    }
    if (!navit_get_attr(pedestrian_data.nav, attr_route, &route, NULL)) {
        dbg(lvl_debug, "no route in navit");
        return;
    }
    if (!route_get_attr(route.u.route, attr_map, &route_map, NULL)) {
        dbg(lvl_debug, "no map in route");
        return;
    }
    route_map_rect = map_rect_new(route_map.u.map, NULL);
    if (!route_map_rect) {
        dbg(lvl_debug, "no route map rect");
        return;
    }
    while ((item = map_rect_get_item(route_map_rect))) {
        if (item->type == type_street_route) {
            struct coord c[2];
            if (item_coord_get(item, c, 2) == 2) {
                struct coord *center = transform_get_center(navit_get_trans(pedestrian_data.nav));
                int angle = transform_get_angle_delta(center, &c[1], 0);
                angle -= pedestrian_data.yaw;
                if (angle < 0) {
                    angle += 360;
                }
                if (angle >= 360) {
                    angle -= 360;
                }
                if (angle > 180 && angle < 350) {
                    pedestrian_draw_arrow(gra, "gui_arrow_left_32_32.png", 0,
                                          pedestrian_data.h / 2 - 16);
                }
                if (angle > 10 && angle <= 180) {
                    pedestrian_draw_arrow(gra, "gui_arrow_right_32_32.png", pedestrian_data.w - 32,
                                          pedestrian_data.h / 2 - 16);
                }
            }
            break;
        }
    }
    map_rect_destroy(route_map_rect);
}

static void pedestrian_graphics_postdraw(struct graphics *gra) {
#if 0
    struct graphics_gc * gc = graphics_gc_new(gra);
    struct point p;
    struct color c = {0xadad,0xd8d8,0xe6e6,0xffff};
    p.x=0;
    p.y=0;
    graphics_gc_set_foreground(gc, &c);
    graphics_draw_rectangle(gra, gc, &p, 1000, 200);
    graphics_gc_destroy(gc);
#endif
    pedestrian_draw_arrows(gra);
}

#ifndef HAVE_API_ANDROID
struct tilt_data {
    int len,axis;
    char buffer[32];
};

void pedestrian_write_tilt(int fd, int axis) {
    char *buffer="01";
    int ret;

    ret=write(fd, buffer+axis, 1);
    if (ret != 2) {
        dbg(lvl_debug,"ret=%dn",ret);
    }
}


void pedestrian_read_tilt(int fd, struct navit *nav, struct tilt_data *data) {
    int val,ret,maxlen=3;
    ret=read(fd, data->buffer+data->len, maxlen-data->len);
    if (ret > 0) {
        data->len+=ret;
        data->buffer[data->len]='\0';
    }
    if (data->len == 3) {
        struct attr attr;
        sscanf(data->buffer,"%02x",&val);
        data->len=0;
        if (navit_get_attr(nav, attr_transformation, &attr, NULL)) {
            struct transformation *trans=attr.u.transformation;
            dbg(lvl_debug,"ok axis=%d val=0x%x", data->axis, val);
            if (data->axis != 1) {
                transform_set_pitch(trans, 90+(val-0x80));
            } else {
                transform_set_roll(trans, 0x80-val);
            }
        }
        data->axis=1-data->axis;

#if 0
        pedestrian_write_tilt(fd, data->axis);
#endif
    }
}

void pedestrian_write_tilt_timer(int fd, struct tilt_data *data) {
    pedestrian_write_tilt(fd, data->axis);
}

void pedestrian_setup_tilt(struct navit *nav) {
    int fd,on=1;
    struct termios t;
    struct callback *cb,*cbt;
    struct tilt_data *data=g_new0(struct tilt_data, 1);
    char buffer[32];
    fd=open("/dev/tiltsensor",O_RDWR);
    if (fd == -1) {
        dbg(lvl_error,"Failed to set up tilt sensor, error %d",errno);
        return;
    }
    tcgetattr(fd, &t);
    cfmakeraw(&t);
    cfsetspeed(&t, B19200);
    tcsetattr(fd, TCSANOW, &t);
    ioctl(fd, FIONBIO, &on);
    cb=callback_new_3(callback_cast(pedestrian_read_tilt), fd, nav, data);
    cbt=callback_new_2(callback_cast(pedestrian_write_tilt_timer), fd, data);
    event_add_watch(fd, event_watch_cond_read, cb);
    event_add_timeout(300, 1, cbt);
    read(fd, buffer, 32);
#if 0
    pedestrian_write_tilt(fd, 0);
#endif
}

#else


float sensors[2][3];

static void android_sensors(struct navit *nav, int sensor, float *x, float *y, float *z) {
    int yaw = 0, pitch = 0;
    struct attr attr;
    sensors[sensor - 1][0] = *x;
    sensors[sensor - 1][1] = *y;
    sensors[sensor - 1][2] = *z;
    if (sensors_locked) {
        return;
    }
    dbg(lvl_debug, "enter %d %f %f %f\n", sensor, *x, *y, *z);
    if (sensor == TYPE_ACCELEROMETER) {
        if (*x > 7.5) {
            orientation = ORIENTATION_LANDSCAPE;
        }
        if (*y > 7.5) {
            orientation = ORIENTATION_PORTRAIT;
        }
        if (*z > 7.5) {
            orientation = ORIENTATION_FLAT;
        }
        dbg(lvl_debug, "orientation = %d\n", orientation);
    }
    if ((orientation_old != orientation)) {
        struct attr attr, flags_graphics, osd_configuration;
        navit_set_attr(nav, orientation == ORIENTATION_FLAT ? &initial_layout : &main_layout);
        navit_get_attr(nav, attr_transformation, &attr, NULL);
        transform_set_scale(attr.u.transformation, orientation == ORIENTATION_FLAT ? 64 : 16);
        flags_graphics.type = attr_flags_graphics;
        flags_graphics.u.num = orientation == ORIENTATION_FLAT ? 0 : 10;
        navit_set_attr(nav, &flags_graphics);
        osd_configuration.type = attr_osd_configuration;
        osd_configuration.u.num = orientation == ORIENTATION_FLAT ? 1 : 2;
        navit_set_attr(nav, &osd_configuration);
        orientation_old = orientation;
    }

    switch (orientation) {
    case ORIENTATION_FLAT:
        if (sensor == TYPE_MAGNETIC_FIELD) {
            yaw = (int) (atan2f(-*y, -*x) * 180 / M_PI + 180);
        }
        pitch = 0;
        break;
    case ORIENTATION_LANDSCAPE:
        if (sensor == TYPE_ACCELEROMETER) {
            pitch = (int) (atan2f(*x, *z) * 180 / M_PI);
        }
        if (sensor == TYPE_MAGNETIC_FIELD) {
            yaw = (int) (atan2f(-*y, *z) * 180 / M_PI + 180);
        }
        break;
    case ORIENTATION_PORTRAIT:
        if (sensor == TYPE_ACCELEROMETER) {
            pitch = (int) (atan2f(*y, *z) * 180 / M_PI);
        }
        if (sensor == TYPE_MAGNETIC_FIELD) {
            yaw = (int) (atan2f(*x, *z) * 180 / M_PI + 180);
        }
        break;
    default:
        break;
    }
    if (navit_get_attr(nav, attr_transformation, &attr, NULL)) {
        struct transformation *trans = attr.u.transformation;
        if (sensor == TYPE_ACCELEROMETER) {
            if (orientation != ORIENTATION_FLAT) {
                pitch += 2.0;
            }
            transform_set_pitch(trans, pitch);
            dbg(lvl_debug, "pich %d %i", orientation, pitch);
        } else {
            struct attr attr;
            attr.type = attr_orientation;
            attr.u.num = yaw - 1;
            if (attr.u.num < 0)
                attr.u.num += 360;
            pedestrian_data.yaw = (int) attr.u.num;
            navit_set_attr(nav, &attr);
            dbg(lvl_debug, "yaw %d %i", orientation, yaw);
            if (orientation == ORIENTATION_FLAT) {
                navit_set_center_cursor(nav, 1, 0);
            }
        }
    }
}

#endif

static void pedestrian_log(char **logstr) {
#ifdef HAVE_API_ANDROID
    char *tag = g_strdup_printf(
                    "\t\t<navit:compass:x>%f</navit:compass:x>\n"
                    "\t\t<navit:compass:y>%f</navit:compass:y>\n"
                    "\t\t<navit:compass:z>%f</navit:compass:z>\n"
                    "\t\t<navit:accel:x>%f</navit:accel:x>\n"
                    "\t\t<navit:accel:y>%f</navit:accel:y>\n"
                    "\t\t<navit:accel:z>%f</navit:accel:z>\n",
                    sensors[0][0], sensors[0][1], sensors[0][2],
                    sensors[1][0], sensors[1][1], sensors[1][2]);
    vehicle_log_gpx_add_tag(tag, logstr);
#endif
}

#ifdef DEMO
static void vehicle_changed(struct vehicle *v, struct transformation *trans) {
    struct attr attr;
    if (vehicle_get_attr(v, attr_position_direction, &attr, NULL)) {
        int dir=(int)(*attr.u.numd);
        dbg(lvl_debug,"enter %d\n",dir);
        transform_set_pitch(trans, 90);
        transform_set_yaw(trans, dir);
    }
}
#endif


static void pedestrian_navit_init(struct navit *nav) {
    struct attr route;
    struct attr route_map;
    struct attr map;
    struct attr mapset;
    struct attr graphics, attr, flags_graphics;
    struct transformation *trans;
    struct attr_iter *iter;

#ifdef HAVE_API_ANDROID
    struct callback *cb;
    jclass navitsensorsclass;
    jmethodID cid;
    jobject navitsensors;

    dbg(lvl_debug, "enter\n");
    orientation = ORIENTATION_UNKNOWN;
    orientation_old = ORIENTATION_UNKNOWN;
    if (android_find_class_global("org/navitproject/navit/NavitSensors", &navitsensorsclass)) {
        dbg(lvl_debug, "class found\n");
        cid = (*jnienv)->GetMethodID(jnienv, navitsensorsclass, "<init>",
                                     "(Landroid/content/Context;J)V");
        dbg(lvl_debug, "cid=%p\n", cid);
        if (cid) {
            cb = callback_new_1(callback_cast(android_sensors), nav);
            navitsensors = (*jnienv)->NewObject(jnienv, navitsensorsclass, cid, android_activity, cb);
            dbg(lvl_debug, "object=%p\n", navitsensors);
            if (navitsensors) {
                navitsensors = (*jnienv)->NewGlobalRef(jnienv, navitsensors);
            }
        }
    }
#endif
    pedestrian_data.nav = nav;
    flags_graphics.type = attr_flags_graphics;
    flags_graphics.u.num = 10;
    navit_set_attr(nav, &flags_graphics);
    if (navit_get_attr(nav, attr_graphics, &graphics, NULL)) {
        struct attr attr;
        struct callback *cb = callback_new_attr_1(callback_cast(pedestrian_graphics_resize),
                              attr_resize, graphics.u.graphics);
        graphics_add_callback(graphics.u.graphics, cb);
        cb = callback_new_attr_1(callback_cast(pedestrian_graphics_postdraw), attr_postdraw, graphics.u.graphics);
        graphics_add_callback(graphics.u.graphics, cb);
        attr.type = attr_use_camera;
        attr.u.num = 1;
        graphics_set_attr(graphics.u.graphics, &attr);
    }
    osd_rocket_init(nav);
#if 1
#ifndef HAVE_API_ANDROID
    pedestrian_setup_tilt(nav);
#endif
    trans = navit_get_trans(nav);
    transform_set_pitch(trans, 90);
    transform_set_roll(trans, 0);
    transform_set_hog(trans, 2);
    transform_set_distance(trans, 0);
    transform_set_scales(trans, 750, 620, 32 << 8);
    if (!navit_get_attr(nav, attr_route, &route, NULL))
        return;
    if (!route_get_attr(route.u.route, attr_map, &route_map, NULL))
        return;
    dbg(lvl_debug, "enter 1\n");
#if 0
    struct attr active;
    active.type=attr_active;
    active.u.num=0;
    if (!map_set_attr(route_map.u.map, &active))
        return;
    dbg(lvl_debug,"enter 2\n");
#endif
    if (!navit_get_attr(nav, attr_mapset, &mapset, NULL))
        return;
    map.type = attr_map;
    map.u.map = map_new(NULL, (struct attr *[]) {
        &(struct attr) {
            attr_type, {"route_occluded"}
        }, &(struct attr) {
            attr_data, {""}
        },
        &(struct attr) {
            attr_description, {"Occluded Route"}
        },
        &(struct attr) {
            attr_navit, {(void *) nav}
        }, NULL
    });
    global_map = map.u.map;
    mapset_add_attr(mapset.u.mapset, &map);

#if 0
    map=route_get_attr(route, attr_map);
#endif
#endif
    transform_set_scale(trans, 16);
    navit_get_attr(nav, attr_layout, &initial_layout, NULL);
    iter = navit_attr_iter_new(NULL);
    while (navit_get_attr(nav, attr_layout, &attr, iter)) {
        if (!strcmp(attr.u.layout->name, "Route")) {
            dbg(lvl_debug, "found %s", attr_to_name(attr.type));
            main_layout = attr;
#if 1
            navit_set_attr(nav, &attr);
#endif
            break;
        }
    }
    navit_attr_iter_destroy(iter);
    if (navit_get_attr(nav, attr_vehicle, &attr, NULL)) {
        struct attr cbattr;
        cbattr.u.callback = callback_new_attr_0(callback_cast(pedestrian_log), attr_log_gpx);
        cbattr.type = attr_callback;
        vehicle_add_attr(attr.u.vehicle, &cbattr);
#ifdef DEMO
        cbattr.u.callback=callback_new_attr_2(callback_cast(vehicle_changed), attr_position_coord_geo, attr.u.vehicle, trans);
        cbattr.type=attr_callback;
        vehicle_add_attr(attr.u.vehicle, &cbattr);
#endif
    }

}

static void pedestrian_navit(struct navit *nav, int add) {
    dbg(lvl_debug, "enter\n");
    struct attr callback;
    if (add) {
        callback.type = attr_callback;
        callback.u.callback = callback_new_attr_0(callback_cast(pedestrian_navit_init), attr_navit);
        navit_add_attr(nav, &callback);
    }
}

void plugin_init(void) {
    struct attr callback, navit;
    struct attr_iter *iter;
#ifdef HAVE_API_ANDROID
    jclass ActivityClass;
    jmethodID Activity_setRequestedOrientation;

    if (!android_find_class_global("android/app/Activity", &ActivityClass)) {
        dbg(lvl_error, "failed to get class android/app/Activity\n");
    }
    Activity_setRequestedOrientation = (*jnienv)->GetMethodID(jnienv, ActivityClass,
                                       "setRequestedOrientation", "(I)V");
    if (Activity_setRequestedOrientation == NULL) {
        dbg(lvl_error, "failed to get method setRequestedOrientation from android/app/Activity\n");
    }
    (*jnienv)->CallVoidMethod(jnienv, android_activity, Activity_setRequestedOrientation, 0);
#endif

    plugin_register_category_osd("marker", osd_marker_new);
    plugin_register_category_map("route_occluded", map_route_occluded_new);
    callback.type = attr_callback;
    callback.u.callback = callback_new_attr_0(callback_cast(pedestrian_navit), attr_navit);
    config_add_attr(config, &callback);
    iter = config_attr_iter_new(NULL);
    while (config_get_attr(config, attr_navit, &navit, iter)) {
        pedestrian_navit_init(navit.u.navit);
    }
    config_attr_iter_destroy(iter);
}

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

#include <unistd.h>
#include <glib.h>
#include <poll.h>
#include "config.h"
#include "window.h"
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "item.h"
#include "xmlconfig.h"
#include "plugin.h"
#include "event.h"
#include "debug.h"
#include "callback.h"
#include "android.h"
#include "command.h"

struct graphics_priv {
    jclass NavitGraphicsClass;
    jmethodID NavitGraphics_draw_polyline, NavitGraphics_draw_polygon, NavitGraphics_draw_rectangle,
              NavitGraphics_draw_circle, NavitGraphics_draw_text, NavitGraphics_draw_image,
              NavitGraphics_draw_image_warp, NavitGraphics_draw_mode, NavitGraphics_draw_drag,
              NavitGraphics_overlay_disable, NavitGraphics_overlay_resize, NavitGraphics_SetCamera,
              NavitGraphics_setBackgroundColor, NavitGraphics_draw_polygon_with_holes;

    jclass PaintClass;
    jmethodID Paint_init,Paint_setStrokeWidth,Paint_setARGB;

    jobject NavitGraphics;
    jobject Paint;

    jclass BitmapFactoryClass;
    jmethodID BitmapFactory_decodeFile, BitmapFactory_decodeResource;

    jclass BitmapClass;
    jmethodID Bitmap_getHeight, Bitmap_getWidth, Bitmap_createScaledBitmap;

    jclass ContextClass;
    jmethodID Context_getResources;

    jclass ResourcesClass;
    jobject Resources;
    jmethodID Resources_getIdentifier;

    jobject packageName;

    struct callback_list *cbl;
    struct window win;
    struct padding *padding;
    jint bgcolor;
};

struct graphics_font_priv {
    int size;
};

struct graphics_gc_priv {
    struct graphics_priv *gra;
    int linewidth;
    enum draw_mode_num mode;
    int a,r,g,b;
    unsigned char *dashes;
    int ndashes;
};

struct graphics_image_priv {
    jobject Bitmap;
    int width;
    int height;
    struct point hot;
};

static GHashTable *image_cache_hash = NULL;

static int find_class_global(char *name, jclass *ret) {
    *ret=(*jnienv)->FindClass(jnienv, name);
    if (! *ret) {
        dbg(lvl_error,"Failed to get Class %s",name);
        return 0;
    }
    *ret = (*jnienv)->NewGlobalRef(jnienv, *ret);
    return 1;
}

static int find_method(jclass class, char *name, char *args, jmethodID *ret) {
    *ret = (*jnienv)->GetMethodID(jnienv, class, name, args);
    if (*ret == NULL) {
        dbg(lvl_error,"Failed to get Method %s with signature %s",name,args);
        return 0;
    }
    return 1;
}

static int find_static_method(jclass class, char *name, char *args, jmethodID *ret) {
    *ret = (*jnienv)->GetStaticMethodID(jnienv, class, name, args);
    if (*ret == NULL) {
        dbg(lvl_error,"Failed to get static Method %s with signature %s",name,args);
        return 0;
    }
    return 1;
}

static void graphics_destroy(struct graphics_priv *gr) {
}

static void font_destroy(struct graphics_font_priv *font) {
    g_free(font);
}

static struct graphics_font_methods font_methods = {
    font_destroy
};

static struct graphics_font_priv *font_new(struct graphics_priv *gr, struct graphics_font_methods *meth, char *font,
        int size, int flags) {
    struct graphics_font_priv *ret=g_new0(struct graphics_font_priv, 1);
    *meth=font_methods;

    ret->size=size;
    return ret;
}

static void gc_destroy(struct graphics_gc_priv *gc) {
    g_free(gc->dashes);
    g_free(gc);
}

static void gc_set_linewidth(struct graphics_gc_priv *gc, int w) {
    gc->linewidth = w;
}

static void gc_set_dashes(struct graphics_gc_priv *gc, int w, int offset, unsigned char *dash_list, int n) {
    g_free(gc->dashes);
    gc->ndashes=n;
    if(n) {
        gc->dashes=g_malloc(n);
        memcpy(gc->dashes, dash_list, n);
    } else {
        gc->dashes=NULL;
    }
}

static void gc_set_foreground(struct graphics_gc_priv *gc, struct color *c) {
    gc->r = c->r >> 8;
    gc->g = c->g >> 8;
    gc->b = c->b >> 8;
    gc->a = c->a >> 8;
}

static void gc_set_background(struct graphics_gc_priv *gc, struct color *c) {
}

static struct graphics_gc_methods gc_methods = {
    gc_destroy,
    gc_set_linewidth,
    gc_set_dashes,
    gc_set_foreground,
    gc_set_background
};

static struct graphics_gc_priv *gc_new(struct graphics_priv *gr, struct graphics_gc_methods *meth) {
    struct graphics_gc_priv *ret=g_new0(struct graphics_gc_priv, 1);
    *meth=gc_methods;

    ret->gra = gr;
    ret->a = ret->r = ret->g = ret->b = 255;
    ret->linewidth=1;
    return ret;
}

static void image_destroy(struct graphics_image_priv *img) {
    // unused?
}

static struct graphics_image_methods image_methods = {
    image_destroy
};


static struct graphics_image_priv *image_new(struct graphics_priv *gra, struct graphics_image_methods *meth, char *path,
        int *w, int *h, struct point *hot, int rotation) {
    struct graphics_image_priv* ret = NULL;

    ret=g_new0(struct graphics_image_priv, 1);
    jstring string;
    jclass localBitmap = NULL;
    int id;

    dbg(lvl_debug,"enter %s",path);
    if (!strncmp(path,"res/drawable/",13)) {
        jstring a=(*jnienv)->NewStringUTF(jnienv, "drawable");
        char *path_noext=g_strdup(path+13);
        char *pos=strrchr(path_noext, '.');
        if (pos)
            *pos='\0';
        dbg(lvl_debug,"path_noext=%s",path_noext);
        string = (*jnienv)->NewStringUTF(jnienv, path_noext);
        g_free(path_noext);
        id=(*jnienv)->CallIntMethod(jnienv, gra->Resources, gra->Resources_getIdentifier, string, a, gra->packageName);
        dbg(lvl_debug,"id=%d",id);
        if (id)
            localBitmap=(*jnienv)->CallStaticObjectMethod(jnienv, gra->BitmapFactoryClass, gra->BitmapFactory_decodeResource,
                        gra->Resources, id);
        (*jnienv)->DeleteLocalRef(jnienv, a);
    } else {
        string = (*jnienv)->NewStringUTF(jnienv, path);
        localBitmap=(*jnienv)->CallStaticObjectMethod(jnienv, gra->BitmapFactoryClass, gra->BitmapFactory_decodeFile, string);
    }
    if (localBitmap) {
        ret->width=(*jnienv)->CallIntMethod(jnienv, localBitmap, gra->Bitmap_getWidth);
        ret->height=(*jnienv)->CallIntMethod(jnienv, localBitmap, gra->Bitmap_getHeight);
        if((*w!=IMAGE_W_H_UNSET && *w!=ret->width) || (*h!=IMAGE_W_H_UNSET && *w!=ret->height)) {
            jclass scaledBitmap=(*jnienv)->CallStaticObjectMethod(jnienv, gra->BitmapClass,
                                gra->Bitmap_createScaledBitmap, localBitmap, (*w==IMAGE_W_H_UNSET)?ret->width:*w, (*h==IMAGE_W_H_UNSET)?ret->height:*h,
                                JNI_TRUE);
            if(!scaledBitmap) {
                dbg(lvl_error,"Bitmap scaling to %dx%d failed for %s",*w,*h,path);
            } else {
                (*jnienv)->DeleteLocalRef(jnienv, localBitmap);
                localBitmap=scaledBitmap;
                ret->width=(*jnienv)->CallIntMethod(jnienv, localBitmap, gra->Bitmap_getWidth);
                ret->height=(*jnienv)->CallIntMethod(jnienv, localBitmap, gra->Bitmap_getHeight);
            }
        }
        ret->Bitmap = (*jnienv)->NewGlobalRef(jnienv, localBitmap);
        (*jnienv)->DeleteLocalRef(jnienv, localBitmap);

        dbg(lvl_debug,"w=%d h=%d for %s",ret->width,ret->height,path);
        ret->hot.x=ret->width/2;
        ret->hot.y=ret->height/2;
    } else {
        g_free(ret);
        ret=NULL;
        dbg(lvl_warning,"Failed to open %s",path);
    }
    (*jnienv)->DeleteLocalRef(jnienv, string);
    if (ret) {
        *w=ret->width;
        *h=ret->height;
        if (hot)
            *hot=ret->hot;
    }

    return ret;
}

static void initPaint(struct graphics_priv *gra, struct graphics_gc_priv *gc) {
    float wf = gc->linewidth;
    (*jnienv)->CallVoidMethod(jnienv, gc->gra->Paint, gra->Paint_setStrokeWidth, wf);
    (*jnienv)->CallVoidMethod(jnienv, gc->gra->Paint, gra->Paint_setARGB, gc->a, gc->r, gc->g, gc->b);
}

static void draw_lines(struct graphics_priv *gra, struct graphics_gc_priv *gc, struct point *p, int count) {
    int arrsize=1+4+1+gc->ndashes+count*2;
    jint pc[arrsize];
    int i;
    jintArray points;
    if (count <= 0)
        return;
    points = (*jnienv)->NewIntArray(jnienv,arrsize);
    pc[0]=gc->linewidth;
    pc[1]=gc->a;
    pc[2]=gc->r;
    pc[3]=gc->g;
    pc[4]=gc->b;
    pc[5]=gc->ndashes;
    for (i = 0 ; i < gc->ndashes ; i++) {
        pc[6+i] = gc->dashes[i];
    }
    for (i = 0 ; i < count ; i++) {
        pc[6+gc->ndashes+i*2]=p[i].x;
        pc[6+gc->ndashes+i*2+1]=p[i].y;
    }
    (*jnienv)->SetIntArrayRegion(jnienv, points, 0, arrsize, pc);
    (*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_polyline, gc->gra->Paint, points);
    (*jnienv)->DeleteLocalRef(jnienv, points);
}

static void draw_polygon_with_holes (struct graphics_priv *gra, struct graphics_gc_priv *gc, struct point *p, int count,
                                     int hole_count, int* ccount, struct point **holes) {
    int i;
    /* need to get us some arrays for java */
    int java_p_size;
    jintArray java_p;
    int java_ccount_size;
    jintArray java_ccount;
    int java_holes_size;
    jintArray java_holes;

    /* Don't even try to draw a polygon with less than 3 points */
    if(count < 3)
        return;

    /* get java array for coordinates */
    java_p_size=count*2;
    java_p = (*jnienv)->NewIntArray(jnienv,java_p_size);
    jint j_p[java_p_size];
    for (i = 0 ; i < count ; i++) {
        j_p[i*2]=p[i].x;
        j_p[(i*2)+1]=p[i].y;
    }

    /* get java array for ccount */
    java_ccount_size = hole_count;
    java_ccount=(*jnienv)->NewIntArray(jnienv,java_ccount_size);
    jint j_ccount[java_ccount_size];
    /* get java array for hole coordinates */
    java_holes_size = 0;
    for(i=0; i < hole_count; i ++) {
        java_holes_size += ccount[i] * 2;
    }
    java_holes=(*jnienv)->NewIntArray(jnienv,java_ccount_size);
    /* copy over the holes to the jint array */
    int j_holes_used=0;
    jint j_holes[java_holes_size];
    for(i=0; i < hole_count; i ++) {
        int j;
        j_ccount[i] = ccount[i] * 2;
        for(j=0; j<ccount[i]; j ++) {
            j_holes[j_holes_used + (j * 2)] = holes[i][j].x;
            j_holes[j_holes_used + (j * 2) +1] = holes[i][j].y;
        }
        j_holes_used += j_ccount[i];
    }
    /* attach the arrays with their storage to the JVM */
    (*jnienv)->SetIntArrayRegion(jnienv, java_p, 0, java_p_size, j_p);
    (*jnienv)->SetIntArrayRegion(jnienv, java_ccount, 0, java_ccount_size, j_ccount);
    (*jnienv)->SetIntArrayRegion(jnienv, java_holes, 0, java_holes_size, j_holes);
    /* call the java function */
    (*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_polygon_with_holes, gc->gra->Paint,
                              gc->linewidth, gc->r, gc->g, gc->b, gc->a, java_p, java_ccount, java_holes);
    /* clean up */
    (*jnienv)->DeleteLocalRef(jnienv, java_holes);
    (*jnienv)->DeleteLocalRef(jnienv, java_ccount);
    (*jnienv)->DeleteLocalRef(jnienv, java_p);
}

static void draw_polygon(struct graphics_priv *gra, struct graphics_gc_priv *gc, struct point *p, int count) {
    int arrsize=1+4+count*2;
    jint pc[arrsize];
    int i;
    jintArray points;
    if (count <= 0)
        return;
    points = (*jnienv)->NewIntArray(jnienv,arrsize);
    for (i = 0 ; i < count ; i++) {
        pc[5+i*2]=p[i].x;
        pc[5+i*2+1]=p[i].y;
    }
    pc[0]=gc->linewidth;
    pc[1]=gc->a;
    pc[2]=gc->r;
    pc[3]=gc->g;
    pc[4]=gc->b;
    (*jnienv)->SetIntArrayRegion(jnienv, points, 0, arrsize, pc);
    (*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_polygon, gc->gra->Paint, points);
    (*jnienv)->DeleteLocalRef(jnienv, points);
}

static void draw_rectangle(struct graphics_priv *gra, struct graphics_gc_priv *gc, struct point *p, int w, int h) {
    initPaint(gra, gc);
    (*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_rectangle, gc->gra->Paint, p->x, p->y, w,
                              h);
}

static void draw_circle(struct graphics_priv *gra, struct graphics_gc_priv *gc, struct point *p, int r) {
    initPaint(gra, gc);
    (*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_circle, gc->gra->Paint, p->x, p->y, r);
}


static void draw_text(struct graphics_priv *gra, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg,
                      struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy) {
    int bgcolor=0;
    dbg(lvl_debug,"enter %s", text);
    initPaint(gra, fg);
    if(bg)
        bgcolor=(bg->a<<24)| (bg->r<<16) | (bg->g<<8) | bg->b;
    jstring string = (*jnienv)->NewStringUTF(jnienv, text);
    (*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_text, fg->gra->Paint, p->x, p->y, string,
                              font->size, dx, dy, bgcolor);
    (*jnienv)->DeleteLocalRef(jnienv, string);
}

static void draw_image(struct graphics_priv *gra, struct graphics_gc_priv *fg, struct point *p,
                       struct graphics_image_priv *img) {
    dbg(lvl_debug,"enter %p",img);
    initPaint(gra, fg);
    (*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_image, fg->gra->Paint, p->x, p->y,
                              img->Bitmap);

}

static void draw_image_warp (struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, int count,
                             struct graphics_image_priv *img) {

    /*
     *
     *
     * if coord count==3 then top.left top.right bottom.left
     *
     */

    if (count==3) {
        initPaint(gr, fg);
        (*jnienv)->CallVoidMethod(jnienv, gr->NavitGraphics, gr->NavitGraphics_draw_image_warp, fg->gra->Paint, count,
                                  p[0].x, p[0].y,p[1].x, p[1].y, p[2].x, p[2].y, img->Bitmap);
    } else
        dbg(lvl_debug,"draw_image_warp is called with unsupported count parameter value %d", count);
}



static void draw_drag(struct graphics_priv *gra, struct point *p) {
    (*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_drag, p ? p->x : 0, p ? p->y : 0);
}

static void background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc) {
}

static void draw_mode(struct graphics_priv *gra, enum draw_mode_num mode) {
    (*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_mode, (int)mode);
}

static struct graphics_priv * overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p,
        int w, int h, int wraparound);

static void *get_data(struct graphics_priv *this, const char *type) {
    if (!strcmp(type,"padding"))
        return this->padding;
    if (!strcmp(type,"window"))
        return &this->win;
    return NULL;
}

static void image_free(struct graphics_priv *gr, struct graphics_image_priv *priv) {
}

static void get_text_bbox(struct graphics_priv *gr, struct graphics_font_priv *font, char *text, int dx, int dy,
                          struct point *ret, int estimate) {
    int len = g_utf8_strlen(text, -1);
    int xMin = 0;
    int yMin = 0;
    int yMax = 13*font->size/256;
    int xMax = 9*font->size*len/256;

    ret[0].x = xMin;
    ret[0].y = -yMin;
    ret[1].x = xMin;
    ret[1].y = -yMax;
    ret[2].x = xMax;
    ret[2].y = -yMax;
    ret[3].x = xMax;
    ret[3].y = -yMin;
}

static void overlay_disable(struct graphics_priv *gra, int disable) {
    (*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_overlay_disable, disable);
}

static void overlay_resize(struct graphics_priv *gra, struct point *pnt, int w, int h, int wraparound) {
    (*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_overlay_resize, pnt ? pnt->x:0, pnt ? pnt->y:0,
                              w, h, wraparound);
}

static int set_attr(struct graphics_priv *gra, struct attr *attr) {
    switch (attr->type) {
    case attr_use_camera:
        (*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_SetCamera, attr->u.num);
        return 1;
    case attr_background_color:
        gra->bgcolor = (attr->u.color->a / 0x101) << 24
                       | (attr->u.color->r / 0x101) << 16
                       | (attr->u.color->g / 0x101) << 8
                       | (attr->u.color->b / 0x101);
        dbg(lvl_debug, "set attr_background_color %04x %04x %04x %04x (%08x)",
            attr->u.color->r, attr->u.color->g, attr->u.color->b, attr->u.color->a, gra->bgcolor);
        if (gra->NavitGraphics_setBackgroundColor != NULL)
            (*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_setBackgroundColor, gra->bgcolor);
        else
            dbg(lvl_error, "NavitGraphics.setBackgroundColor not found, cannot set background color");
        return 1;
    default:
        return 0;
    }
}


int show_native_keyboard (struct graphics_keyboard *kbd);

void hide_native_keyboard (struct graphics_keyboard *kbd);


static struct graphics_methods graphics_methods = {
    graphics_destroy,
    draw_mode,
    draw_lines,
    draw_polygon,
    draw_rectangle,
    draw_circle,
    draw_text,
    draw_image,
    draw_image_warp,
    draw_drag,
    font_new,
    gc_new,
    background_gc,
    overlay_new,
    image_new,
    get_data,
    image_free,
    get_text_bbox,
    overlay_disable,
    overlay_resize,
    set_attr,
    show_native_keyboard,
    hide_native_keyboard,
    NULL, /*get_dpi*/
    draw_polygon_with_holes
};

static void resize_callback(struct graphics_priv *gra, int w, int h) {
    dbg(lvl_debug,"w=%d h=%d ok",w,h);
    dbg(lvl_debug,"gra=%p, gra->cbl=%p", gra, gra->cbl);
    callback_list_call_attr_2(gra->cbl, attr_resize, (void *)w, (void *)h);
}

static void padding_changed_callback(struct graphics_priv *gra, int left, int top, int right,
                                     int bottom) {
    dbg(lvl_debug, "win.padding left=%d top=%d right=%d bottom=%d", left, top, right, bottom);
    gra->padding->left = left;
    gra->padding->top = top;
    gra->padding->right = right;
    gra->padding->bottom = bottom;
}

static void motion_callback(struct graphics_priv *gra, int x, int y) {
    struct point p;
    p.x=x;
    p.y=y;
    callback_list_call_attr_1(gra->cbl, attr_motion, (void *)&p);
}

static void keypress_callback(struct graphics_priv *gra, char *s) {
    dbg(lvl_debug,"enter %s",s);
    callback_list_call_attr_1(gra->cbl, attr_keypress, s);
}

static void button_callback(struct graphics_priv *gra, int pressed, int button, int x, int y) {
    struct point p;
    p.x=x;
    p.y=y;
    callback_list_call_attr_3(gra->cbl, attr_button, (void *)pressed, (void *)button, (void *)&p);
}


static int set_activity(jobject graphics) {
    jclass ActivityClass;
    jmethodID cid;

    ActivityClass = (*jnienv)->GetObjectClass(jnienv, android_activity);
    dbg(lvl_debug,"at 5");
    if (ActivityClass == NULL) {
        dbg(lvl_debug,"no activity class found");
        return 0;
    }
    dbg(lvl_debug,"at 6");
    cid = (*jnienv)->GetMethodID(jnienv, ActivityClass, "setContentView", "(Landroid/view/View;)V");
    if (cid == NULL) {
        dbg(lvl_error,"no setContentView method found");
        return 0;
    }
    dbg(lvl_debug,"at 7");
    (*jnienv)->CallVoidMethod(jnienv, android_activity, cid, graphics);
    dbg(lvl_debug,"at 8");
    return 1;
}

/**
 * @brief Initializes a new Android graphics instance.
 *
 * This initializes a new Android graphics instance, which can either be the main view or an overlay.
 *
 * @param ret The new graphics instance
 * @param parent The graphics instance that contains the new instance ({@code NULL} for the main view)
 * @param p The position of the overlay in its parent ({@code NULL} for the main view)
 * @param w The width of the overlay (0 for the main view)
 * @param h The height of the overlay (0 for the main view)
 * @param wraparound (0 for the main view)
 * @param use_camera Whether to use the camera (0 for overlays)
 */
static int graphics_android_init(struct graphics_priv *ret, struct graphics_priv *parent, struct point *pnt, int w,
                                 int h, int wraparound, int use_camera) {
    struct callback *cb;
    jmethodID cid, Context_getPackageName;

    dbg(lvl_debug,"at 2 jnienv=%p",jnienv);
    if (parent)
        ret->padding = parent->padding;
    if (!find_class_global("android/graphics/Paint", &ret->PaintClass))
        return 0;
    if (!find_method(ret->PaintClass, "<init>", "(I)V", &ret->Paint_init))
        return 0;
    if (!find_method(ret->PaintClass, "setARGB", "(IIII)V", &ret->Paint_setARGB))
        return 0;
    if (!find_method(ret->PaintClass, "setStrokeWidth", "(F)V", &ret->Paint_setStrokeWidth))
        return 0;

    if (!find_class_global("android/graphics/BitmapFactory", &ret->BitmapFactoryClass))
        return 0;
    if (!find_static_method(ret->BitmapFactoryClass, "decodeFile", "(Ljava/lang/String;)Landroid/graphics/Bitmap;",
                            &ret->BitmapFactory_decodeFile))
        return 0;
    if (!find_static_method(ret->BitmapFactoryClass, "decodeResource",
                            "(Landroid/content/res/Resources;I)Landroid/graphics/Bitmap;", &ret->BitmapFactory_decodeResource))
        return 0;

    if (!find_class_global("android/graphics/Bitmap", &ret->BitmapClass))
        return 0;
    if (!find_method(ret->BitmapClass, "getHeight", "()I", &ret->Bitmap_getHeight))
        return 0;
    if (!find_method(ret->BitmapClass, "getWidth", "()I", &ret->Bitmap_getWidth))
        return 0;
    if (!find_static_method(ret->BitmapClass, "createScaledBitmap",
                            "(Landroid/graphics/Bitmap;IIZ)Landroid/graphics/Bitmap;", &ret->Bitmap_createScaledBitmap))
        return 0;

    if (!find_class_global("android/content/Context", &ret->ContextClass))
        return 0;
    if (!find_method(ret->ContextClass, "getResources", "()Landroid/content/res/Resources;", &ret->Context_getResources))
        return 0;


    ret->Resources=(*jnienv)->CallObjectMethod(jnienv, android_activity, ret->Context_getResources);
    if (ret->Resources)
        ret->Resources = (*jnienv)->NewGlobalRef(jnienv, ret->Resources);
    if (!find_class_global("android/content/res/Resources", &ret->ResourcesClass))
        return 0;
    if (!find_method(ret->ResourcesClass, "getIdentifier", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I",
                     &ret->Resources_getIdentifier))
        return 0;

    if (!find_method(ret->ContextClass, "getPackageName", "()Ljava/lang/String;", &Context_getPackageName))
        return 0;
    ret->packageName=(*jnienv)->CallObjectMethod(jnienv, android_activity, Context_getPackageName);
    ret->packageName=(*jnienv)->NewGlobalRef(jnienv, ret->packageName);

    if (!find_class_global("org/navitproject/navit/NavitGraphics", &ret->NavitGraphicsClass))
        return 0;
    dbg(lvl_debug,"at 3");
    cid = (*jnienv)->GetMethodID(jnienv, ret->NavitGraphicsClass, "<init>",
                                 "(Landroid/app/Activity;Lorg/navitproject/navit/NavitGraphics;IIIIII)V");
    if (cid == NULL) {
        dbg(lvl_error,"no method found");
        return 0; /* exception thrown */
    }
    dbg(lvl_debug,"at 4 android_activity=%p",android_activity);
    ret->NavitGraphics=(*jnienv)->NewObject(jnienv, ret->NavitGraphicsClass, cid, android_activity,
                                            parent ? parent->NavitGraphics : NULL, pnt ? pnt->x:0, pnt ? pnt->y:0, w, h, wraparound, use_camera);
    dbg(lvl_debug,"result=%p",ret->NavitGraphics);
    if (ret->NavitGraphics)
        ret->NavitGraphics = (*jnienv)->NewGlobalRef(jnienv, ret->NavitGraphics);

    /* Create a single global Paint, otherwise android will quickly run out
     * of global refs.*/
    /* 0x101 = text kerning (default), antialiasing */
    ret->Paint=(*jnienv)->NewObject(jnienv, ret->PaintClass, ret->Paint_init, 0x101);

    dbg(lvl_debug,"result=%p",ret->Paint);
    if (ret->Paint)
        ret->Paint = (*jnienv)->NewGlobalRef(jnienv, ret->Paint);

    cid = (*jnienv)->GetMethodID(jnienv, ret->NavitGraphicsClass, "setSizeChangedCallback", "(J)V");
    if (cid == NULL) {
        dbg(lvl_error,"no setResizeCallback method found");
        return 0; /* exception thrown */
    }
    cb=callback_new_1(callback_cast(resize_callback), ret);
    (*jnienv)->CallVoidMethod(jnienv, ret->NavitGraphics, cid, (jlong)cb);

    cid = (*jnienv)->GetMethodID(jnienv, ret->NavitGraphicsClass, "setPaddingChangedCallback", "(J)V");
    if (cid == NULL) {
        dbg(lvl_error,"no setPaddingCallback method found");
        return 0; /* exception thrown */
    }
    cb=callback_new_1(callback_cast(padding_changed_callback), ret);
    (*jnienv)->CallVoidMethod(jnienv, ret->NavitGraphics, cid, (jlong)cb);

    cid = (*jnienv)->GetMethodID(jnienv, ret->NavitGraphicsClass, "setButtonCallback", "(J)V");
    if (cid == NULL) {
        dbg(lvl_error,"no setButtonCallback method found");
        return 0; /* exception thrown */
    }
    cb=callback_new_1(callback_cast(button_callback), ret);
    (*jnienv)->CallVoidMethod(jnienv, ret->NavitGraphics, cid, (jlong)cb);

    cid = (*jnienv)->GetMethodID(jnienv, ret->NavitGraphicsClass, "setMotionCallback", "(J)V");
    if (cid == NULL) {
        dbg(lvl_error,"no setMotionCallback method found");
        return 0; /* exception thrown */
    }
    cb=callback_new_1(callback_cast(motion_callback), ret);
    (*jnienv)->CallVoidMethod(jnienv, ret->NavitGraphics, cid, (jlong)cb);

    cid = (*jnienv)->GetMethodID(jnienv, ret->NavitGraphicsClass, "setKeypressCallback", "(J)V");
    if (cid == NULL) {
        dbg(lvl_error,"no setKeypressCallback method found");
        return 0; /* exception thrown */
    }
    cb=callback_new_1(callback_cast(keypress_callback), ret);
    (*jnienv)->CallVoidMethod(jnienv, ret->NavitGraphics, cid, (jlong)cb);

    if (!find_method(ret->NavitGraphicsClass, "draw_polyline", "(Landroid/graphics/Paint;[I)V",
                     &ret->NavitGraphics_draw_polyline))
        return 0;
    if (!find_method(ret->NavitGraphicsClass, "draw_polygon_with_holes", "(Landroid/graphics/Paint;IIIII[I[I[I)V",
                     &ret->NavitGraphics_draw_polygon_with_holes))
        return 0;
    if (!find_method(ret->NavitGraphicsClass, "draw_polygon", "(Landroid/graphics/Paint;[I)V",
                     &ret->NavitGraphics_draw_polygon))
        return 0;
    if (!find_method(ret->NavitGraphicsClass, "draw_rectangle", "(Landroid/graphics/Paint;IIII)V",
                     &ret->NavitGraphics_draw_rectangle))
        return 0;
    if (!find_method(ret->NavitGraphicsClass, "draw_circle", "(Landroid/graphics/Paint;III)V",
                     &ret->NavitGraphics_draw_circle))
        return 0;
    if (!find_method(ret->NavitGraphicsClass, "draw_text", "(Landroid/graphics/Paint;IILjava/lang/String;IIII)V",
                     &ret->NavitGraphics_draw_text))
        return 0;
    if (!find_method(ret->NavitGraphicsClass, "draw_image", "(Landroid/graphics/Paint;IILandroid/graphics/Bitmap;)V",
                     &ret->NavitGraphics_draw_image))
        return 0;
    if (!find_method(ret->NavitGraphicsClass, "draw_image_warp",
                     "(Landroid/graphics/Paint;IIIIIIILandroid/graphics/Bitmap;)V", &ret->NavitGraphics_draw_image_warp))
        return 0;
    if (!find_method(ret->NavitGraphicsClass, "draw_mode", "(I)V", &ret->NavitGraphics_draw_mode))
        return 0;
    if (!find_method(ret->NavitGraphicsClass, "draw_drag", "(II)V", &ret->NavitGraphics_draw_drag))
        return 0;
    if (!find_method(ret->NavitGraphicsClass, "overlay_disable", "(I)V", &ret->NavitGraphics_overlay_disable))
        return 0;
    if (!find_method(ret->NavitGraphicsClass, "overlay_resize", "(IIIII)V", &ret->NavitGraphics_overlay_resize))
        return 0;
    if (!find_method(ret->NavitGraphicsClass, "setCamera", "(I)V", &ret->NavitGraphics_SetCamera))
        return 0;
#if 0
    set_activity(ret->NavitGraphics);
#endif
    return 1;
}

static jclass NavitClass;
static jmethodID Navit_disableSuspend, Navit_exit, Navit_fullscreen, Navit_runOptionsItem, Navit_showMenu,
       Navit_showNativeKeyboard, Navit_hideNativeKeyboard;

static int graphics_android_fullscreen(struct window *win, int on) {
    (*jnienv)->CallVoidMethod(jnienv, android_activity, Navit_fullscreen, on);
    return 1;
}

static void graphics_android_disable_suspend(struct window *win) {
    dbg(lvl_debug,"enter");
    (*jnienv)->CallVoidMethod(jnienv, android_activity, Navit_disableSuspend);
}

/**
 * @brief Runs an item from the Android menu.
 *
 * This is a callback function which implements multiple API functions.
 *
 * @param this The {@code graohics_prov} structure
 * @param function The API function which was called
 * @param in Parameters to pass to the API function
 * @param out Points to a buffer which will receive a pointer to the output of the command
 * @param valid
 */
static void graphics_android_cmd_runMenuItem(struct graphics_priv *this, char *function, struct attr **in,
        struct attr ***out, int *valid) {
    int ncmd=0;
    dbg(lvl_debug,"Running %s",function);
    if(!strcmp(function,"map_download_dialog")) {
        ncmd=3;
    } else if(!strcmp(function,"backup_restore_dialog")) {
        ncmd=7;
    } else if(!strcmp(function,"set_map_location")) {
        ncmd=10;
    }
    (*jnienv)->CallVoidMethod(jnienv, android_activity, Navit_runOptionsItem, ncmd);
}

/**
 * @brief Shows the Android menu.
 *
 * This is the callback function associated with the {@code menu()} API function.
 *
 * @param this The {@code graohics_prov} structure
 * @param function The API function which was called
 * @param in Parameters to pass to the API function
 * @param out Points to a buffer which will receive a pointer to the output of the command
 * @param valid
 */
static void graphics_android_cmd_menu(struct graphics_priv *this, char *function, struct attr **in, struct attr ***out,
                                      int *valid) {
    dbg(lvl_debug, "enter");
    (*jnienv)->CallVoidMethod(jnienv, android_activity, Navit_showMenu);
}

/**
 * The command table. Each entry consists of an API function name and the callback function which implements
 * this command.
 */
static struct command_table commands[] = {
    {"map_download_dialog",command_cast(graphics_android_cmd_runMenuItem)},
    {"set_map_location",command_cast(graphics_android_cmd_runMenuItem)},
    {"backup_restore_dialog",command_cast(graphics_android_cmd_runMenuItem)},
    {"menu", command_cast(graphics_android_cmd_menu)},
};

/**
 * @brief Creates a new Android graphics instance.
 *
 * This method is called when the graphics plugin is initialized. It creates the main view, i.e. the map view.
 * Unless overlay mode is enabled, it also holds any OSD items.
 *
 * @param nav The navit instance.
 * @param meth The methods for the new graphics instance
 * @param attrs The attributes for the new graphics instance
 * @param cbl The callback list for the new graphics instance
 *
 * @return The new graphics instance
 */
static struct graphics_priv *graphics_android_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs,
        struct callback_list *cbl) {
    struct graphics_priv *ret;
    struct attr *attr;
    int use_camera=0;
    jmethodID cid;
    jint android_bgcolor;

    dbg(lvl_debug, "enter");
    if (!event_request_system("android","graphics_android"))
        return NULL;
    ret=g_new0(struct graphics_priv, 1);

    ret->cbl=cbl;
    *meth=graphics_methods;
    ret->win.priv=ret;
    ret->win.fullscreen=graphics_android_fullscreen;
    ret->win.disable_suspend=graphics_android_disable_suspend;
    ret->padding = g_new0(struct padding, 1);
    ret->padding->left = 0;
    ret->padding->top = 0;
    ret->padding->right = 0;
    ret->padding->bottom = 0;
    /* attr_background_color is the background color for system bars (API 17+ only) */
    if ((attr=attr_search(attrs, NULL, attr_background_color))) {
        ret->bgcolor = (attr->u.color->a / 0x101) << 24
                       | (attr->u.color->r / 0x101) << 16
                       | (attr->u.color->g / 0x101) << 8
                       | (attr->u.color->b / 0x101);
        dbg(lvl_debug, "attr_background_color %04x %04x %04x %04x (%08x)",
            attr->u.color->r, attr->u.color->g, attr->u.color->b, attr->u.color->a, ret->bgcolor);
    } else {
        /* default is the same as for OSD */
        ret->bgcolor = 0xa0000000;
    }
    if ((attr=attr_search(attrs, NULL, attr_use_camera))) {
        use_camera=attr->u.num;
    }
    if ((attr=attr_search(attrs, NULL, attr_callback_list))) {
        command_add_table(attr->u.callback_list, commands, sizeof(commands)/sizeof(struct command_table), ret);
    }
    image_cache_hash = g_hash_table_new(g_str_hash, g_str_equal);
    if (graphics_android_init(ret, NULL, NULL, 0, 0, 0, use_camera)) {
        cid = (*jnienv)->GetMethodID(jnienv, ret->NavitGraphicsClass, "hasMenuButton", "()Z");
        if (cid != NULL) {
            attr = g_new0(struct attr, 1);
            attr->type = attr_has_menu_button;
            attr->u.num = (*jnienv)->CallBooleanMethod(jnienv, ret->NavitGraphics, cid);

            /*
             * Although the attribute refers to information obtained by the graphics plugin, we are storing it
             * with the navit object: the object is easier to access from anywhere in the program, and ultimately
             * it refers to a configuration value affecting all of Navit, thus users are likely to look for it in
             * the navit object (as the fact that graphics also handles input devices is not immedately obvious).
             */
            navit_object_set_attr((struct navit_object *) nav, attr);
            dbg(lvl_debug, "attr_has_menu_button=%ld", attr->u.num);
            g_free(attr);
        }
        ret->NavitGraphics_setBackgroundColor = (*jnienv)->GetMethodID(jnienv, ret->NavitGraphicsClass, "setBackgroundColor",
                                                "(I)V");
        if (ret->NavitGraphics_setBackgroundColor != NULL) {
            (*jnienv)->CallVoidMethod(jnienv, ret->NavitGraphics, ret->NavitGraphics_setBackgroundColor, ret->bgcolor);
        }
        dbg(lvl_debug,"returning %p",ret);
        return ret;
    } else {
        g_free(ret);
        return NULL;
    }
}

/**
 * @brief Creates a new overlay
 *
 * This method creates a graphics instance for a new overlay. If overlay mode is enabled, a separate overlay is
 * created for each OSD item.
 *
 * @param gr The parent graphics instance, i.e. the one which will contain the overlay.
 * @param meth The methods for the new graphics instance
 * @param p The position of the overlay in its parent
 * @param w The width of the overlay
 * @param h The height of the overlay
 * @param wraparound
 *
 * @return The graphics instance for the new overlay
 */
static struct graphics_priv *overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p,
        int w, int h, int wraparound) {
    struct graphics_priv *ret=g_new0(struct graphics_priv, 1);
    *meth=graphics_methods;
    if (graphics_android_init(ret, gr, p, w, h, wraparound, 0)) {
        dbg(lvl_debug,"returning %p",ret);
        return ret;
    } else {
        g_free(ret);
        return NULL;
    }
}


static void event_android_main_loop_run(void) {
    dbg(lvl_debug,"enter");
}

static void event_android_main_loop_quit(void) {
    dbg(lvl_debug,"enter");
    (*jnienv)->CallVoidMethod(jnienv, android_activity, Navit_exit);
}


static jclass NavitTimeoutClass;
static jmethodID NavitTimeout_init;
static jmethodID NavitTimeout_remove;

static jclass NavitIdleClass;
static jmethodID NavitIdle_init;
static jmethodID NavitIdle_remove;

static jclass NavitWatchClass;
static jmethodID NavitWatch_init;
static jmethodID NavitWatch_remove;


static void do_poll(JNIEnv *env, int fd, int cond) {
    struct pollfd pfd;
    pfd.fd=fd;
    dbg(lvl_debug,"poll called for %d %d", fd, cond);
    switch ((enum event_watch_cond)cond) {
    case event_watch_cond_read:
        pfd.events=POLLIN;
        break;
    case event_watch_cond_write:
        pfd.events=POLLOUT;
        break;
    case event_watch_cond_except:
        pfd.events=POLLERR;
        break;
    default:
        pfd.events=0;
    }
    pfd.revents=0;
    poll(&pfd, 1, -1);
}

static struct event_watch *event_android_add_watch(int h, enum event_watch_cond cond, struct callback *cb) {
    jobject ret;
    ret=(*jnienv)->NewObject(jnienv, NavitWatchClass, NavitWatch_init, (jlong)do_poll, h, (jint) cond, (jlong)cb);
    dbg(lvl_debug,"result for %d,%d,%p = %p",h,cond,cb,ret);
    if (ret)
        ret = (*jnienv)->NewGlobalRef(jnienv, ret);
    return (struct event_watch *)ret;
}

static void event_android_remove_watch(struct event_watch *ev) {
    dbg(lvl_debug,"enter %p",ev);
    if (ev) {
        jobject obj=(jobject )ev;
        (*jnienv)->CallVoidMethod(jnienv, obj, NavitWatch_remove);
        (*jnienv)->DeleteGlobalRef(jnienv, obj);
    }
}

struct event_timeout {
    void (*handle_timeout)(struct event_timeout *priv);
    jobject jni_timeout;
    int multi;
    struct callback *cb;
};

static void event_android_remove_timeout(struct event_timeout *priv) {
    if (priv && priv->jni_timeout) {
        (*jnienv)->CallVoidMethod(jnienv, priv->jni_timeout, NavitTimeout_remove);
        (*jnienv)->DeleteGlobalRef(jnienv, priv->jni_timeout);
        priv->jni_timeout = NULL;
    }
}

static void event_android_handle_timeout(struct event_timeout *priv) {
    callback_call_0(priv->cb);
    if (!priv->multi)
        event_android_remove_timeout(priv);
}

static struct event_timeout *event_android_add_timeout(int timeout, int multi, struct callback *cb) {
    struct event_timeout *ret = g_new0(struct event_timeout, 1);
    ret->cb = cb;
    ret->multi = multi;
    ret->handle_timeout = event_android_handle_timeout;
    ret->jni_timeout = (*jnienv)->NewObject(jnienv, NavitTimeoutClass, NavitTimeout_init, timeout, multi, (jlong)(ret));
    dbg(lvl_debug,"result for %d,%d,%p = %p",timeout,multi,cb,ret);
    if (ret->jni_timeout)
        ret->jni_timeout = (*jnienv)->NewGlobalRef(jnienv, ret->jni_timeout);
    return ret;
}


static struct event_idle *event_android_add_idle(int priority, struct callback *cb) {
#if 0
    jobject ret;
    dbg(lvl_debug,"enter");
    ret=(*jnienv)->NewObject(jnienv, NavitIdleClass, NavitIdle_init, (int)cb);
    dbg(lvl_debug,"result for %p=%p",cb,ret);
    if (ret)
        ret = (*jnienv)->NewGlobalRef(jnienv, ret);
    return (struct event_idle *)ret;
#endif
    return (struct event_idle *)event_android_add_timeout(1, 1, cb);
}

static void event_android_remove_idle(struct event_idle *ev) {
#if 0
    dbg(lvl_debug,"enter %p",ev);
    if (ev) {
        jobject obj=(jobject )ev;
        (*jnienv)->CallVoidMethod(jnienv, obj, NavitIdle_remove);
        (*jnienv)->DeleteGlobalRef(jnienv, obj);
    }
#endif
    event_android_remove_timeout((struct event_timeout *)ev);
}

static void event_android_call_callback(struct callback_list *cb) {
    dbg(lvl_debug,"enter");
}

static struct event_methods event_android_methods = {
    event_android_main_loop_run,
    event_android_main_loop_quit,
    event_android_add_watch,
    event_android_remove_watch,
    event_android_add_timeout,
    event_android_remove_timeout,
    event_android_add_idle,
    event_android_remove_idle,
    event_android_call_callback,
};

static struct event_priv *event_android_new(struct event_methods *meth) {
    dbg(lvl_debug,"enter");
    if (!find_class_global("org/navitproject/navit/NavitTimeout", &NavitTimeoutClass))
        return NULL;
    NavitTimeout_init = (*jnienv)->GetMethodID(jnienv, NavitTimeoutClass, "<init>", "(IZJ)V");
    if (NavitTimeout_init == NULL)
        return NULL;
    NavitTimeout_remove = (*jnienv)->GetMethodID(jnienv, NavitTimeoutClass, "remove", "()V");
    if (NavitTimeout_remove == NULL)
        return NULL;
#if 0
    if (!find_class_global("org/navitproject/navit/NavitIdle", &NavitIdleClass))
        return NULL;
    NavitIdle_init = (*jnienv)->GetMethodID(jnienv, NavitIdleClass, "<init>", "(I)V");
    if (NavitIdle_init == NULL)
        return NULL;
    NavitIdle_remove = (*jnienv)->GetMethodID(jnienv, NavitIdleClass, "remove", "()V");
    if (NavitIdle_remove == NULL)
        return NULL;
#endif

    if (!find_class_global("org/navitproject/navit/NavitWatch", &NavitWatchClass))
        return NULL;
    NavitWatch_init = (*jnienv)->GetMethodID(jnienv, NavitWatchClass, "<init>", "(JIIJ)V");
    if (NavitWatch_init == NULL)
        return NULL;
    NavitWatch_remove = (*jnienv)->GetMethodID(jnienv, NavitWatchClass, "remove", "()V");
    if (NavitWatch_remove == NULL)
        return NULL;

    if (!find_class_global("org/navitproject/navit/Navit", &NavitClass))
        return NULL;
    Navit_disableSuspend = (*jnienv)->GetMethodID(jnienv, NavitClass, "disableSuspend", "()V");
    if (Navit_disableSuspend == NULL)
        return NULL;
    Navit_exit = (*jnienv)->GetMethodID(jnienv, NavitClass, "onDestroy", "()V");
    if (Navit_exit == NULL)
        return NULL;
    Navit_fullscreen = (*jnienv)->GetMethodID(jnienv, NavitClass, "fullscreen", "(I)V");
    if (Navit_fullscreen == NULL)
        return NULL;
    Navit_runOptionsItem = (*jnienv)->GetMethodID(jnienv, NavitClass, "runOptionsItem", "(I)V");
    if (Navit_runOptionsItem == NULL)
        return NULL;
    Navit_showMenu = (*jnienv)->GetMethodID(jnienv, NavitClass, "showMenu", "()V");
    if (Navit_showMenu == NULL)
        return NULL;
    Navit_showNativeKeyboard = (*jnienv)->GetMethodID(jnienv, NavitClass, "showNativeKeyboard", "()I");
    Navit_hideNativeKeyboard = (*jnienv)->GetMethodID(jnienv, NavitClass, "hideNativeKeyboard", "()V");

    dbg(lvl_debug,"ok");
    *meth=event_android_methods;
    return NULL;
}

/* below needs review, android resizes the view  and the actual height of the keyboard is not
 * passed down here and is irrelevant, only wether it is onscreen matters, so
 * android returns a height of 1 px in the case of an onscreen keyboard just
 * to keep the logic to remove the keyboard from the screen afterwards working untill
 * the logic in native code is reviewed.
 */
/**
 * @brief Displays the native input method.
 *
 * This method decides whether a native on-screen input method, such as a virtual keyboard, needs to be
 * displayed. A typical case in which there is no need for an on-screen input method is if a hardware
 * keyboard is present.
 *
 * Note that the Android platform lacks reliable means of determining whether an on-screen input method
 * is going to be displayed or how much screen space it is going to occupy. Therefore this method tries
 * to guess these values. They have been tested and found to work correctly with the AOSP keyboard, and
 * are thus expected to be compatible with most on-screen keyboards found on the market, but results may
 * be unexpected with other input methods.
 *
 * @param kbd A {@code struct graphics_keyboard} which describes the requirements for the input method
 * and will be populated with the data of the input method before the function returns.
 *
 * @return True if the input method is going to be displayed, false if not.
 */
int show_native_keyboard (struct graphics_keyboard *kbd) {
    if (Navit_showNativeKeyboard == NULL) {
        dbg(lvl_error, "method Navit.showNativeKeyboard() not found, cannot display keyboard");
        return 0;
    }
    kbd->h = (*jnienv)->CallIntMethod(jnienv, android_activity, Navit_showNativeKeyboard);
    dbg(lvl_error, "keyboard height is %d px is a lie", kbd->h);
    /* zero height means we're not showing a keyboard, therefore normalize height to boolean */
    return !!(kbd->h);
}


/**
 * @brief Hides the native input method and frees associated private data.
 *
 * @param kbd The {@code struct graphics_keyboard} which was passed to the earlier call to
 * {@link show_native_keyboard(struct graphics_keyboard *)}. The {@code gra_priv} member of the struct
 * will be freed by this function.
 */
void hide_native_keyboard (struct graphics_keyboard *kbd) {
    if (Navit_hideNativeKeyboard == NULL) {
        dbg(lvl_error, "method Navit.hideNativeKeyboard() not found, cannot dismiss keyboard");
        return;
    }
    (*jnienv)->CallVoidMethod(jnienv, android_activity, Navit_hideNativeKeyboard);
    g_free(kbd->gra_priv);
}


void plugin_init(void) {
    dbg(lvl_debug,"enter");
    plugin_register_category_graphics("android", graphics_android_new);
    plugin_register_category_event("android", event_android_new);
}

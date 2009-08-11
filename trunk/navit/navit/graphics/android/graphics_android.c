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
#include <unistd.h>
#include "config.h"
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "plugin.h"
#include "event.h"
#include "debug.h"
#include "callback.h"
#include "android.h"

int dummy;

struct graphics_priv {
	jclass NavitGraphicsClass;
	jmethodID NavitGraphics_draw_polyline, NavitGraphics_draw_polygon, NavitGraphics_draw_rectangle, NavitGraphics_draw_circle, NavitGraphics_draw_text, NavitGraphics_draw_image, NavitGraphics_draw_mode;

	jclass PaintClass;
	jmethodID Paint_init,Paint_setStrokeWidth,Paint_setARGB;

	jobject NavitGraphics;
	
	jclass BitmapFactoryClass;	
	jmethodID BitmapFactory_decodeFile;

	jclass BitmapClass;
	jmethodID Bitmap_getHeight, Bitmap_getWidth;

	struct callback_list *cbl;
};

struct graphics_font_priv {
	int size;
};

struct graphics_gc_priv {
	struct graphics_priv *gra;
	jobject Paint;
};

struct graphics_image_priv {
	jobject Bitmap;
};

static int
find_class_global(char *name, jclass *ret)
{
	*ret=(*jnienv)->FindClass(jnienv, name);
	if (! *ret) {
		dbg(0,"Failed to get Class %s\n",name);
		return 0;
	}
	(*jnienv)->NewGlobalRef(jnienv, *ret);
	return 1;
}

static int
find_method(jclass class, char *name, char *args, jmethodID *ret)
{
	*ret = (*jnienv)->GetMethodID(jnienv, class, name, args);
	if (*ret == NULL) {
		dbg(0,"Failed to get Method %s with signature %s\n",name,args);
		return 0;
	}
	return 1;
}

static int
find_static_method(jclass class, char *name, char *args, jmethodID *ret)
{
	*ret = (*jnienv)->GetStaticMethodID(jnienv, class, name, args);
	if (*ret == NULL) {
		dbg(0,"Failed to get static Method %s with signature %s\n",name,args);
		return 0;
	}
	return 1;
}

static void
graphics_destroy(struct graphics_priv *gr)
{
}

static void font_destroy(struct graphics_font_priv *font)
{
	g_free(font);
}

static struct graphics_font_methods font_methods = {
	font_destroy
};

static struct graphics_font_priv *font_new(struct graphics_priv *gr, struct graphics_font_methods *meth, char *font,  int size, int flags)
{
	struct graphics_font_priv *ret=g_new0(struct graphics_font_priv, 1);
	*meth=font_methods;

	ret->size=size;
	return ret;
}

static void
gc_destroy(struct graphics_gc_priv *gc)
{
	(*jnienv)->DeleteGlobalRef(jnienv, gc->Paint);
	g_free(gc);
}

static void
gc_set_linewidth(struct graphics_gc_priv *gc, int w)
{
	float wf=w;
	(*jnienv)->CallVoidMethod(jnienv, gc->Paint, gc->gra->Paint_setStrokeWidth, wf);
}

static void
gc_set_dashes(struct graphics_gc_priv *gc, int w, int offset, unsigned char *dash_list, int n)
{
}

static void
gc_set_foreground(struct graphics_gc_priv *gc, struct color *c)
{
	(*jnienv)->CallVoidMethod(jnienv, gc->Paint, gc->gra->Paint_setARGB, c->a >> 8, c->r >> 8, c->g >> 8, c->b >> 8);
}

static void
gc_set_background(struct graphics_gc_priv *gc, struct color *c)
{
}

static struct graphics_gc_methods gc_methods = {
	gc_destroy,
	gc_set_linewidth,
	gc_set_dashes,	
	gc_set_foreground,	
	gc_set_background	
};

static struct graphics_gc_priv *gc_new(struct graphics_priv *gr, struct graphics_gc_methods *meth)
{
	struct graphics_gc_priv *ret=g_new0(struct graphics_gc_priv, 1);
	*meth=gc_methods;

	ret->gra=gr;	
	ret->Paint=(*jnienv)->NewObject(jnienv, gr->PaintClass, gr->Paint_init);
	dbg(1,"result=%p\n",ret->Paint);
	if (ret->Paint)
		(*jnienv)->NewGlobalRef(jnienv, ret->Paint);
	return ret;
}

static void image_destroy(struct graphics_image_priv *img)
{
	dbg(0,"enter\n");
}

static struct graphics_image_methods image_methods = {
        image_destroy
};


static struct graphics_image_priv *
image_new(struct graphics_priv *gra, struct graphics_image_methods *meth, char *path, int *w, int *h, struct point *hot, int rotation)
{
	struct graphics_image_priv *ret=g_new0(struct graphics_image_priv, 1);
	dbg(0,"enter\n");
	*meth=image_methods;
	jstring string = (*jnienv)->NewStringUTF(jnienv, path);
	dbg(0,"%p %p %p\n",gra->BitmapFactoryClass, gra->BitmapFactory_decodeFile, string);
	ret->Bitmap=(*jnienv)->CallStaticObjectMethod(jnienv, gra->BitmapFactoryClass, gra->BitmapFactory_decodeFile, string);
	dbg(0,"result=%p\n",ret->Bitmap);
	if (ret->Bitmap) {
		(*jnienv)->NewGlobalRef(jnienv, ret->Bitmap);
		*w=(*jnienv)->CallIntMethod(jnienv, ret->Bitmap, gra->Bitmap_getWidth);
		*h=(*jnienv)->CallIntMethod(jnienv, ret->Bitmap, gra->Bitmap_getHeight);
		dbg(0,"w=%d h=%d for %s\n",*w,*h,path);
		*w=64;
		*h=64;
		hot->x=*w/2;
		hot->y=*h/2;
	} else {
		g_free(ret);
		ret=NULL;
		dbg(0,"Failed to open %s\n",path);
	}
	(*jnienv)->DeleteLocalRef(jnienv, string);
	
	return ret;
}

static void
draw_lines(struct graphics_priv *gra, struct graphics_gc_priv *gc, struct point *p, int count)
{
	jintArray points = (*jnienv)->NewIntArray(jnienv,count*2);
	jint pc[count*2];
	int i;
	for (i = 0 ; i < count ; i++) {
		pc[i*2]=p[i].x;
		pc[i*2+1]=p[i].y;
	}
	(*jnienv)->SetIntArrayRegion(jnienv, points, 0, count*2, pc);
	(*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_polyline, gc->Paint, points);
	(*jnienv)->DeleteLocalRef(jnienv, points);
}

static void
draw_polygon(struct graphics_priv *gra, struct graphics_gc_priv *gc, struct point *p, int count)
{
	jintArray points = (*jnienv)->NewIntArray(jnienv,count*2);
	jint pc[count*2];
	int i;
	for (i = 0 ; i < count ; i++) {
		pc[i*2]=p[i].x;
		pc[i*2+1]=p[i].y;
	}
	(*jnienv)->SetIntArrayRegion(jnienv, points, 0, count*2, pc);
	(*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_polygon, gc->Paint, points);
	(*jnienv)->DeleteLocalRef(jnienv, points);
}

static void
draw_rectangle(struct graphics_priv *gra, struct graphics_gc_priv *gc, struct point *p, int w, int h)
{
	(*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_rectangle, gc->Paint, p->x, p->y, w, h);
}

static void
draw_circle(struct graphics_priv *gra, struct graphics_gc_priv *gc, struct point *p, int r)
{
	(*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_circle, gc->Paint, p->x, p->y, r);
}


static void
draw_text(struct graphics_priv *gra, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy)
{
	dbg(1,"enter %s\n", text);
	jstring string = (*jnienv)->NewStringUTF(jnienv, text);
	(*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_text, fg->Paint, p->x, p->y, string);
	(*jnienv)->DeleteLocalRef(jnienv, string);
}

static void
draw_image(struct graphics_priv *gra, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img)
{
	dbg(1,"enter %p\n",img);
	(*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_image, fg->Paint, p->x, p->y, img->Bitmap);
	
}

static void
draw_image_warp(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, int count, char *data)
{
}

static void
draw_restore(struct graphics_priv *gr, struct point *p, int w, int h)
{
}

static void draw_drag(struct graphics_priv *gr, struct point *p)
{
}

static void
background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc)
{
}

static void
draw_mode(struct graphics_priv *gra, enum draw_mode_num mode)
{
	dbg(1,"enter %d\n",mode);
	(*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_mode, (int)mode);
}

static struct graphics_priv * overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h, int alpha, int wraparound);

static void *
get_data(struct graphics_priv *this, char *type)
{
	return &dummy;
}

static void image_free(struct graphics_priv *gr, struct graphics_image_priv *priv)
{
}

static void get_text_bbox(struct graphics_priv *gr, struct graphics_font_priv *font, char *text, int dx, int dy, struct point *ret, int estimate)
{
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

static void overlay_disable(struct graphics_priv *gr, int disable)
{
}

static void overlay_resize(struct graphics_priv *gr, struct point *p, int w, int h, int alpha, int wraparound)
{
}

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
	draw_restore,
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
};

static void
resize_callback(struct graphics_priv *gra, int w, int h)
{
	dbg(0,"w=%d h=%d ok\n",w,h);
	 callback_list_call_attr_2(gra->cbl, attr_resize, (void *)w, (void *)h);
}

static void
motion_callback(struct graphics_priv *gra, int x, int y)
{
	struct point p;
	p.x=x;
	p.y=y;
	callback_list_call_attr_1(gra->cbl, attr_motion, (void *)&p);
}

static void
button_callback(struct graphics_priv *gra, int pressed, int button, int x, int y)
{
	struct point p;
	p.x=x;
	p.y=y;
	callback_list_call_attr_3(gra->cbl, attr_button, (void *)pressed, (void *)button, (void *)&p);
}


static struct graphics_priv *
overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h, int alpha, int wraparound)
{
	struct graphics_priv *ret=g_new0(struct graphics_priv, 1);
	*meth=graphics_methods;
	return ret;
}

static int
set_activity(jobject graphics)
{
	jclass ActivityClass;
	jmethodID cid;

	ActivityClass = (*jnienv)->GetObjectClass(jnienv, android_activity);
	dbg(0,"at 5\n");
	if (ActivityClass == NULL) {
		dbg(0,"no activity class found\n");
		return 0;
	}
	dbg(0,"at 6\n");
	cid = (*jnienv)->GetMethodID(jnienv, ActivityClass, "setContentView", "(Landroid/view/View;)V");
	if (cid == NULL) {
		dbg(0,"no setContentView method found\n");
		return 0;
	}
	dbg(0,"at 7\n");
	(*jnienv)->CallVoidMethod(jnienv, android_activity, cid, graphics);
	dbg(0,"at 8\n");
	return 1;
}

static int
graphics_android_init(struct graphics_priv *ret)
{
	struct callback *cb;
	jmethodID cid;

	dbg(0,"at 1\n");
	if (!event_request_system("android","graphics_android"))
		return 0;

	dbg(0,"at 2 jnienv=%p\n",jnienv);
	if (!find_class_global("android/graphics/Paint", &ret->PaintClass))
		return 0;
	if (!find_method(ret->PaintClass, "<init>", "()V", &ret->Paint_init))
		return 0;
	if (!find_method(ret->PaintClass, "setARGB", "(IIII)V", &ret->Paint_setARGB))
		return 0;
	if (!find_method(ret->PaintClass, "setStrokeWidth", "(F)V", &ret->Paint_setStrokeWidth))
		return 0;

	if (!find_class_global("android/graphics/BitmapFactory", &ret->BitmapFactoryClass))
		return 0;
	if (!find_static_method(ret->BitmapFactoryClass, "decodeFile", "(Ljava/lang/String;)Landroid/graphics/Bitmap;", &ret->BitmapFactory_decodeFile))
		return 0;

	if (!find_class_global("android/graphics/Bitmap", &ret->BitmapClass))
		return 0;
	if (!find_method(ret->BitmapClass, "getHeight", "()I", &ret->Bitmap_getHeight))
		return 0;
	if (!find_method(ret->BitmapClass, "getWidth", "()I", &ret->Bitmap_getWidth))
		return 0;

	if (!find_class_global("org/navitproject/navitgraphics/NavitGraphics", &ret->NavitGraphicsClass))
		return 0;
	dbg(0,"at 3\n");
	cid = (*jnienv)->GetMethodID(jnienv, ret->NavitGraphicsClass, "<init>", "(Landroid/app/Activity;)V");
	if (cid == NULL) {
		dbg(0,"no method found\n");
		return 0; /* exception thrown */
	}
	dbg(0,"at 4 android_activity=%p\n",android_activity);
	ret->NavitGraphics=(*jnienv)->NewObject(jnienv, ret->NavitGraphicsClass, cid, android_activity);
	dbg(0,"result=%p\n",ret->NavitGraphics);
	if (ret->NavitGraphics)
		(*jnienv)->NewGlobalRef(jnienv, ret->NavitGraphics);

	cid = (*jnienv)->GetMethodID(jnienv, ret->NavitGraphicsClass, "setSizeChangedCallback", "(I)V");
	if (cid == NULL) {
		dbg(0,"no SetResizeCallback method found\n");
		return 0; /* exception thrown */
	}
	cb=callback_new_1(callback_cast(resize_callback), ret);
	(*jnienv)->CallVoidMethod(jnienv, ret->NavitGraphics, cid, (int)cb);

	cid = (*jnienv)->GetMethodID(jnienv, ret->NavitGraphicsClass, "setButtonCallback", "(I)V");
	if (cid == NULL) {
		dbg(0,"no SetButtonCallback method found\n");
		return 0; /* exception thrown */
	}
	cb=callback_new_1(callback_cast(button_callback), ret);
	(*jnienv)->CallVoidMethod(jnienv, ret->NavitGraphics, cid, (int)cb);

	cid = (*jnienv)->GetMethodID(jnienv, ret->NavitGraphicsClass, "setMotionCallback", "(I)V");
	if (cid == NULL) {
		dbg(0,"no SetMotionCallback method found\n");
		return 0; /* exception thrown */
	}
	cb=callback_new_1(callback_cast(motion_callback), ret);
	(*jnienv)->CallVoidMethod(jnienv, ret->NavitGraphics, cid, (int)cb);

	if (!find_method(ret->NavitGraphicsClass, "draw_polyline", "(Landroid/graphics/Paint;[I)V", &ret->NavitGraphics_draw_polyline))
		return 0;
	if (!find_method(ret->NavitGraphicsClass, "draw_polygon", "(Landroid/graphics/Paint;[I)V", &ret->NavitGraphics_draw_polygon))
		return 0;
	if (!find_method(ret->NavitGraphicsClass, "draw_rectangle", "(Landroid/graphics/Paint;IIII)V", &ret->NavitGraphics_draw_rectangle))
		return 0;
	if (!find_method(ret->NavitGraphicsClass, "draw_circle", "(Landroid/graphics/Paint;III)V", &ret->NavitGraphics_draw_circle))
		return 0;
	if (!find_method(ret->NavitGraphicsClass, "draw_text", "(Landroid/graphics/Paint;IILjava/lang/String;)V", &ret->NavitGraphics_draw_text))
		return 0;
	if (!find_method(ret->NavitGraphicsClass, "draw_image", "(Landroid/graphics/Paint;IILandroid/graphics/Bitmap;)V", &ret->NavitGraphics_draw_image))
		return 0;
	if (!find_method(ret->NavitGraphicsClass, "draw_mode", "(I)V", &ret->NavitGraphics_draw_mode))
		return 0;
	set_activity(ret->NavitGraphics);
	return 1;
}


static struct graphics_priv *
graphics_android_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs, struct callback_list *cbl)
{
	struct graphics_priv *ret=g_new0(struct graphics_priv, 1);

	ret->cbl=cbl;
	*meth=graphics_methods;
	if (graphics_android_init(ret)) {
		return ret;
	} else {
		g_free(ret);
		return NULL;
	}
}

static void
event_android_main_loop_run(void)
{
        dbg(0,"enter\n");
}

static void event_android_main_loop_quit(void)
{
        dbg(0,"enter\n");
}

static struct event_watch *
event_android_add_watch(void *h, enum event_watch_cond cond, struct callback *cb)
{
        dbg(0,"enter\n");
        return NULL;
}

static void
event_android_remove_watch(struct event_watch *ev)
{
        dbg(0,"enter\n");
}


static struct event_timeout *
event_android_add_timeout(int timeout, int multi, struct callback *cb)
{
        dbg(0,"enter\n");
	return NULL;
}

static void
event_android_remove_timeout(struct event_timeout *to)
{
        dbg(0,"enter\n");
}


static struct event_idle *
event_android_add_idle(int priority, struct callback *cb)
{
        dbg(0,"enter\n");
        return NULL;
}

static void
event_android_remove_idle(struct event_idle *ev)
{
        dbg(0,"enter\n");
}

static void
event_android_call_callback(struct callback_list *cb)
{
        dbg(0,"enter\n");
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

static struct event_priv *
event_android_new(struct event_methods *meth)
{
        *meth=event_android_methods;
        return NULL;
}


void
plugin_init(void)
{
	dbg(0,"enter\n");
        plugin_register_graphics_type("android", graphics_android_new);
	plugin_register_event_type("android", event_android_new);
}

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
#include "plugin.h"
#include "event.h"
#include "debug.h"
#include "callback.h"
#include "android.h"

int dummy;

struct graphics_priv {
	jclass NavitGraphicsClass;
	jmethodID NavitGraphics_draw_polyline, NavitGraphics_draw_polygon, NavitGraphics_draw_rectangle, NavitGraphics_draw_circle, NavitGraphics_draw_text, NavitGraphics_draw_image, NavitGraphics_draw_mode, NavitGraphics_draw_drag, NavitGraphics_overlay_disable, NavitGraphics_overlay_resize, NavitGraphics_SetCamera;

	jclass PaintClass;
	jmethodID Paint_init,Paint_setStrokeWidth,Paint_setARGB;

	jobject NavitGraphics;
	jobject Paint;
	
	jclass BitmapFactoryClass;	
	jmethodID BitmapFactory_decodeFile, BitmapFactory_decodeResource;

	jclass BitmapClass;
	jmethodID Bitmap_getHeight, Bitmap_getWidth;

	jclass ContextClass;
	jmethodID Context_getResources;

	jclass ResourcesClass;
	jobject Resources;
	jmethodID Resources_getIdentifier;

	jobject packageName;

	struct callback_list *cbl;
	struct window win;
};

struct graphics_font_priv {
	int size;
};

struct graphics_gc_priv {
	struct graphics_priv *gra;
	int linewidth;
	enum draw_mode_num mode;
	int a,r,g,b;
};

struct graphics_image_priv {
	jobject Bitmap;
	int width;
	int height;
	struct point hot;
};

static GHashTable *image_cache_hash = NULL;

static int
find_class_global(char *name, jclass *ret)
{
	*ret=(*jnienv)->FindClass(jnienv, name);
	if (! *ret) {
		dbg(0,"Failed to get Class %s\n",name);
		return 0;
	}
	*ret = (*jnienv)->NewGlobalRef(jnienv, *ret);
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
	g_free(gc);
}

static void
gc_set_linewidth(struct graphics_gc_priv *gc, int w)
{
    gc->linewidth = w;
}

static void
gc_set_dashes(struct graphics_gc_priv *gc, int w, int offset, unsigned char *dash_list, int n)
{
}

static void
gc_set_foreground(struct graphics_gc_priv *gc, struct color *c)
{
    gc->r = c->r >> 8;
    gc->g = c->g >> 8;
    gc->b = c->b >> 8;
    gc->a = c->a >> 8;
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

	ret->gra = gr;
	ret->a = ret->r = ret->g = ret->b = 255;
	ret->linewidth=1;
	return ret;
}

static void image_destroy(struct graphics_image_priv *img)
{
    // unused?
}

static struct graphics_image_methods image_methods = {
        image_destroy
};


static struct graphics_image_priv *
image_new(struct graphics_priv *gra, struct graphics_image_methods *meth, char *path, int *w, int *h, struct point *hot, int rotation)
{
	struct graphics_image_priv* ret = NULL;

	if ( !g_hash_table_lookup_extended( image_cache_hash, path, NULL, (gpointer)&ret) )
	{
		ret=g_new0(struct graphics_image_priv, 1);
		jstring string;
		jclass localBitmap = NULL;
		int id;

		dbg(1,"enter %s\n",path);
		if (!strncmp(path,"res/drawable/",13)) {
			jstring a=(*jnienv)->NewStringUTF(jnienv, "drawable");
			char *path_noext=g_strdup(path+13);
			char *pos=strrchr(path_noext, '.');
			if (pos)
				*pos='\0';
			dbg(1,"path_noext=%s\n",path_noext);
			string = (*jnienv)->NewStringUTF(jnienv, path_noext);
			g_free(path_noext);
			id=(*jnienv)->CallIntMethod(jnienv, gra->Resources, gra->Resources_getIdentifier, string, a, gra->packageName);
			dbg(1,"id=%d\n",id);
			if (id)
				localBitmap=(*jnienv)->CallStaticObjectMethod(jnienv, gra->BitmapFactoryClass, gra->BitmapFactory_decodeResource, gra->Resources, id);
			(*jnienv)->DeleteLocalRef(jnienv, a);
		} else {
			string = (*jnienv)->NewStringUTF(jnienv, path);
			localBitmap=(*jnienv)->CallStaticObjectMethod(jnienv, gra->BitmapFactoryClass, gra->BitmapFactory_decodeFile, string);
		}
		dbg(1,"result=%p\n",localBitmap);
		if (localBitmap) {
			ret->Bitmap = (*jnienv)->NewGlobalRef(jnienv, localBitmap);
			(*jnienv)->DeleteLocalRef(jnienv, localBitmap);
			ret->width=(*jnienv)->CallIntMethod(jnienv, ret->Bitmap, gra->Bitmap_getWidth);
			ret->height=(*jnienv)->CallIntMethod(jnienv, ret->Bitmap, gra->Bitmap_getHeight);
			dbg(1,"w=%d h=%d for %s\n",ret->width,ret->height,path);
			ret->hot.x=ret->width/2;
			ret->hot.y=ret->height/2;
		} else {
			g_free(ret);
			ret=NULL;
			dbg(1,"Failed to open %s\n",path);
		}
		(*jnienv)->DeleteLocalRef(jnienv, string);
		g_hash_table_insert(image_cache_hash, g_strdup( path ),  (gpointer)ret );
	}
	if (ret) {
		*w=ret->width;
		*h=ret->height;
		if (hot)
			*hot=ret->hot;
	}

	return ret;
}

static void initPaint(struct graphics_priv *gra, struct graphics_gc_priv *gc)
{
    float wf = gc->linewidth;
    (*jnienv)->CallVoidMethod(jnienv, gc->gra->Paint, gra->Paint_setStrokeWidth, wf);
    (*jnienv)->CallVoidMethod(jnienv, gc->gra->Paint, gra->Paint_setARGB, gc->a, gc->r, gc->g, gc->b);
}

static void
draw_lines(struct graphics_priv *gra, struct graphics_gc_priv *gc, struct point *p, int count)
{
	jint pc[count*2];
	int i;
	jintArray points;
	if (count <= 0)
		return;
	points = (*jnienv)->NewIntArray(jnienv,count*2);
	for (i = 0 ; i < count ; i++) {
		pc[i*2]=p[i].x;
		pc[i*2+1]=p[i].y;
	}
	initPaint(gra, gc);
	(*jnienv)->SetIntArrayRegion(jnienv, points, 0, count*2, pc);
	(*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_polyline, gc->gra->Paint, points);
	(*jnienv)->DeleteLocalRef(jnienv, points);
}

static void
draw_polygon(struct graphics_priv *gra, struct graphics_gc_priv *gc, struct point *p, int count)
{
	jint pc[count*2];
	int i;
	jintArray points;
	if (count <= 0)
		return;
	points = (*jnienv)->NewIntArray(jnienv,count*2);
	for (i = 0 ; i < count ; i++) {
		pc[i*2]=p[i].x;
		pc[i*2+1]=p[i].y;
	}
	initPaint(gra, gc);
	(*jnienv)->SetIntArrayRegion(jnienv, points, 0, count*2, pc);
	(*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_polygon, gc->gra->Paint, points);
	(*jnienv)->DeleteLocalRef(jnienv, points);
}

static void
draw_rectangle(struct graphics_priv *gra, struct graphics_gc_priv *gc, struct point *p, int w, int h)
{
        initPaint(gra, gc);
	(*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_rectangle, gc->gra->Paint, p->x, p->y, w, h);
}

static void
draw_circle(struct graphics_priv *gra, struct graphics_gc_priv *gc, struct point *p, int r)
{
        initPaint(gra, gc);
	(*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_circle, gc->gra->Paint, p->x, p->y, r);
}


static void
draw_text(struct graphics_priv *gra, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy)
{
	dbg(1,"enter %s\n", text);
	initPaint(gra, fg);
	jstring string = (*jnienv)->NewStringUTF(jnienv, text);
	(*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_text, fg->gra->Paint, p->x, p->y, string, font->size, dx, dy);
	(*jnienv)->DeleteLocalRef(jnienv, string);
}

static void
draw_image(struct graphics_priv *gra, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img)
{
	dbg(1,"enter %p\n",img);
	initPaint(gra, fg);
	(*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_image, fg->gra->Paint, p->x, p->y, img->Bitmap);
	
}

static void
draw_restore(struct graphics_priv *gr, struct point *p, int w, int h)
{
}

static void draw_drag(struct graphics_priv *gra, struct point *p)
{
	(*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_drag, p ? p->x : 0, p ? p->y : 0);
}

static void
background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc)
{
}

static void
draw_mode(struct graphics_priv *gra, enum draw_mode_num mode)
{
	(*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_draw_mode, (int)mode);
}

static struct graphics_priv * overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h, int alpha, int wraparound);

static void *
get_data(struct graphics_priv *this, const char *type)
{
	if (strcmp(type,"window"))
		return NULL;
	return &this->win;
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

static void overlay_disable(struct graphics_priv *gra, int disable)
{
	(*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_overlay_disable, disable);
}

static void overlay_resize(struct graphics_priv *gra, struct point *pnt, int w, int h, int alpha, int wraparound)
{
	(*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_overlay_resize, pnt ? pnt->x:0 , pnt ? pnt->y:0, w, h, alpha, wraparound);
}

static int
set_attr(struct graphics_priv *gra, struct attr *attr)
{
	switch (attr->type) {
	case attr_use_camera:
		(*jnienv)->CallVoidMethod(jnienv, gra->NavitGraphics, gra->NavitGraphics_SetCamera, attr->u.num);
		return 1;
	default:
		return 0;
	}
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
	NULL,
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
	set_attr,
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
keypress_callback(struct graphics_priv *gra, char *s)
{
	dbg(0,"enter %s\n",s);
	callback_list_call_attr_1(gra->cbl, attr_keypress, s);
}

static void
button_callback(struct graphics_priv *gra, int pressed, int button, int x, int y)
{
	struct point p;
	p.x=x;
	p.y=y;
	callback_list_call_attr_3(gra->cbl, attr_button, (void *)pressed, (void *)button, (void *)&p);
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
graphics_android_init(struct graphics_priv *ret, struct graphics_priv *parent, struct point *pnt, int w, int h, int alpha, int wraparound, int use_camera)
{
	struct callback *cb;
	jmethodID cid, Context_getPackageName;

	dbg(0,"at 2 jnienv=%p\n",jnienv);
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
	if (!find_static_method(ret->BitmapFactoryClass, "decodeFile", "(Ljava/lang/String;)Landroid/graphics/Bitmap;", &ret->BitmapFactory_decodeFile))
		return 0;
	if (!find_static_method(ret->BitmapFactoryClass, "decodeResource", "(Landroid/content/res/Resources;I)Landroid/graphics/Bitmap;", &ret->BitmapFactory_decodeResource))
		return 0;

	if (!find_class_global("android/graphics/Bitmap", &ret->BitmapClass))
		return 0;
	if (!find_method(ret->BitmapClass, "getHeight", "()I", &ret->Bitmap_getHeight))
		return 0;
	if (!find_method(ret->BitmapClass, "getWidth", "()I", &ret->Bitmap_getWidth))
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
	if (!find_method(ret->ResourcesClass, "getIdentifier", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I", &ret->Resources_getIdentifier))
		return 0;

	if (!find_method(ret->ContextClass, "getPackageName", "()Ljava/lang/String;", &Context_getPackageName))
		return 0;
	ret->packageName=(*jnienv)->CallObjectMethod(jnienv, android_activity, Context_getPackageName);
	ret->packageName=(*jnienv)->NewGlobalRef(jnienv, ret->packageName);

	if (!find_class_global("org/navitproject/navit/NavitGraphics", &ret->NavitGraphicsClass))
		return 0;
	dbg(0,"at 3\n");
	cid = (*jnienv)->GetMethodID(jnienv, ret->NavitGraphicsClass, "<init>", "(Landroid/app/Activity;Lorg/navitproject/navit/NavitGraphics;IIIIIII)V");
	if (cid == NULL) {
		dbg(0,"no method found\n");
		return 0; /* exception thrown */
	}
	dbg(0,"at 4 android_activity=%p\n",android_activity);
	ret->NavitGraphics=(*jnienv)->NewObject(jnienv, ret->NavitGraphicsClass, cid, android_activity, parent ? parent->NavitGraphics : NULL, pnt ? pnt->x:0 , pnt ? pnt->y:0, w, h, alpha, wraparound, use_camera);
	dbg(0,"result=%p\n",ret->NavitGraphics);
	if (ret->NavitGraphics)
		ret->NavitGraphics = (*jnienv)->NewGlobalRef(jnienv, ret->NavitGraphics);

	/* Create a single global Paint, otherwise android will quickly run out
	 * of global refs.*/
	/* 0x101 = text kerning (default), antialiasing */
	ret->Paint=(*jnienv)->NewObject(jnienv, ret->PaintClass, ret->Paint_init, 0x101);

	dbg(0,"result=%p\n",ret->Paint);
	if (ret->Paint)
		ret->Paint = (*jnienv)->NewGlobalRef(jnienv, ret->Paint);

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

	cid = (*jnienv)->GetMethodID(jnienv, ret->NavitGraphicsClass, "setKeypressCallback", "(I)V");
	if (cid == NULL) {
		dbg(0,"no SetKeypressCallback method found\n");
		return 0; /* exception thrown */
	}
	cb=callback_new_1(callback_cast(keypress_callback), ret);
	(*jnienv)->CallVoidMethod(jnienv, ret->NavitGraphics, cid, (int)cb);

	if (!find_method(ret->NavitGraphicsClass, "draw_polyline", "(Landroid/graphics/Paint;[I)V", &ret->NavitGraphics_draw_polyline))
		return 0;
	if (!find_method(ret->NavitGraphicsClass, "draw_polygon", "(Landroid/graphics/Paint;[I)V", &ret->NavitGraphics_draw_polygon))
		return 0;
	if (!find_method(ret->NavitGraphicsClass, "draw_rectangle", "(Landroid/graphics/Paint;IIII)V", &ret->NavitGraphics_draw_rectangle))
		return 0;
	if (!find_method(ret->NavitGraphicsClass, "draw_circle", "(Landroid/graphics/Paint;III)V", &ret->NavitGraphics_draw_circle))
		return 0;
	if (!find_method(ret->NavitGraphicsClass, "draw_text", "(Landroid/graphics/Paint;IILjava/lang/String;III)V", &ret->NavitGraphics_draw_text))
		return 0;
	if (!find_method(ret->NavitGraphicsClass, "draw_image", "(Landroid/graphics/Paint;IILandroid/graphics/Bitmap;)V", &ret->NavitGraphics_draw_image))
		return 0;
	if (!find_method(ret->NavitGraphicsClass, "draw_mode", "(I)V", &ret->NavitGraphics_draw_mode))
		return 0;
	if (!find_method(ret->NavitGraphicsClass, "draw_drag", "(II)V", &ret->NavitGraphics_draw_drag))
		return 0;
	if (!find_method(ret->NavitGraphicsClass, "overlay_disable", "(I)V", &ret->NavitGraphics_overlay_disable))
		return 0;
	if (!find_method(ret->NavitGraphicsClass, "overlay_resize", "(IIIIII)V", &ret->NavitGraphics_overlay_resize))
		return 0;
	if (!find_method(ret->NavitGraphicsClass, "SetCamera", "(I)V", &ret->NavitGraphics_SetCamera))
		return 0;
#if 0
	set_activity(ret->NavitGraphics);
#endif
	return 1;
}

static int
graphics_android_fullscreen(struct window *win, int on)
{
	return 1;
}

static jclass NavitClass;
static jmethodID Navit_disableSuspend, Navit_exit;

static void
graphics_android_disable_suspend(struct window *win)
{
	dbg(1,"enter\n");
	(*jnienv)->CallVoidMethod(jnienv, android_activity, Navit_disableSuspend);
}


static struct graphics_priv *
graphics_android_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs, struct callback_list *cbl)
{
	struct graphics_priv *ret;
	struct attr *attr;
	int use_camera=0;
	if (!event_request_system("android","graphics_android"))
		return NULL;
	ret=g_new0(struct graphics_priv, 1);

	ret->cbl=cbl;
	*meth=graphics_methods;
	ret->win.priv=ret;
	ret->win.fullscreen=graphics_android_fullscreen;
	ret->win.disable_suspend=graphics_android_disable_suspend;
	if ((attr=attr_search(attrs, NULL, attr_use_camera))) {
		use_camera=attr->u.num;
	}
	image_cache_hash = g_hash_table_new(g_str_hash, g_str_equal);
	if (graphics_android_init(ret, NULL, NULL, 0, 0, 0, 0, use_camera)) {
		dbg(0,"returning %p\n",ret);
		return ret;
	} else {
		g_free(ret);
		return NULL;
	}
}

static struct graphics_priv *
overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h, int alpha, int wraparound)
{
	struct graphics_priv *ret=g_new0(struct graphics_priv, 1);
	*meth=graphics_methods;
	if (graphics_android_init(ret, gr, p, w, h, alpha, wraparound, 0)) {
		dbg(0,"returning %p\n",ret);
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


static void do_poll(JNIEnv *env, int fd, int cond)
{
	struct pollfd pfd;
	pfd.fd=fd;
	dbg(1,"%p poll called for %d %d\n", fd, cond);
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

static struct event_watch *
event_android_add_watch(void *h, enum event_watch_cond cond, struct callback *cb)
{
	jobject ret;
	ret=(*jnienv)->NewObject(jnienv, NavitWatchClass, NavitWatch_init, (int)do_poll, (int) h, (int) cond, (int)cb);
	dbg(0,"result for %p,%d,%p=%p\n",h,cond,cb,ret);
	if (ret)
		ret = (*jnienv)->NewGlobalRef(jnienv, ret);
	return (struct event_watch *)ret;
}

static void
event_android_remove_watch(struct event_watch *ev)
{
	dbg(0,"enter %p\n",ev);
	if (ev) {
		jobject obj=(jobject )ev;
		(*jnienv)->CallVoidMethod(jnienv, obj, NavitWatch_remove);
		(*jnienv)->DeleteGlobalRef(jnienv, obj);
	}
}

struct event_timeout
{
	void (*handle_timeout)(struct event_timeout *priv);
	jobject jni_timeout;
	int multi;
	struct callback *cb;
};

static void
event_android_remove_timeout(struct event_timeout *priv)
{
	if (priv && priv->jni_timeout) {
		(*jnienv)->CallVoidMethod(jnienv, priv->jni_timeout, NavitTimeout_remove);
		(*jnienv)->DeleteGlobalRef(jnienv, priv->jni_timeout);
		priv->jni_timeout = NULL;
	}
}

static void event_android_handle_timeout(struct event_timeout *priv)
{
	callback_call_0(priv->cb);
	if (!priv->multi)
		event_android_remove_timeout(priv);
}

static struct event_timeout *
event_android_add_timeout(int timeout, int multi, struct callback *cb)
{
	struct event_timeout *ret = g_new0(struct event_timeout, 1);
	ret->cb = cb;
	ret->multi = multi;
	ret->handle_timeout = event_android_handle_timeout;
	ret->jni_timeout = (*jnienv)->NewObject(jnienv, NavitTimeoutClass, NavitTimeout_init, timeout, multi, (int)ret);
	if (ret->jni_timeout)
		ret->jni_timeout = (*jnienv)->NewGlobalRef(jnienv, ret->jni_timeout);
	return ret;
}


static struct event_idle *
event_android_add_idle(int priority, struct callback *cb)
{
#if 0
	jobject ret;
        dbg(1,"enter\n");
	ret=(*jnienv)->NewObject(jnienv, NavitIdleClass, NavitIdle_init, (int)cb);
	dbg(1,"result for %p=%p\n",cb,ret);
	if (ret)
		ret = (*jnienv)->NewGlobalRef(jnienv, ret);
	return (struct event_idle *)ret;
#endif
	return (struct event_idle *)event_android_add_timeout(1, 1, cb);
}

static void
event_android_remove_idle(struct event_idle *ev)
{
#if 0
	dbg(1,"enter %p\n",ev);
	if (ev) {
		jobject obj=(jobject )ev;
		(*jnienv)->CallVoidMethod(jnienv, obj, NavitIdle_remove);
		(*jnienv)->DeleteGlobalRef(jnienv, obj);
	}
#endif
	event_android_remove_timeout((struct event_timeout *)ev);
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
	dbg(0,"enter\n");
	if (!find_class_global("org/navitproject/navit/NavitTimeout", &NavitTimeoutClass))
		return NULL;
	NavitTimeout_init = (*jnienv)->GetMethodID(jnienv, NavitTimeoutClass, "<init>", "(IZI)V");
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
	NavitWatch_init = (*jnienv)->GetMethodID(jnienv, NavitWatchClass, "<init>", "(IIII)V");
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
	Navit_exit = (*jnienv)->GetMethodID(jnienv, NavitClass, "exit", "()V");
	if (Navit_exit == NULL) 
		return NULL;
	dbg(0,"ok\n");
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

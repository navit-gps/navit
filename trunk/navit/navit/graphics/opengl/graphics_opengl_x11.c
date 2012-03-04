#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <glib.h>
#include "debug.h"
#include "graphics_opengl.h"

struct graphics_opengl_window_system {
	Display *display;
	int screen;
	Window window,root_window;
	XVisualInfo visual;
	Colormap colormap;
};


static void
graphics_opengl_x11_destroy(struct graphics_opengl_window_system *x11)
{
	if (x11->display) {
		if (x11->window)
			XDestroyWindow(x11->display, x11->window);
		if (x11->colormap) 
			XFreeColormap(x11->display, x11->colormap);
		XCloseDisplay(x11->display);
	}
	g_free(x11);
}

static void *
graphics_opengl_get_display(struct graphics_opengl_window_system *x11)
{
	return x11->display;
}

static void *
graphics_opengl_get_window(struct graphics_opengl_window_system *x11)
{
	return (void *)x11->window;
}

struct graphics_opengl_window_system_methods graphics_opengl_x11_methods = {
	graphics_opengl_x11_destroy,
	graphics_opengl_get_display,
	graphics_opengl_get_window,
};	
	
struct graphics_opengl_window_system *
graphics_opengl_x11_new(int w, int h, int depth, struct graphics_opengl_window_system_methods **methods)
{
	struct graphics_opengl_window_system *ret=g_new0(struct graphics_opengl_window_system, 1);
	XSetWindowAttributes attributes;
	unsigned long valuemask;

	*methods=&graphics_opengl_x11_methods;
	ret->display=XOpenDisplay(NULL);
	if (!ret->display) {
		dbg(0,"failed to open display\n");
		goto error;
	}
	ret->screen=XDefaultScreen(ret->display);
	ret->root_window=RootWindow(ret->display, ret->screen);
	if (!XMatchVisualInfo(ret->display, ret->screen, depth, TrueColor, &ret->visual)) {
		dbg(0,"failed to find visual\n");
		goto error;
	}
	ret->colormap=XCreateColormap(ret->display, ret->root_window, ret->visual.visual, AllocNone);
	valuemask = /* CWBackPixel | */ CWBorderPixel | CWEventMask | CWColormap ; // | CWBackingStore;
	attributes.colormap = ret->colormap;
	attributes.border_pixel = 0;
	attributes.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask;
	attributes.backing_store = Always;
	ret->window=XCreateWindow(ret->display, RootWindow(ret->display, ret->screen), 0, 0, w, h, 0, ret->visual.depth, InputOutput, ret->visual.visual, valuemask, &attributes);
	XMapWindow(ret->display, ret->window);
	XFlush(ret->display);
	return ret;
error:
	graphics_opengl_x11_destroy(ret);
	return NULL;	
}

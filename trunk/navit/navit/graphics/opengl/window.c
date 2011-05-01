#define USE_OPENGLES2 1

#if USE_OPENGLES2
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#else
#include <GLES/gl.h>
#include <GLES/egl.h>
#endif
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "debug.h"

EGLSurface eglwindow;
EGLDisplay egldisplay;
Display *dpy;
Window window;

#if USE_OPENGLES2
static EGLint attributeList[] = {
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,
	EGL_DEPTH_SIZE, 16,
	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	EGL_NONE
};

#else
static EGLint attributeList[] = {
	EGL_RED_SIZE, 1,
	EGL_DEPTH_SIZE, 1,
	EGL_NONE
};
#endif


int
createEGLWindow(int width, int height, char *name)
{
	EGLConfig config[4];
	EGLContext cx;
	int nconfig;
	XSizeHints sizehints;
	XSetWindowAttributes swa;
	XVisualInfo *vi, tmp;
	int vid, n;

#if USE_OPENGLES2
	EGLint aEGLContextAttributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
#endif


	egldisplay = eglGetDisplay(dpy = XOpenDisplay(NULL));
	eglInitialize(egldisplay, 0, 0);
	if (!eglChooseConfig
	    (egldisplay, attributeList, config,
	     sizeof config / sizeof config[0], &nconfig)) {
		dbg(0, "can't find requested config\n");
		return 0;
	}
#if USE_OPENGLES2
	cx = eglCreateContext(egldisplay, config[0], EGL_NO_CONTEXT, aEGLContextAttributes);
#else
	cx = eglCreateContext(egldisplay, config[0], EGL_NO_CONTEXT, NULL);
#endif

	eglGetConfigAttrib(egldisplay, config[0], EGL_NATIVE_VISUAL_ID,
			   &vid);
	tmp.visualid = vid;
	vi = XGetVisualInfo(dpy, VisualIDMask, &tmp, &n);
	swa.colormap =
	    XCreateColormap(dpy, RootWindow(dpy, vi->screen), vi->visual,
			    AllocNone);
	sizehints.flags = 0;
	sizehints.flags = PMinSize | PMaxSize;
	sizehints.min_width = sizehints.max_width = width;
	sizehints.min_height = sizehints.max_height = height;


	swa.border_pixel = 0;
	swa.event_mask =
	    ExposureMask | StructureNotifyMask | KeyPressMask |
	    ButtonPressMask | ButtonReleaseMask;
	window =
	    XCreateWindow(dpy, RootWindow(dpy, vi->screen), 0, 0, width,
			  height, 0, vi->depth, InputOutput, vi->visual,
			  CWBorderPixel | CWColormap | CWEventMask, &swa);
	XMapWindow(dpy, window);
	XSetStandardProperties(dpy, window, name, name, None, (void *) 0, 0, &sizehints);

	eglwindow = eglCreateWindowSurface(egldisplay, config[0], (NativeWindowType) window, 0);
	eglMakeCurrent(egldisplay, eglwindow, eglwindow, cx);
	return 1;
}

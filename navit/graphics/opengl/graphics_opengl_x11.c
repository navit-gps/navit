#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <glib.h>
#include "debug.h"
#include "event.h"
#include "callback.h"
#include "graphics_opengl.h"

struct graphics_opengl_window_system {
    Display *display;
    int screen;
    Window window,root_window;
    XVisualInfo visual;
    Colormap colormap;
    struct callback *cb;
    struct event_watch *watch;
    void *data;
    void (*resize)(void *data, int w, int h);
    void (*button)(void *data, int button, int state, int x, int y);
    void (*motion)(void *data, int x, int y);
    void *keypress;

};


static void graphics_opengl_x11_destroy(struct graphics_opengl_window_system *x11) {
    if (x11->watch)
        event_remove_watch(x11->watch);
    if (x11->cb)
        callback_destroy(x11->cb);
    if (x11->display) {
        if (x11->window)
            XDestroyWindow(x11->display, x11->window);
        if (x11->colormap)
            XFreeColormap(x11->display, x11->colormap);
        XCloseDisplay(x11->display);
    }
    g_free(x11);
}

static void *graphics_opengl_get_display(struct graphics_opengl_window_system *x11) {
    return x11->display;
}

static void *graphics_opengl_get_window(struct graphics_opengl_window_system *x11) {
    return (void *)x11->window;
}

static void graphics_opengl_set_callbacks(struct graphics_opengl_window_system *x11, void *data, void *resize, void *button, void *motion, void *keypress) {
    x11->data=data;
    x11->resize=resize;
    x11->button=button;
    x11->motion=motion;
    x11->keypress=keypress;
}


struct graphics_opengl_window_system_methods graphics_opengl_x11_methods = {
    graphics_opengl_x11_destroy,
    graphics_opengl_get_display,
    graphics_opengl_get_window,
    graphics_opengl_set_callbacks,
};

static void graphics_opengl_x11_watch(struct graphics_opengl_window_system *x11) {
    XEvent event;
    while (XPending(x11->display)) {
        XNextEvent(x11->display, &event);
        switch(event.type) {
        case ButtonPress:
            if (x11->button)
                x11->button(x11->data,event.xbutton.button,1,event.xbutton.x,event.xbutton.y);
            break;
        case ButtonRelease:
            if (x11->button)
                x11->button(x11->data,event.xbutton.button,0,event.xbutton.x,event.xbutton.y);
            break;
        case ConfigureNotify:
            dbg(lvl_debug,"ConfigureNotify");
            break;
        case Expose:
            dbg(lvl_debug,"Expose");
            break;
        case KeyPress:
            dbg(lvl_debug,"KeyPress");
            break;
        case KeyRelease:
            dbg(lvl_debug,"KeyRelease");
            break;
        case MapNotify:
            dbg(lvl_debug,"MapNotify");
            break;
        case MotionNotify:
            if (x11->motion)
                x11->motion(x11->data,event.xmotion.x,event.xmotion.y);
            break;
        case ReparentNotify:
            dbg(lvl_debug,"ReparentNotify");
            break;
        default:
            dbg(lvl_debug,"type %d",event.type);
        }
    }
}

struct graphics_opengl_window_system *
graphics_opengl_x11_new(void *displayname, int w, int h, int depth, struct graphics_opengl_window_system_methods **methods) {
    struct graphics_opengl_window_system *ret=g_new0(struct graphics_opengl_window_system, 1);
    XSetWindowAttributes attributes;
    unsigned long valuemask;

    ret->cb=callback_new_1(callback_cast(graphics_opengl_x11_watch), ret);
    if (!event_request_system("glib", "graphics_opengl_x11_new"))
        goto error;
    *methods=&graphics_opengl_x11_methods;
    ret->display=XOpenDisplay(displayname);
    if (!ret->display) {
        dbg(lvl_error,"failed to open display");
        goto error;
    }
    ret->watch=event_add_watch(ConnectionNumber(ret->display), event_watch_cond_read, ret->cb);
    ret->screen=XDefaultScreen(ret->display);
    ret->root_window=RootWindow(ret->display, ret->screen);
    if (!XMatchVisualInfo(ret->display, ret->screen, depth, TrueColor, &ret->visual)) {
        dbg(lvl_error,"failed to find visual");
        goto error;
    }
    ret->colormap=XCreateColormap(ret->display, ret->root_window, ret->visual.visual, AllocNone);
    valuemask = /* CWBackPixel | */ CWBorderPixel | CWEventMask | CWColormap ; // | CWBackingStore;
    attributes.colormap = ret->colormap;
    attributes.border_pixel = 0;
    attributes.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask | ButtonMotionMask | ButtonReleaseMask |
                            KeyPressMask | KeyReleaseMask;
    attributes.backing_store = Always;
    ret->window=XCreateWindow(ret->display, RootWindow(ret->display, ret->screen), 0, 0, w, h, 0, ret->visual.depth,
                              InputOutput, ret->visual.visual, valuemask, &attributes);
    XMapWindow(ret->display, ret->window);
    XFlush(ret->display);
    graphics_opengl_x11_watch(ret);
    return ret;
error:
    graphics_opengl_x11_destroy(ret);
    return NULL;
}

#include "config.h"
#ifdef USE_OPENGLES2
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#else
#include <GLES/gl.h>
#include <GLES/egl.h>
#endif
#include <glib.h>
#include "graphics_opengl.h"
#include "debug.h"

struct graphics_opengl_platform {
    EGLSurface eglwindow;
    EGLDisplay egldisplay;
    EGLConfig config[1];
    EGLContext context;
};

static EGLint attributeList[] = {
    EGL_RED_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
#ifdef USE_OPENGLES2
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
#endif
    EGL_NONE
};

EGLint aEGLContextAttributes[] = {
#ifdef USE_OPENGLES2
    EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
    EGL_NONE
};

static void graphics_opengl_egl_destroy(struct graphics_opengl_platform *egl) {
    g_free(egl);
}

static void graphics_opengl_egl_swap_buffers(struct graphics_opengl_platform *egl) {
    eglSwapBuffers(egl->egldisplay, egl->eglwindow);
}

struct graphics_opengl_platform_methods graphics_opengl_egl_methods = {
    graphics_opengl_egl_destroy,
    graphics_opengl_egl_swap_buffers,
};


struct graphics_opengl_platform *
graphics_opengl_egl_new(void *display, void *window, struct graphics_opengl_platform_methods **methods) {
    struct graphics_opengl_platform *ret=g_new0(struct graphics_opengl_platform,1);
    EGLint major,minor,nconfig;

    *methods=&graphics_opengl_egl_methods;
    ret->egldisplay = eglGetDisplay((EGLNativeDisplayType)display);
    if (!ret->egldisplay) {
        dbg(lvl_error, "can't get display");
        goto error;
    }
    if (!eglInitialize(ret->egldisplay, &major, &minor)) {
        dbg(lvl_error, "eglInitialize failed");
        goto error;
    }
    dbg(lvl_debug,"eglInitialize ok with version %d.%d",major,minor);
    eglBindAPI(EGL_OPENGL_ES_API);
    if (!eglChooseConfig(ret->egldisplay, attributeList, ret->config, sizeof(ret->config)/sizeof(EGLConfig), &nconfig)) {
        dbg(lvl_error, "eglChooseConfig failed");
        goto error;
    }
    if (nconfig != 1) {
        dbg(lvl_error, "unexpected number of configs %d",nconfig);
        goto error;
    }
    ret->eglwindow = eglCreateWindowSurface(ret->egldisplay, ret->config[0], (NativeWindowType) window, NULL);
    if (ret->eglwindow == EGL_NO_SURFACE) {
        dbg(lvl_error, "eglCreateWindowSurface failed");
        goto error;
    }
    ret->context = eglCreateContext(ret->egldisplay, ret->config[0], EGL_NO_CONTEXT, aEGLContextAttributes);
    if (ret->context == EGL_NO_CONTEXT) {
        dbg(lvl_error, "eglCreateContext failed");
        goto error;
    }
    eglMakeCurrent(ret->egldisplay, ret->eglwindow, ret->eglwindow, ret->context);
    return ret;
error:
    graphics_opengl_egl_destroy(ret);
    return NULL;
}


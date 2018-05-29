/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2010 Navit Team
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

#define USE_FLOAT 0
#define REQUIRES_POWER_OF_2 0

#include <glib.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>

#include <time.h>

#include "item.h"
#include "attr.h"
#include "config.h"
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "plugin.h"
#include "event.h"
#include "debug.h"
#include "callback.h"
#include "keys.h"
#include "window.h"
#include "navit/font/freetype/font_freetype.h"
#include "graphics_opengl.h"

#if defined(WINDOWS) || defined(WIN32)
#define PIXEL_FORMAT GL_RGBA
#include <windows.h>
# define sleep(i) Sleep(i * 1000)
#else
#define PIXEL_FORMAT GL_BGRA
#endif

#ifdef HAVE_FREEIMAGE
#include <FreeImage.h>
#endif

#ifdef USE_OPENGLES
#ifdef USE_OPENGLES2
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#undef USE_FLOAT
#define USE_FLOAT 1
#else
#include <GLES/gl.h>
#include <GLES/egl.h>
#endif
#ifdef USE_GLUT_FOR_OPENGLES
#define APIENTRY
#define GLU_TESS_BEGIN                     100100
#define GLU_TESS_VERTEX                    100101
#define GLU_TESS_END                       100102
#define GLU_TESS_COMBINE                   100105
typedef struct GLUtesselator GLUtesselator;
typedef double GLdouble;
#endif
#else
#define glOrthof	glOrtho
#undef USE_FLOAT
#define USE_FLOAT 1
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>		/* glut.h includes gl.h and glu.h */
#endif
#endif

#if USE_FLOAT
#define glF(x)  x
#define glD(x)  x
#define GL_F    GL_FLOAT
typedef GLfloat GLf;
#else
#define glF(x)  ((GLfixed)((x)*(1<<16)))
#define glD(x)  glF(x)
#define GL_F    GL_FIXED
typedef GLfixed GLf;

#define glClearColor    glClearColorx
#define glTranslatef    glTranslatex
#define glRotatef       glRotatex
#define glMaterialfv    glMaterialxv
#define glMaterialf     glMaterialx
#define glOrthof        glOrthox
#define glScalef        glScalex
#define glColor4f       glColor4x
#endif
#ifdef FREEGLUT
#include <GL/freeglut_ext.h>
#endif

#define SCREEN_WIDTH 700
#define SCREEN_HEIGHT 700

//#define MIRRORED_VIEW 1

struct graphics_gc_priv {
    struct graphics_priv *gr;
    float fr, fg, fb, fa;
    float br, bg, bb, ba;
    int linewidth;
    unsigned char *dash_list;
    int dash_count;
    int dash_mask;
} graphics_gc_priv;

struct graphics_priv {
    int button_timeout;
    struct point p;
    int width;
    int height;
    int library_init;
    int visible;
    int overlay_enabled;
    int overlay_autodisabled;
    int wraparound;
    struct graphics_priv *parent;
    struct graphics_priv *overlays;
    struct graphics_priv *next;
    struct graphics_gc_priv *background_gc;
    enum draw_mode_num mode;
    void (*resize_callback) (void *data, int w, int h);
    void *resize_callback_data;
    void (*motion_callback) (void *data, struct point * p);
    void *motion_callback_data;
    void (*button_callback) (void *data, int press, int button, struct point * p);
    void *button_callback_data;
#ifdef USE_OPENGLES
    GLuint program;
    GLint mvp_location, position_location, color_location, texture_position_location, use_texture_location,
          texture_location;
#else
    GLuint DLid;
#endif
    struct callback_list *cbl;
    struct font_freetype_methods freetype_methods;
    struct navit *nav;
    int timeout;
    int delay;
    struct window window;
    int dirty;		//display needs to be redrawn (draw on root graphics or overlay is done)
    int force_redraw;	//display needs to be redrawn (draw on root graphics or overlay is done)
    time_t last_refresh_time;	//last display refresh time
    struct graphics_opengl_window_system *window_system;
    struct graphics_opengl_window_system_methods *window_system_methods;
    struct graphics_opengl_platform *platform;
    struct graphics_opengl_platform_methods *platform_methods;
};

static struct graphics_priv *graphics_priv_root;
struct graphics_image_priv {
    int w;
    int h;
    int hot_x;
    int hot_y;
    unsigned char *data;
    char *path;
} graphics_image_priv;

struct mouse_event_queue_element {
    int button;
    int state;
    int x;
    int y;
};

static const int mouse_event_queue_size = 100;
static int mouse_event_queue_begin_idx = 0;
static int mouse_event_queue_end_idx = 0;
static struct mouse_event_queue_element mouse_queue[100];

//hastable for uncompressed image data
static GHashTable *hImageData;

#ifdef USE_OPENGLES
#else
/*  prototypes */
void APIENTRY tessBeginCB(GLenum which);
void APIENTRY tessEndCB(void);
void APIENTRY tessErrorCB(GLenum errorCode);
void APIENTRY tessVertexCB(const GLvoid * data);
void APIENTRY tessVertexCB2(const GLvoid * data);
void APIENTRY tessCombineCB(GLdouble c[3], void *d[4], GLfloat w[4], void **out);
const char *getPrimitiveType(GLenum type);
#endif

static struct graphics_priv *graphics_opengl_new_helper(struct
        graphics_methods
        *meth);
static void display(void);
static void resize_callback(int w, int h);
#ifdef USE_OPENGLES
static void click_notify_do(struct graphics_priv *priv, int button, int state, int x, int y);
#endif
static void motion_notify_do(struct graphics_priv *priv, int x, int y);
static void resize_callback_do(struct graphics_priv *priv, int w, int h);
static void glut_close(void);

#ifdef USE_OPENGLES2
const char vertex_src [] =
    "                                        \
   attribute vec2        position;       \
   attribute vec2        texture_position;       \
   uniform mat4        mvp;             \
   varying vec2 v_texture_position; \
                                         \
   void main()                           \
   {                                     \
      v_texture_position=texture_position; \
      gl_Position = mvp*vec4(position, 0.0, 1.0);   \
   }                                     \
";

const char fragment_src [] =
    "                                                      \
   uniform lowp vec4   avcolor;                        \
   uniform sampler2D texture; \
   uniform bool use_texture; \
   varying vec2 v_texture_position; \
   void  main()                                        \
   {                                                   \
      if (use_texture) { \
         gl_FragColor = texture2D(texture, v_texture_position);     \
      } else { \
         gl_FragColor = avcolor; \
      } \
   }                                                   \
";
#endif

static void graphics_destroy(struct graphics_priv *gr) {
    /*FIXME graphics_destroy is never called */
    /*TODO add destroy code for image cache(delete entries in hImageData) */
    gr->freetype_methods.destroy();
    g_free(gr);
    gr = NULL;
}

static void gc_destroy(struct graphics_gc_priv *gc) {
    g_free(gc);
    gc = NULL;
}

static void gc_set_linewidth(struct graphics_gc_priv *gc, int w) {
    gc->linewidth = w;
}

static void gc_set_dashes(struct graphics_gc_priv *gc, int width, int offset, unsigned char *dash_list, int n) {
    int i;
    const int cOpenglMaskBits = 16;
    gc->dash_count = n;
    if (1 == n) {
        gc->dash_mask = 0;
        for (i = 0; i < cOpenglMaskBits; ++i) {
            gc->dash_mask <<= 1;
            gc->dash_mask |= (i / n) % 2;
        }
    } else if (1 < n) {
        unsigned char *curr = dash_list;
        int cnt = 0;	//dot counter
        int dcnt = 0;	//dash element counter
        int sum_dash = 0;
        gc->dash_mask = 0;

        for (i = 0; i < n; ++i) {
            sum_dash += dash_list[i];
        }

        //scale dashlist elements to max size
        if (sum_dash > cOpenglMaskBits) {
            int num_error[2] = { 0, 0 };	//count elements rounded to 0 for odd(drawn) and even(masked) for compensation
            double factor = (1.0 * cOpenglMaskBits) / sum_dash;
            for (i = 0; i < n; ++i) {	//calculate dashlist max and largest common denomiator for scaling
                dash_list[i] *= factor;
                if (dash_list[i] == 0) {
                    ++dash_list[i];
                    ++num_error[i % 2];
                } else if (0 < num_error[i % 2]
                           && 2 < dash_list[i]) {
                    ++dash_list[i];
                    --num_error[i % 2];
                }
            }
        }
        //calculate mask
        for (i = 0; i < cOpenglMaskBits; ++i) {
            gc->dash_mask <<= 1;
            gc->dash_mask |= 1 - dcnt % 2;
            ++cnt;
            if (cnt == *curr) {
                cnt = 0;
                ++curr;
                ++dcnt;
                if (dcnt == n) {
                    curr = dash_list;
                }
            }
        }
    }
}


static void gc_set_foreground(struct graphics_gc_priv *gc, struct color *c) {
    gc->fr = c->r / 65535.0;
    gc->fg = c->g / 65535.0;
    gc->fb = c->b / 65535.0;
    gc->fa = c->a / 65535.0;
}

static void gc_set_background(struct graphics_gc_priv *gc, struct color *c) {
    gc->br = c->r / 65535.0;
    gc->bg = c->g / 65535.0;
    gc->bb = c->b / 65535.0;
    gc->ba = c->a / 65535.0;
}

static struct graphics_gc_methods gc_methods = {
    gc_destroy,
    gc_set_linewidth,
    gc_set_dashes,
    gc_set_foreground,
    gc_set_background
};

static struct graphics_gc_priv *gc_new(struct graphics_priv *gr, struct graphics_gc_methods *meth) {
    struct graphics_gc_priv *gc = g_new0(struct graphics_gc_priv, 1);

    *meth = gc_methods;
    gc->gr = gr;
    gc->linewidth = 1;
    return gc;
}

static struct graphics_image_priv image_error;

static struct graphics_image_priv *image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *path, int *w, int *h, struct point *hot, int rotation) {
#ifdef HAVE_FREEIMAGE
    FIBITMAP *image;
    RGBQUAD aPixel;
    unsigned char *data;
    int width, height, i, j;
    struct graphics_image_priv *gi;
    //check if image already exists in hashmap
    struct graphics_image_priv *curr_elem =
        g_hash_table_lookup(hImageData, path);
    if (curr_elem == &image_error) {
        //found but couldn't be loaded
        return NULL;
    } else if (curr_elem) {
        //found and OK -> use hastable entry
        *w = curr_elem->w;
        *h = curr_elem->h;
        hot->x = curr_elem->w / 2 - 1;
        hot->y = curr_elem->h / 2 - 1;
        return curr_elem;
    } else {
        if (strlen(path) < 4) {
            g_hash_table_insert(hImageData, g_strdup(path), &image_error);
            return NULL;
        }
        char *ext_str = path + strlen(path) - 3;
        if (strstr(ext_str, "png") || strstr(path, "PNG")) {
            if ((image =
                        FreeImage_Load(FIF_PNG, path, 0)) == NULL) {
                g_hash_table_insert(hImageData, g_strdup(path), &image_error);
                return NULL;
            }
        } else if (strstr(ext_str, "xpm") || strstr(path, "XPM")) {
            if ((image =
                        FreeImage_Load(FIF_XPM, path, 0)) == NULL) {
                g_hash_table_insert(hImageData, g_strdup(path), &image_error);
                return NULL;
            }
        } else if (strstr(ext_str, "svg") || strstr(path, "SVG")) {
            char path_new[256];
            snprintf(path_new, strlen(path) - 3, "%s", path);
            strcat(path_new, "_48_48.png");

            if ((image = FreeImage_Load(FIF_PNG, path_new, 0)) == NULL) {
                g_hash_table_insert(hImageData, g_strdup(path), &image_error);
                return NULL;
            }
        } else {
            g_hash_table_insert(hImageData, g_strdup(path), &image_error);
            return NULL;
        }

        if (FreeImage_GetBPP(image) == 64) {
            FIBITMAP *image2;
            image2 = FreeImage_ConvertTo32Bits(image);
            FreeImage_Unload(image);
            image = image2;
        }
#if FREEIMAGE_MAJOR_VERSION*100+FREEIMAGE_MINOR_VERSION  >= 313
        if (rotation) {
            FIBITMAP *image2;
            image2 = FreeImage_Rotate(image, rotation, NULL);
            image = image2;
        }
#endif

        gi = g_new0(struct graphics_image_priv, 1);

        width = FreeImage_GetWidth(image);
        height = FreeImage_GetHeight(image);

        if ((*w != width || *h != height) && *w != IMAGE_W_H_UNSET && *h != IMAGE_W_H_UNSET) {
            FIBITMAP *image2;
            image2 = FreeImage_Rescale(image, *w, *h, FILTER_BOX);
            FreeImage_Unload(image);
            image = image2;
            width = *w;
            height = *h;
        }

        data = (unsigned char *) malloc(width * height * 4);

        RGBQUAD *palette = NULL;
        if (FreeImage_GetBPP(image) == 8) {
            palette = FreeImage_GetPalette(image);
        }

        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                unsigned char idx;
                if (FreeImage_GetBPP(image) == 8) {
                    FreeImage_GetPixelIndex(image, j, height - i - 1, &idx);
                    data[4 * width * i + 4 * j + 0] =
                        palette[idx].rgbRed;
                    data[4 * width * i + 4 * j + 1] =
                        palette[idx].rgbGreen;
                    data[4 * width * i + 4 * j + 2] =
                        palette[idx].rgbBlue;
                    data[4 * width * i + 4 * j + 3] =
                        255;
                } else if (FreeImage_GetBPP(image) == 16
                           || FreeImage_GetBPP(image) == 24
                           || FreeImage_GetBPP(image) == 32) {
                    FreeImage_GetPixelColor(image, j, height - i - 1, &aPixel);
                    int transparent =
                        (aPixel.rgbRed == 0
                         && aPixel.rgbBlue == 0
                         && aPixel.rgbGreen == 0);
                    data[4 * width * i + 4 * j + 0] =
                        transparent ? 0 : (aPixel.
                                           rgbRed);
                    data[4 * width * i + 4 * j + 1] =
                        (aPixel.rgbGreen);
                    data[4 * width * i + 4 * j + 2] =
                        transparent ? 0 : (aPixel.
                                           rgbBlue);
                    data[4 * width * i + 4 * j + 3] =
                        transparent ? 0 : 255;

                }
            }
        }

        FreeImage_Unload(image);

        *w = width;
        *h = height;
        gi->w = width;
        gi->h = height;
        gi->hot_x = width / 2 - 1;
        gi->hot_y = height / 2 - 1;
        hot->x = width / 2 - 1;
        hot->y = height / 2 - 1;
        gi->data = data;
        gi->path = path;
        //add to hashtable
        g_hash_table_insert(hImageData, g_strdup(path), gi);
        return gi;
    }
#else
    dbg(lvl_error,"FreeImage not available - cannot load any images.");
    return NULL;
#endif
}


static void set_color(struct graphics_priv *gr, struct graphics_gc_priv *gc) {
#ifdef USE_OPENGLES2
    GLfloat col[4];
    col[0]=gc->fr;
    col[1]=gc->fg;
    col[2]=gc->fb;
    col[3]=1.0;
    glUniform4fv(gr->color_location, 1, col);
#else
    glColor4f(glF(gc->fr), glF(gc->fg), glF(gc->fb), glF(gc->fa));
#endif
}

static void draw_array(struct graphics_priv *gr, struct point *p, int count, GLenum mode) {
    int i;
#ifdef USE_OPENGLES
    GLf x[count*2];
#else
    glBegin(mode);
#endif

    for (i = 0 ; i < count ; i++) {
#ifdef USE_OPENGLES
        x[i*2]=glF(p[i].x);
        x[i*2+1]=glF(p[i].y);
#else
        glVertex2f(p[i].x, p[i].y);
#endif
    }
#ifdef USE_OPENGLES
#ifdef USE_OPENGLES2
    glVertexAttribPointer (gr->position_location, 2, GL_FLOAT, 0, 0, x );
#else
    glVertexPointer(2, GL_F, 0, x);
#endif
    glDrawArrays(mode, 0, count);
#else
    glEnd();
#endif
}

static void draw_rectangle_do(struct graphics_priv *gr, struct point *p, int w, int h) {
    struct point pa[4];
    pa[0]=pa[1]=pa[2]=pa[3]=*p;
    pa[0].x+=w;
    pa[1].x+=w;
    pa[1].y+=h;
    pa[3].y+=h;
    draw_array(gr, pa, 4, GL_TRIANGLE_STRIP);
}

#ifdef USE_OPENGLES

static int next_power2(int x) {
    int r=1;
    while (r < x)
        r*=2;
    return r;
}

static void draw_image_es(struct graphics_priv *gr, struct point *p, int w, int h, unsigned char *data) {
    GLf x[8];

    memset(x, 0, sizeof(x));
#if REQUIRES_POWER_OF_2
    int w2=next_power2(w);
    int h2=next_power2(h);
    int y;
    if (w2 != w || h2 != h) {
        char *newpix=g_malloc0(w2*h2*4);
        for (y=0 ; y < h ; y++)
            memcpy(newpix+y*w2*4, data+y*w*4, w*4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w2, h2, 0, GL_RGBA, GL_UNSIGNED_BYTE, newpix);
        g_free(newpix);
        w=w2;
        h=h2;
    } else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
#endif
    x[0]+=glF(1);
    x[2]+=glF(1);
    x[3]+=glF(1);
    x[7]+=glF(1);
#ifdef USE_OPENGLES2
    glUniform1i(gr->use_texture_location, 1);
    glEnableVertexAttribArray(gr->texture_position_location);
    glVertexAttribPointer (gr->texture_position_location, 2, GL_FLOAT, 0, 0, x );
#else
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_TEXTURE_2D);
    glTexCoordPointer(2, GL_F, 0, x);
#endif
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw_rectangle_do(gr, p, w, h);
#ifdef USE_OPENGLES2
    glUniform1i(gr->use_texture_location, 0);
    glDisableVertexAttribArray(gr->texture_position_location);
#else
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_TEXTURE_2D);
#endif
    glDisable(GL_BLEND);
}


#endif

static void get_overlay_pos(struct graphics_priv *gr, struct point *point_out) {
    if (gr->parent == NULL) {
        point_out->x = 0;
        point_out->y = 0;
        return;
    }
    point_out->x = gr->p.x;
    if (point_out->x < 0) {
        point_out->x += gr->parent->width;
    }

    point_out->y = gr->p.y;
    if (point_out->y < 0) {
        point_out->y += gr->parent->height;
    }
}

static void draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count) {
    if ((gr->parent && !gr->parent->overlay_enabled)
            || (gr->parent && gr->parent->overlay_enabled
                && !gr->overlay_enabled)) {
        return;
    }
#if !defined(USE_OPENGLES) || defined(USE_OPENGLES2)
    glLineWidth(gc->linewidth);
#endif
    set_color(gr, gc);
    graphics_priv_root->dirty = 1;

#ifndef USE_OPENGLES
    if (!gr->parent && 0 < gc->dash_count) {
        glLineStipple(1, gc->dash_mask);
        glEnable(GL_LINE_STIPPLE);
    }
#endif
    draw_array(gr, p, count, GL_LINE_STRIP);
#ifndef USE_OPENGLES
    if (!gr->parent && 0 < gc->dash_count) {
        glDisable(GL_LINE_STIPPLE);
    }
#endif
}

#if !defined(USE_OPENGLES) || defined(USE_GLUT_FOR_OPENGLES)
static int tess_count;
static struct point tess_array[512];
static GLenum tess_type;

const char *getPrimitiveType(GLenum type) {
    char *ret = "";

    switch (type) {
    case 0x0000:
        ret = "GL_POINTS";
        break;
    case 0x0001:
        ret = "GL_LINES";
        break;
    case 0x0002:
        ret = "GL_LINE_LOOP";
        break;
    case 0x0003:
        ret = "GL_LINE_STRIP";
        break;
    case 0x0004:
        ret = "GL_TRIANGLES";
        break;
    case 0x0005:
        ret = "GL_TRIANGLE_STRIP";
        break;
    case 0x0006:
        ret = "GL_TRIANGLE_FAN";
        break;
    case 0x0007:
        ret = "GL_QUADS";
        break;
    case 0x0008:
        ret = "GL_QUAD_STRIP";
        break;
    case 0x0009:
        ret = "GL_POLYGON";
        break;
    }
    return ret;
}

void APIENTRY tessBeginCB(GLenum which) {
    dbg(lvl_debug, "glBegin( %s );", getPrimitiveType(which));
    tess_type=which;
    tess_count=0;
}



void APIENTRY tessEndCB(void) {
    dbg(lvl_debug, "glEnd();");
    draw_array(graphics_priv_root, tess_array, tess_count, tess_type);
}



void APIENTRY tessVertexCB(const GLvoid * data) {
    // cast back to double type
    const GLdouble *ptr = (const GLdouble *) data;
    dbg(lvl_debug, "  glVertex3d();");

    tess_array[tess_count].x=ptr[0];
    tess_array[tess_count].y=ptr[1];
    if (tess_count < 511)
        tess_count++;
    else
        dbg(lvl_error,"overflow");
}

void APIENTRY tessCombineCB(GLdouble c[3], void *d[4], GLfloat w[4], void **out) {
    GLdouble *nv = (GLdouble *) malloc(sizeof(GLdouble) * 3);
    nv[0] = c[0];
    nv[1] = c[1];
    nv[2] = c[2];
    *out = nv;
}

#endif


static void draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count) {
    if ((gr->parent && !gr->parent->overlay_enabled)
            || (gr->parent && gr->parent->overlay_enabled
                && !gr->overlay_enabled)) {
        return;
    }
    set_color(gr, gc);
    graphics_priv_root->dirty = 1;
#if defined(USE_OPENGLES) && !defined(USE_GLUT_FOR_OPENGLES)
    draw_array(gr, p, count, GL_LINE_STRIP);
#else

    GLUtesselator *tess = gluNewTess();	// create a tessellator
    if (!tess)
        return;		// failed to create tessellation object, return 0

    GLdouble quad1[count][3];
    int i;
    for (i = 0; i < count; i++) {
        quad1[i][0] = (GLdouble) (p[i].x);
        quad1[i][1] = (GLdouble) (p[i].y);
        quad1[i][2] = 0;
    }


    // register callback functions
    gluTessCallback(tess, GLU_TESS_BEGIN,	(void (APIENTRY *)(void)) tessBeginCB);
    gluTessCallback(tess, GLU_TESS_END,		(void (APIENTRY *)(void)) tessEndCB);
    //     gluTessCallback(tess, GLU_TESS_ERROR, (void (*)(void))tessErrorCB);
    gluTessCallback(tess, GLU_TESS_VERTEX,	(void (APIENTRY *)(void)) tessVertexCB);
    gluTessCallback(tess, GLU_TESS_COMBINE, (void (APIENTRY *)(void)) tessCombineCB);

    // tessellate and compile a concave quad into display list
    // gluTessVertex() takes 3 params: tess object, pointer to vertex coords,
    // and pointer to vertex data to be passed to vertex callback.
    // The second param is used only to perform tessellation, and the third
    // param is the actual vertex data to draw. It is usually same as the second
    // param, but It can be more than vertex coord, for example, color, normal
    // and UV coords which are needed for actual drawing.
    // Here, we are looking at only vertex coods, so the 2nd and 3rd params are
    // pointing same address.
    gluTessBeginPolygon(tess, 0);	// with NULL data
    gluTessBeginContour(tess);
    for (i = 0; i < count; i++) {
        gluTessVertex(tess, quad1[i], quad1[i]);
    }
    gluTessEndContour(tess);
    gluTessEndPolygon(tess);

    gluDeleteTess(tess);	// delete after tessellation
#endif
}

static void draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h) {
    if ((gr->parent && !gr->parent->overlay_enabled)
            || (gr->parent && gr->parent->overlay_enabled
                && !gr->overlay_enabled)) {
        return;
    }
    set_color(gr, gc);
    draw_rectangle_do(gr, p, w, h);
    graphics_priv_root->dirty = 1;
}

static void display_text_draw(struct font_freetype_text *text, struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, int color, struct point *p) {
    int i, x, y, stride;
    struct font_freetype_glyph *g, **gp;
    unsigned char *shadow, *glyph;
    struct color transparent = { 0x0000, 0x0000, 0x0000, 0x0000 };
    struct color black = {
        fg->fr * 65535, fg->fg * 65535, fg->fb * 65535, fg->fa * 65535
    };
    struct color white = { 0xffff, 0xffff, 0xffff, 0xffff };

    if (bg) {
        if (COLOR_IS_WHITE(black) && COLOR_IS_BLACK(white)) {
            black.r = 65535;
            black.g = 65535;
            black.b = 65535;
            black.a = 65535;

            white.r = 0;
            white.g = 0;
            white.b = 0;
            white.a = 65535;
        } else if (COLOR_IS_BLACK(black) && COLOR_IS_WHITE(white)) {
            white.r = 65535;
            white.g = 65535;
            white.b = 65535;
            white.a = 65535;

            black.r = 0;
            black.g = 0;
            black.b = 0;
            black.a = 65535;
        } else {
            white.r = bg->fr;
            white.g = bg->fg;
            white.b = bg->fb;
            white.a = bg->fa;
        }
    } else {
        white.r = 0;
        white.g = 0;
        white.b = 0;
        white.a = 0;
    }

    gp = text->glyph;
    i = text->glyph_count;
    x = p->x << 6;
    y = p->y << 6;
    while (i-- > 0) {
        g = *gp++;
        if (g->w && g->h && bg) {
            stride = (g->w + 2) * 4;
            if (color) {
                shadow = g_malloc(stride * (g->h + 2));
                gr->freetype_methods.get_shadow(g, shadow, stride, &white, &transparent);
#ifdef USE_OPENGLES
                struct point p;
                p.x=((x + g->x) >> 6)-1;
                p.y=((y + g->y) >> 6)-1;
                draw_image_es(gr, &p, g->w+2, g->h+2, shadow);
#else
#ifdef MIRRORED_VIEW
                glPixelZoom(-1.0, -1.0);	//mirrored mode
#else
                glPixelZoom(1.0, -1.0);
#endif
                glRasterPos2d((x + g->x) >> 6, (y + g->y) >> 6);
                glDrawPixels(g->w + 2, g->h + 2, PIXEL_FORMAT, GL_UNSIGNED_BYTE, shadow);
#endif
                g_free(shadow);
            }
        }
        x += g->dx;
        y += g->dy;
    }

    x = p->x << 6;
    y = p->y << 6;
    gp = text->glyph;
    i = text->glyph_count;
    while (i-- > 0) {
        g = *gp++;
        if (g->w && g->h) {
            if (color) {
                stride = g->w;
                if (bg) {
                    glyph =
                        g_malloc(stride * g->h * 4);
                    gr->freetype_methods.get_glyph(g, glyph, stride * 4, &black, &white, &transparent);
#ifdef USE_OPENGLES
                    struct point p;
                    p.x=(x + g->x) >> 6;
                    p.y=(y + g->y) >> 6;
                    draw_image_es(gr, &p, g->w, g->h, glyph);
#else
#ifdef MIRRORED_VIEW
                    glPixelZoom(-1.0, -1.0);	//mirrored mode
#else
                    glPixelZoom(1.0, -1.0);
#endif
                    glRasterPos2d((x + g->x) >> 6, (y + g->y) >> 6);
                    glDrawPixels(g->w, g->h, PIXEL_FORMAT, GL_UNSIGNED_BYTE, glyph);
#endif
                    g_free(glyph);
                }
                stride *= 4;
                glyph = g_malloc(stride * g->h);
                gr->freetype_methods.get_glyph(g, glyph, stride, &black, &white, &transparent);
#ifdef USE_OPENGLES
                struct point p;
                p.x=(x + g->x) >> 6;
                p.y=(y + g->y) >> 6;
                draw_image_es(gr, &p, g->w, g->h, glyph);
#else
#ifdef MIRRORED_VIEW
                glPixelZoom(-1.0, -1.0);	//mirrored mode
#else
                glPixelZoom(1.0, -1.0);
#endif
                glRasterPos2d((x + g->x) >> 6, (y + g->y) >> 6);
                glDrawPixels(g->w, g->h, PIXEL_FORMAT, GL_UNSIGNED_BYTE, glyph);
#endif
                g_free(glyph);
            }
        }
        x += g->dx;
        y += g->dy;
    }
}

static void draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy) {
    if ((gr->parent && !gr->parent->overlay_enabled)
            || (gr->parent && gr->parent->overlay_enabled
                && !gr->overlay_enabled)) {
        return;
    }

    struct font_freetype_text *t;
    int color = 1;

    if (!font) {
        dbg(lvl_error, "no font, returning");
        return;
    }

    graphics_priv_root->dirty = 1;

    t = gr->freetype_methods.text_new(text, (struct font_freetype_font *) font, dx, dy);

    struct point p_eff;
    p_eff.x = p->x;
    p_eff.y = p->y;

    display_text_draw(t, gr, fg, bg, color, &p_eff);
    gr->freetype_methods.text_destroy(t);
}


static void draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img) {
#ifdef USE_OPENGLES
    draw_image_es(gr, p, img->w, img->h, img->data);
#else
    if ((gr->parent && !gr->parent->overlay_enabled)
            || (gr->parent && gr->parent->overlay_enabled
                && !gr->overlay_enabled)) {
        return;
    }

    if (!img || !img->data) {
        return;
    }

    graphics_priv_root->dirty = 1;

    struct point p_eff;
    p_eff.x = p->x + img->hot_x;
    p_eff.y = p->y + img->hot_y;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glRasterPos2d(p_eff.x - img->hot_x, p_eff.y - img->hot_y);
    glDrawPixels(img->w, img->h, GL_RGBA, GL_UNSIGNED_BYTE, img->data);
#endif

}

static void draw_drag(struct graphics_priv *gr, struct point *p) {

    if (p) {
        gr->p.x = p->x;
        gr->p.y = p->y;
    }
}

static void background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc) {
    gr->background_gc = gc;
}

static void handle_mouse_queue(void) {
#ifdef USE_OPENGLES
#else
    static int locked = 0;
    if (!locked) {
        locked = 1;
    } else {
        return;
    }

    if (mouse_event_queue_begin_idx < mouse_event_queue_end_idx) {
        if (mouse_queue[mouse_event_queue_begin_idx].button ==
                GLUT_LEFT_BUTTON
                && mouse_queue[mouse_event_queue_begin_idx].state ==
                GLUT_UP) {
            struct point p;
            p.x =
                mouse_queue[mouse_event_queue_begin_idx %
                                                        mouse_event_queue_size].x;
            p.y =
                mouse_queue[mouse_event_queue_begin_idx %
                                                        mouse_event_queue_size].y;
            graphics_priv_root->force_redraw = 1;
            callback_list_call_attr_3(graphics_priv_root->cbl, attr_button, (void *) 0, 1, (void *) &p);
        } else if (mouse_queue[mouse_event_queue_begin_idx].
                   button == GLUT_LEFT_BUTTON
                   && mouse_queue[mouse_event_queue_begin_idx].
                   state == GLUT_DOWN) {
            struct point p;
            p.x =
                mouse_queue[mouse_event_queue_begin_idx %
                                                        mouse_event_queue_size].x;
            p.y =
                mouse_queue[mouse_event_queue_begin_idx %
                                                        mouse_event_queue_size].y;
            graphics_priv_root->force_redraw = 1;
            callback_list_call_attr_3(graphics_priv_root->cbl, attr_button, (void *) 1, 1, (void *) &p);
        }
        ++mouse_event_queue_begin_idx;
    }
    locked = 0;
#endif
}


/*draws root graphics and its overlays*/
static int redraw_screen(struct graphics_priv *gr) {
#ifdef USE_OPENGLES
#else
    graphics_priv_root->dirty = 0;

    glCallList(gr->DLid);
    //display overlays display list
    struct graphics_priv *overlay;
    overlay = gr->overlays;
    while (gr->overlay_enabled && overlay) {
        if (overlay->overlay_enabled) {
            glPushMatrix();
            struct point p_eff;
            get_overlay_pos(overlay, &p_eff);
            glTranslatef(p_eff.x, p_eff.y, 1);
            glCallList(overlay->DLid);
            glPopMatrix();
        }
        overlay = overlay->next;
    }
    glutSwapBuffers();
#endif

    return TRUE;
}


#ifndef USE_OPENGLES
/*filters call to redraw in overlay enabled(map) mode*/
static gboolean redraw_filter(gpointer data) {
    struct graphics_priv *gr = (struct graphics_priv*) data;
    if (gr->overlay_enabled && gr->dirty) {
        redraw_screen(gr);
    }
    return 0;
}
#endif



static void draw_mode(struct graphics_priv *gr, enum draw_mode_num mode) {
    if (gr->parent) {	//overlay
#ifdef USE_OPENGLES
#else
        if (mode == draw_mode_begin) {
            glNewList(gr->DLid, GL_COMPILE);
        }

        if (mode == draw_mode_end) {
            glEndList();
        }
#endif
    } else {		//root graphics
        if (mode == draw_mode_begin) {
#ifdef USE_OPENGLES
#else
            glNewList(gr->DLid, GL_COMPILE);
#endif
        }

        if (mode == draw_mode_end) {
#ifdef USE_OPENGLES
            gr->platform_methods->swap_buffers(gr->platform);
#else
            glEndList();
            gr->force_redraw = 1;
            if (!gr->overlay_enabled || gr->force_redraw) {
                redraw_screen(gr);
            }
#endif
        }
    }
    gr->mode = mode;
}

static struct graphics_priv *overlay_new(struct graphics_priv *gr,
        struct graphics_methods *meth,
        struct point *p, int w, int h,
        int wraparound);

static int graphics_opengl_fullscreen(struct window *w, int on) {
    return 1;
}

static void graphics_opengl_disable_suspend(struct window *w) {
}

#ifdef USE_OPENGLES2
static GLuint load_shader(const char *shader_source, GLenum type) {
    GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &shader_source, NULL);
    glCompileShader(shader);

    return shader;
}
#endif

static void *get_data(struct graphics_priv *this, const char *type) {
    /*TODO initialize gtkglext context when type=="gtk_widget" */
    if (!strcmp(type, "gtk_widget")) {
        fprintf(stderr, "Currently GTK gui is not yet supported with opengl graphics driver\n");
        return NULL;
    }
    if (strcmp(type, "window") == 0) {
        struct window *win;
#ifdef USE_OPENGLES
        GLuint vertexShader;
        GLuint fragmentShader;
        GLuint textures;
        GLfloat matrix[16];
        int i;

        this->window_system=graphics_opengl_x11_new(NULL, this->width, this->height, 32, &this->window_system_methods);
        this->platform=graphics_opengl_egl_new(this->window_system_methods->get_display(this->window_system),
                                               this->window_system_methods->get_window(this->window_system),
                                               &this->platform_methods);
        this->window_system_methods->set_callbacks(this->window_system, this, resize_callback_do, click_notify_do, motion_notify_do, NULL);
        resize_callback(this->width,this->height);
#if 0
        glClearColor ( 0.4, 0.4, 0.4, 1);
#endif
        glClear ( GL_COLOR_BUFFER_BIT );
#ifdef USE_OPENGLES2
        this->program=glCreateProgram();
        vertexShader = load_shader(vertex_src, GL_VERTEX_SHADER);
        fragmentShader = load_shader(fragment_src, GL_FRAGMENT_SHADER);
        glAttachShader(this->program, vertexShader);
        glAttachShader(this->program, fragmentShader);
        glLinkProgram(this->program);
        glUseProgram(this->program);
        this->mvp_location=glGetUniformLocation(this->program, "mvp");
        this->position_location=glGetAttribLocation(this->program, "position");
        glEnableVertexAttribArray(this->position_location);
        this->texture_position_location=glGetAttribLocation(this->program, "texture_position");
        this->color_location=glGetUniformLocation(this->program, "avcolor");
        this->texture_location=glGetUniformLocation(this->program, "texture");
        this->use_texture_location=glGetUniformLocation(this->program, "use_texture");
        glUniform1i(this->use_texture_location, 0);
        glUniform1i(this->texture_location, 0);

        for (i = 0 ; i < 16 ; i++)
            matrix[i]=0.0;
        matrix[0]=2.0/this->width;
        matrix[5]=-2.0/this->height;
        matrix[10]=1;
        matrix[12]=-1;
        matrix[13]=1;
        matrix[15]=1;
        glUniformMatrix4fv(this->mvp_location, 1, GL_FALSE, matrix);
#else
        glEnableClientState(GL_VERTEX_ARRAY);
#endif
        glGenTextures(1, &textures);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#ifndef USE_OPENGLES2
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif
#endif
        win = g_new0(struct window, 1);
        win->priv = this;
        win->fullscreen = graphics_opengl_fullscreen;
        win->disable_suspend = graphics_opengl_disable_suspend;
        return win;
    } else {
#ifdef USE_OPENGLES
        return NULL;
#else
        return &this->DLid;
#endif
    }


}

static void image_free(struct graphics_priv *gr, struct graphics_image_priv *priv) {
//TODO free image data in hashtable when graphics is destroyed
//currently graphics destroy is not called !!!
    /*
      g_free(priv->data);
      priv->data = NULL;
      g_free(priv);
      priv = NULL;
    */
}

static void overlay_disable(struct graphics_priv *gr, int disable) {
    gr->overlay_enabled = !disable;
    gr->force_redraw = 1;
    draw_mode(gr, draw_mode_end);
}

static void overlay_resize(struct graphics_priv *gr, struct point *p, int w, int h, int wraparound) {
    int changed = 0;
    int w2, h2;

    if (w == 0) {
        w2 = 1;
    } else {
        w2 = w;
    }

    if (h == 0) {
        h2 = 1;
    } else {
        h2 = h;
    }

    gr->p = *p;
    if (gr->width != w2) {
        gr->width = w2;
        changed = 1;
    }

    if (gr->height != h2) {
        gr->height = h2;
        changed = 1;
    }

    gr->wraparound = wraparound;

    if (changed) {
        if ((w == 0) || (h == 0)) {
            gr->overlay_autodisabled = 1;
        } else {
            gr->overlay_autodisabled = 0;
        }

        callback_list_call_attr_2(gr->cbl, attr_resize, GINT_TO_POINTER(gr->width), GINT_TO_POINTER(gr->height));
    }
}

static struct graphics_methods graphics_methods = {
    graphics_destroy,
    draw_mode,
    draw_lines,
    draw_polygon,
    draw_rectangle,
    NULL,
    draw_text,
    draw_image,
    NULL,
    draw_drag,
    NULL,
    gc_new,
    background_gc,
    overlay_new,
    image_new,
    get_data,
    image_free,
    NULL,
    overlay_disable,
    overlay_resize,
    NULL, /* show_native_keyboard */
    NULL, /* hide_native_keyboard */
};

static struct graphics_priv *graphics_opengl_new_helper(struct graphics_methods *meth) {
    struct font_priv *(*font_freetype_new) (void *meth);
    font_freetype_new = plugin_get_category_font("freetype");

    if (!font_freetype_new) {
        return NULL;
    }

    struct graphics_priv *this = g_new0(struct graphics_priv, 1);

    font_freetype_new(&this->freetype_methods);
    *meth = graphics_methods;

    meth->font_new =
        (struct graphics_font_priv *
         (*)(struct graphics_priv *, struct graphics_font_methods *,
             char *, int, int)) this->freetype_methods.font_new;
    meth->get_text_bbox =
        (void (*) (struct graphics_priv *, struct graphics_font_priv *,
                   char *, int, int, struct point*, int)) this->freetype_methods.get_text_bbox;
    return this;
}

static struct graphics_priv *overlay_new(struct graphics_priv *gr, struct graphics_methods *meth,
        struct point *p, int w, int h, int wraparound) {
    struct graphics_priv *this = graphics_opengl_new_helper(meth);
    this->p = *p;
    this->width = w;
    this->height = h;
    this->parent = gr;

    if ((w == 0) || (h == 0)) {
        this->overlay_autodisabled = 1;
    } else {
        this->overlay_autodisabled = 0;
    }
    this->overlay_enabled = 1;
    this->overlay_autodisabled = 0;

    this->next = gr->overlays;
    gr->overlays = this;
#ifdef USE_OPENGLES
#else
    this->DLid = glGenLists(1);
#endif
    return this;
}


#ifdef USE_OPENGLES
static void click_notify_do(struct graphics_priv *priv, int button, int state, int x, int y) {
    struct point p= {x,y};
    dbg(lvl_debug,"enter state %d button %d",state,button);
    callback_list_call_attr_3(priv->cbl, attr_button, (void *) state, (void *)button, (void *) &p);
}
#endif

static void click_notify(int button, int state, int x, int y) {
    mouse_queue[mouse_event_queue_end_idx %
                                          mouse_event_queue_size].button = button;
    mouse_queue[mouse_event_queue_end_idx %
                                          mouse_event_queue_size].state = state;
#ifdef MIRRORED_VIEW
    mouse_queue[mouse_event_queue_end_idx % mouse_event_queue_size].x =
        graphics_priv_root->width - x;
#else
    mouse_queue[mouse_event_queue_end_idx % mouse_event_queue_size].x =
        x;
#endif
    mouse_queue[mouse_event_queue_end_idx % mouse_event_queue_size].y =
        y;
    ++mouse_event_queue_end_idx;
}

static void motion_notify_do(struct graphics_priv *priv, int x, int y) {
    struct point p;
#ifdef MIRRORED_VIEW
    p.x = priv->width - x;
#else
    p.x = x;
#endif
    p.y = y;
    callback_list_call_attr_1(priv->cbl, attr_motion, (void *) &p);
    return;
}

static void motion_notify(int x, int y) {
    motion_notify_do(graphics_priv_root, x, y);
}

#ifndef USE_OPENGLES
static gboolean graphics_opengl_idle(void *data) {
    static int opengl_init_ok = 0;
    if (!opengl_init_ok) {
        callback_list_call_attr_2(graphics_priv_root->cbl, attr_resize, GINT_TO_POINTER (graphics_priv_root->width), GINT_TO_POINTER (graphics_priv_root->height));
        opengl_init_ok = 1;
    } else {

#ifdef FREEGLUT
        glutMainLoopEvent();
#endif
        handle_mouse_queue();
    }
    return TRUE;
}
#endif

static void ProcessNormalKeys(unsigned char key_in, int x, int y) {
    int key = 0;
    char keybuf[2];

    switch (key_in) {
    case 13:
        key = NAVIT_KEY_RETURN;
        break;
    default:
        key = key_in;
        break;
    }
    keybuf[0] = key;
    keybuf[1] = '\0';
    graphics_priv_root->force_redraw = 1;
    callback_list_call_attr_1(graphics_priv_root->cbl, attr_keypress, (void *) keybuf);
}

static void ProcessSpecialKeys(int key_in, int x, int y) {
    int key = 0;
    char keybuf[2];

    switch (key_in) {
    case 102:
        key = NAVIT_KEY_RIGHT;
        break;
    case 100:
        key = NAVIT_KEY_LEFT;
        break;
    case 103:
        key = NAVIT_KEY_DOWN;
        break;
    case 101:
        key = NAVIT_KEY_UP;
        break;
    case 104:
        key = NAVIT_KEY_ZOOM_OUT;
        break;
    case 105:
        key = NAVIT_KEY_ZOOM_IN;
        break;
    default:
        break;
    }			//switch

    graphics_priv_root->force_redraw = 1;
    keybuf[0] = key;
    keybuf[1] = '\0';
    callback_list_call_attr_1(graphics_priv_root->cbl, attr_keypress, (void *) keybuf);
}

static void resize_callback_do(struct graphics_priv *priv, int w, int h) {
    glViewport(0, 0, w, h);
#ifndef USE_OPENGLES2
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
#ifdef MIRRORED_VIEW
    glOrthof(glF(w), glF(0), glF(h), glF(0), glF(1), glF(-1));
#else
    glOrthof(glF(0), glF(w), glF(h), glF(0), glF(1), glF(-1));
#endif
#endif
    priv->width = w;
    priv->height = h;

    callback_list_call_attr_2(priv->cbl, attr_resize, GINT_TO_POINTER(w), GINT_TO_POINTER(h));
}

static void resize_callback(int w, int h) {
    resize_callback_do(graphics_priv_root, w, h);
}

static void display(void) {
    graphics_priv_root->force_redraw = 1;
    redraw_screen(graphics_priv_root);
    resize_callback(graphics_priv_root->width, graphics_priv_root->height);
}

static void glut_close(void) {
    callback_list_call_attr_0(graphics_priv_root->cbl, attr_window_closed);
}


static struct graphics_priv *graphics_opengl_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs, struct callback_list *cbl) {
    struct attr *attr;

    if (!event_request_system("glib", "graphics_opengl_new"))
        return NULL;

    struct graphics_priv *this = graphics_opengl_new_helper(meth);
    graphics_priv_root = this;

    this->nav = nav;
    this->parent = NULL;
    this->overlay_enabled = 1;

    this->width = SCREEN_WIDTH;
    if ((attr = attr_search(attrs, NULL, attr_w)))
        this->width = attr->u.num;
    this->height = SCREEN_HEIGHT;
    if ((attr = attr_search(attrs, NULL, attr_h)))
        this->height = attr->u.num;
    this->timeout = 100;
    if ((attr = attr_search(attrs, NULL, attr_timeout)))
        this->timeout = attr->u.num;
    this->delay = 0;
    if ((attr = attr_search(attrs, NULL, attr_delay)))
        this->delay = attr->u.num;
    this->cbl = cbl;

    char *cmdline = "";
    int argc = 0;
#ifndef USE_OPENGLES
    glutInit(&argc, &cmdline);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

    glutInitWindowSize(this->width, this->height);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("Navit opengl window");

    glutDisplayFunc(display);
    glutReshapeFunc(resize_callback);
    resize_callback(this->width, this->height);
#endif

    graphics_priv_root->cbl = cbl;
    graphics_priv_root->width = this->width;
    graphics_priv_root->height = this->height;

#ifndef USE_OPENGLES
    glutMotionFunc(motion_notify);
    glutPassiveMotionFunc(motion_notify);
    glutMouseFunc(click_notify);
    glutKeyboardFunc(ProcessNormalKeys);
    glutSpecialFunc(ProcessSpecialKeys);
#ifdef FREEGLUT
    glutCloseFunc(glut_close);
#endif
    this->DLid = glGenLists(1);

    g_timeout_add(G_PRIORITY_DEFAULT + 10, graphics_opengl_idle, NULL);

    /*this will only refresh screen in map(overlay enabled) mode */
    g_timeout_add(G_PRIORITY_DEFAULT + 1000, redraw_filter, this);
#endif

    //create hash table for uncompressed image data
    hImageData = g_hash_table_new(g_str_hash, g_str_equal);
    return this;
}


static void event_opengl_main_loop_run(void) {
    dbg(lvl_info, "enter");
}

static void event_opengl_main_loop_quit(void) {
    dbg(lvl_debug, "enter");
}

static struct event_watch *event_opengl_add_watch(int fd, enum event_watch_cond cond, struct callback *cb) {
    dbg(lvl_debug, "enter");
    return NULL;
}

static void event_opengl_remove_watch(struct event_watch *ev) {
    dbg(lvl_debug, "enter");
}


static struct event_timeout *event_opengl_add_timeout(int timeout, int multi, struct callback *cb) {
    dbg(lvl_debug, "enter");
    return NULL;
}

static void event_opengl_remove_timeout(struct event_timeout *to) {
    dbg(lvl_debug, "enter");
}


static struct event_idle *event_opengl_add_idle(int priority, struct callback *cb) {
    dbg(lvl_debug, "enter");
    return NULL;
}

static void event_opengl_remove_idle(struct event_idle *ev) {
    dbg(lvl_debug, "enter");
}

static void event_opengl_call_callback(struct callback_list *cb) {
    dbg(lvl_debug, "enter");
}

static struct event_methods event_opengl_methods = {
    event_opengl_main_loop_run,
    event_opengl_main_loop_quit,
    event_opengl_add_watch,
    event_opengl_remove_watch,
    event_opengl_add_timeout,
    event_opengl_remove_timeout,
    event_opengl_add_idle,
    event_opengl_remove_idle,
    event_opengl_call_callback,
};

static struct event_priv *event_opengl_new(struct event_methods *meth) {
    *meth = event_opengl_methods;
    return NULL;
}

void plugin_init(void) {
    plugin_register_category_graphics("opengl", graphics_opengl_new);
    plugin_register_category_event("opengl", event_opengl_new);
}

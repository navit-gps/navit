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

#include "SDL_image.h"
#include <GLES2/gl2.h>
#include <EGL/egl.h>

/*
    * This is work in progress *
    * Remainng issues :
    * - SDL mouse cursor sometimes raise an SDL assertion
    * - Dashed lines to implement
    * - Full Keyboard handling
*/

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define glF(x)  x
#define glD(x)  x
#define GL_F    GL_FLOAT
typedef GLfloat GLf;

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
    int fill_poly;
    int show_overlays;
    int button_timeout;
    struct point p;
    int width;
    int height;
    int library_init;
    int visible;
    int overlay_enabled;
    int overlay_autodisabled;
    int wraparound;
    GLuint framebuffer_name;
    GLuint overlay_texture;
    struct graphics_priv *parent;
    struct graphics_priv *overlays;
    struct graphics_priv *next;
    struct graphics_gc_priv *background_gc;
    enum draw_mode_num mode;
    GLuint program;
    GLint mvp_location, position_location, color_location, texture_position_location, use_texture_location,
          texture_location;
    struct callback_list *cbl;
    struct font_freetype_methods freetype_methods;
    struct navit *nav;
    int timeout;
    int delay;
    struct window window;
    int dirty;		//display needs to be redrawn (draw on root graphics or overlay is done)
    int force_redraw;	//display needs to be redrawn (draw on root graphics or overlay is done)
    time_t last_refresh_time;	//last display refresh time
    struct graphics_opengl_platform *platform;
    struct graphics_opengl_platform_methods *platform_methods;
};

struct graphics_image_priv {
    SDL_Surface *img;
} graphics_image_priv;


struct graphics_opengl_platform {
    SDL_Window* eglwindow;
    SDL_GLContext eglcontext;
    EGLDisplay egldisplay;
    EGLConfig config[1];
};

struct contour {
    struct point *p;
    unsigned int count;
};

static GHashTable *hImageData;
static int      USERWANTSTOQUIT = 0;
static struct   graphics_priv *graphics_priv_root;
static struct   graphics_priv *graphics_opengl_new_helper(struct
        graphics_methods
        *meth);

/*
 * GLES 2 Compatible vertex and fragment shaders
 * Taken from opengl driver
 */
const char vertex_src [] =
    "                                        \
   attribute vec2        position;       \
   attribute vec2        texture_position;       \
   uniform mat4          mvp;             \
   varying vec2          v_texture_position; \
                                         \
   void main()                           \
   {                                     \
      v_texture_position=texture_position; \
      gl_Position = mvp*vec4(position, 0.0, 1.0);   \
   }                                     \
";

const char fragment_src [] =
    "                                                      \
   uniform lowp vec4    avcolor;                        \
   uniform sampler2D    texture; \
   uniform bool         use_texture; \
   varying vec2         v_texture_position; \
   void  main()                                        \
   {                                                   \
      if (use_texture) { \
         gl_FragColor = texture2D(texture, v_texture_position);     \
      } else { \
         gl_FragColor = avcolor; \
      } \
   }                                                   \
";

/*
* C conversion of Efficient Polygon Triangulation
* Found at http://www.flipcode.com/archives/Efficient_Polygon_Triangulation.shtml
* Adapted and debugged for this use
*/
float area(const struct contour* contour) {
    int p, q;
    int n = contour->count - 1;
    float A = 0.f;

    for (p=n-1, q=0; q < n; p=q++) {
        A += contour->p[p].x * contour->p[q].y - contour->p[q].x * contour->p[p].y;
    }
    return A * .5f;
}

int inside_triangle(float Ax, float Ay,
                    float Bx, float By,
                    float Cx, float Cy,
                    float Px, float Py) {
    float ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
    float cCROSSap, bCROSScp, aCROSSbp;

    ax = Cx - Bx;
    ay = Cy - By;
    bx = Ax - Cx;
    by = Ay - Cy;
    cx = Bx - Ax;
    cy = By - Ay;
    apx= Px - Ax;
    apy= Py - Ay;
    bpx= Px - Bx;
    bpy= Py - By;
    cpx= Px - Cx;
    cpy= Py - Cy;

    aCROSSbp = ax*bpy - ay*bpx;
    cCROSSap = cx*apy - cy*apx;
    bCROSScp = bx*cpy - by*cpx;

    return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));
}

int snip(const struct point* pnt,int u,int v,int w,int n,int *V) {
    int p;
    float Ax, Ay, Bx, By, Cx, Cy, Px, Py;

    Ax = pnt[V[u]].x;
    Ay = pnt[V[u]].y;

    Bx = pnt[V[v]].x;
    By = pnt[V[v]].y;

    Cx = pnt[V[w]].x;
    Cy = pnt[V[w]].y;

    if ( (((Bx-Ax)*(Cy-Ay)) - ((By-Ay)*(Cx-Ax))) < 0.f )
        return 0;

    for (p=0; p<n; p++) {
        if( (p == u) || (p == v) || (p == w) )
            continue;
        Px = pnt[V[p]].x;
        Py = pnt[V[p]].y;
        if (inside_triangle(Ax,Ay,Bx,By,Cx,Cy,Px,Py))
            return 0;
    }

    return 1;
}

int process_triangles(const struct contour* contour, struct contour* result) {
    int v;
    int contour_size = contour->count - 1;
    int polygon_temp_size = contour_size * 80;
    int final_count = 0;
    result->p = malloc(sizeof(struct point) * polygon_temp_size);

    int n = contour_size;
    if ( n < 3 ) return 0;

    int *V = alloca(sizeof(int)*n);

    if ( 0.0f < area(contour) )
        for (v=0; v<n; v++) V[v] = v;
    else
        for(v=0; v<n; v++) V[v] = (n-1)-v;

    int nv = n;

    int count = 2*nv;

    for(v=nv-1; nv>2; ) {
        /* if we loop, it is probably a non-simple polygon */
        if (0 >= (count--)) {
            //** Triangulate: ERROR - probable bad polygon!
            break;
        }

        /* three consecutive vertices in current polygon, <u,v,w> */
        int u = v  ;
        if (nv <= u) u = 0;     /* previous */
        v = u+1;
        if (nv <= v) v = 0;         /* new v    */
        int w = v+1;
        if (nv <= w) w = 0;     /* next     */

        if ( snip(contour->p,u,v,w,nv,V) ) {
            int a,b,c,s,t;

            /* true names of the vertices */
            a = V[u];
            b = V[v];
            c = V[w];

            /* output Triangle */
            result->p[final_count++] = contour->p[a];
            result->p[final_count++] = contour->p[b];
            result->p[final_count++] = contour->p[c];

            if (final_count >= polygon_temp_size) {
                free(result->p);
                return 0;
            }

            /* remove v from remaining polygon */
            for(s=v,t=v+1; t<nv; s++,t++)
                V[s] = V[t];
            nv--;

            /* resest error detection counter */
            count = 2*nv;
        }
    }

    result->count = final_count;

    return 1;
}

// ** Efficient Polygon Triangulation **

/*
 * Destroys SDL/EGL context
 */
static void sdl_egl_destroy(struct graphics_opengl_platform *egl) {
    if (egl->eglwindow) {
        SDL_GL_DeleteContext(egl->eglcontext);
        SDL_DestroyWindow(egl->eglwindow);
    }
    g_free(egl);
    SDL_Quit();
}

/*
 * Swap EGL buffer
 */
static void sdl_egl_swap_buffers(struct graphics_opengl_platform *egl) {
    SDL_GL_SwapWindow(egl->eglwindow);
}

/*
 * Main graphic destroy
 */
static void graphics_destroy(struct graphics_priv *gr) {
    /*FIXME graphics_destroy is never called */
    gr->freetype_methods.destroy();
    g_free(gr);
    gr = NULL;
    sdl_egl_destroy(gr->platform);
    SDL_Quit();
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

static struct graphics_image_priv *image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *name,
        int *w, int *h, struct point *hot, int rotation) {
    struct graphics_image_priv *gi;

    /* FIXME: meth is not used yet.. so gi leaks. at least xpm is small */

    struct graphics_image_priv *curr_elem =
        g_hash_table_lookup(hImageData, name);

    if (curr_elem == &image_error) {
        //found but couldn't be loaded
        return NULL;
    } else if (curr_elem) {
        //found and OK -> use hastable entry
        *w = curr_elem->img->w;
        *h = curr_elem->img->h;
        hot->x = *w / 2;
        hot->y = *h / 2;
        return curr_elem;
    }

    if (strlen(name) < 4) {
        g_hash_table_insert(hImageData, g_strdup(name), &image_error);
        return NULL;
    }

    gi = g_new0(struct graphics_image_priv, 1);
    gi->img = IMG_Load(name);
    if(gi->img) {
        *w=gi->img->w;
        *h=gi->img->h;
        hot->x=*w/2;
        hot->y=*h/2;
    } else {
        /* TODO: debug "colour parse errors" on xpm */
        dbg(lvl_error,"image_new on '%s' failed: %s", name, IMG_GetError());
        g_free(gi);
        gi = NULL;
        g_hash_table_insert(hImageData, g_strdup(name), &image_error);
        return gi;
    }

    g_hash_table_insert(hImageData, g_strdup(name), gi);
    return gi;
}

static void image_free(struct graphics_priv *gr, struct graphics_image_priv * gi) {
//    SDL_FreeSurface(gi->img);
//    g_free(gi);
}

static void set_color(struct graphics_priv *gr, struct graphics_gc_priv *gc) {
    GLfloat col[4];
    col[0]=gc->fr;
    col[1]=gc->fg;
    col[2]=gc->fb;
    col[3]=gc->fa;
    glUniform4fv(gr->color_location, 1, col);
}

static void draw_array(struct graphics_priv *gr, struct point *p, int count, GLenum mode) {
    int i;
    GLf *x;//[count*2];
    x = alloca(sizeof(GLf)*count*2);
    for (i = 0 ; i < count ; i++) {
        x[i*2]=glF(p[i].x);
        x[i*2+1]=glF(p[i].y);
    }

    glVertexAttribPointer (gr->position_location, 2, GL_FLOAT, 0, 0, x );
    glDrawArrays(mode, 0, count);
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


static void draw_image_es(struct graphics_priv *gr, struct point *p, int w, int h, unsigned char *data) {
    GLf x[8];
    GLuint texture;
    memset(x, 0, sizeof(x));

    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    x[0]+=glF(1);
    x[2]+=glF(1);
    x[3]+=glF(1);
    x[7]+=glF(1);
    glUniform1i(gr->use_texture_location, 1);
    glEnableVertexAttribArray(gr->texture_position_location);
    glVertexAttribPointer (gr->texture_position_location, 2, GL_FLOAT, 0, 0, x );

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw_rectangle_do(gr, p, w, h);

    glUniform1i(gr->use_texture_location, 0);
    glDisableVertexAttribArray(gr->texture_position_location);

    glDisable(GL_BLEND);
    glDeleteTextures(1, &texture);
}

inline void get_overlay_pos(struct graphics_priv *gr, struct point *point_out) {
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

inline void draw_overlay(struct graphics_priv *gr) {
    struct point p_eff;
    GLf x[8];

    get_overlay_pos(gr, &p_eff);

    memset(x, 0, 8*sizeof(GLf));
    x[0]+=glF(1);
    x[1]+=glF(1);
    x[2]+=glF(1);
    x[5]+=glF(1);

    glUniform1i(gr->use_texture_location, 1);
    glEnableVertexAttribArray(gr->texture_position_location);
    glVertexAttribPointer (gr->texture_position_location, 2, GL_FLOAT, 0, 0, x);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, gr->overlay_texture);

    draw_rectangle_do(graphics_priv_root, &p_eff, gr->width, gr->height);

    glUniform1i(gr->use_texture_location, 0);
    glDisableVertexAttribArray(gr->texture_position_location);

    glDisable(GL_BLEND);
}

static void draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count) {
    if ((gr->parent && !gr->parent->overlay_enabled)
            || (gr->parent && gr->parent->overlay_enabled
                && !gr->overlay_enabled)) {
        return;
    }

    glLineWidth(gc->linewidth);

    set_color(gr, gc);
    graphics_priv_root->dirty = 1;

    draw_array(gr, p, count, GL_LINE_STRIP);
}


static void draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count) {
    int ok;
    if ((gr->parent && !gr->parent->overlay_enabled)
            || (gr->parent && gr->parent->overlay_enabled
                && !gr->overlay_enabled)) {
        return;
    }
    set_color(gr, gc);
    graphics_priv_root->dirty = 1;
    struct contour contour, result;
    contour.count = count;
    contour.p = p;
    ok = process_triangles(&contour, &result);
    if (ok && gr->fill_poly) {
        draw_array(gr, result.p, result.count, GL_TRIANGLES);
        free(result.p);
    } else {
        draw_array(gr, p, count, GL_LINE_STRIP);
    }
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

static void display_text_draw(struct font_freetype_text *text, struct graphics_priv *gr, struct graphics_gc_priv *fg,
                              struct graphics_gc_priv *bg, int color, struct point *p) {
    int i, x, y, stride;
    struct font_freetype_glyph *g, **gp;
    unsigned char *shadow, *glyph;
    struct color transparent = { 0x0000, 0x0000, 0x0000, 0x0000 };
    struct color black = {
        fg->fr * 65535, fg->fg * 65535, fg->fb * 65535,
        fg->fa * 65535
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

                struct point p;
                p.x=((x + g->x) >> 6)-1;
                p.y=((y + g->y) >> 6)-1;
                draw_image_es(gr, &p, g->w+2, g->h+2, shadow);

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
                    struct point p;
                    p.x=(x + g->x) >> 6;
                    p.y=(y + g->y) >> 6;
                    draw_image_es(gr, &p, g->w, g->h, glyph);

                    g_free(glyph);
                }
                stride *= 4;
                glyph = g_malloc(stride * g->h);
                gr->freetype_methods.get_glyph(g, glyph, stride, &black, &white, &transparent);
                struct point p;
                p.x=(x + g->x) >> 6;
                p.y=(y + g->y) >> 6;
                draw_image_es(gr, &p, g->w, g->h, glyph);

                g_free(glyph);
            }
        }
        x += g->dx;
        y += g->dy;
    }
}

static void draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg,
                      struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy) {
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


static void draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p,
                       struct graphics_image_priv *img) {
    draw_image_es(gr, p, img->img->w, img->img->h, img->img->pixels);
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

/*
 * Draws map in background
 */
static void draw_background(struct graphics_priv *gr) {
    struct point p_eff;
    GLf x[8];

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, gr->width, gr->height);
    //get_overlay_pos(gr, &p_eff);
    p_eff.x = gr->p.x;
    p_eff.y = gr->p.y;

    memset(x, 0, 8*sizeof(GLf));
    x[0]+=glF(1);
    x[1]+=glF(1);
    x[2]+=glF(1);
    x[5]+=glF(1);

    glUniform1i(gr->use_texture_location, 1);
    glEnableVertexAttribArray(gr->texture_position_location);
    glVertexAttribPointer (gr->texture_position_location, 2, GL_FLOAT, 0, 0, x);

    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, gr->overlay_texture);

    draw_rectangle_do(gr, &p_eff, gr->width, gr->height);

    glUniform1i(gr->use_texture_location, 0);
    glDisableVertexAttribArray(gr->texture_position_location);
}

/*
    Drawing method :
    Map and overlays are rendered in an offscreen buffer (See render to texture)
    and are recomposed altogether at draw_mode_end request for root
*/
static void draw_mode(struct graphics_priv *gr, enum draw_mode_num mode) {
    GLfloat matrix[16];
    struct graphics_priv *overlay = NULL;
    int i;

    if (mode == draw_mode_begin) {
        // Should not be necessary...
        // SDL_GL_MakeCurrent(gr->platform->eglwindow, gr->platform->eglcontext);

        if (gr->parent == NULL) {
            // Full redraw, reset drag position
            gr->p.x = 0;
            gr->p.y = 0;
        }

        // Need to setup appropriate projection matrix
        for (i = 0; i < 16 ; i++) {
            matrix[i] = 0.0;
        }

        matrix[0]=2.0 / gr->width;
        matrix[5]=-2.0 / gr->height;
        matrix[10]=1;
        matrix[12]=-1;
        matrix[13]=1;
        matrix[15]=1;
        glUniformMatrix4fv(gr->mvp_location, 1, GL_FALSE, matrix);

        glBindFramebuffer(GL_FRAMEBUFFER, gr->framebuffer_name);
        glViewport(0,0,gr->width,gr->height);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    if (mode == draw_mode_end && gr->parent == NULL) {
        overlay = gr->overlays;
        draw_background(gr);
        while(overlay) {
            if (gr->overlay_enabled)
                draw_overlay(overlay);
            overlay = overlay->next;
        }
        sdl_egl_swap_buffers(gr->platform);
    }

    gr->mode = mode;
}

static int graphics_opengl_fullscreen(struct window *w, int on) {
    return 1;
}

static void graphics_opengl_disable_suspend(struct window *w) {
    // No op
}


static GLuint load_shader(const char *shader_source, GLenum type) {
    GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &shader_source, NULL);
    glCompileShader(shader);

    return shader;
}

static void *get_data(struct graphics_priv *this, const char *type) {
    GLuint vertexShader;
    GLuint fragmentShader;
    struct window* win;
    int i;

    if (!strcmp(type, "gtk_widget")) {
        fprintf(stderr, "Currently GTK gui is not yet supported with EGL graphics driver\n");
        return NULL;
    }

    if(strcmp(type, "window") == 0) {
        SDL_GL_MakeCurrent(this->platform->eglwindow, this->platform->eglcontext);

        glClearColor ( 0, 0, 0, 1);
        glClear ( GL_COLOR_BUFFER_BIT );

        callback_list_call_attr_2(graphics_priv_root->cbl, attr_resize, GINT_TO_POINTER(this->width),
                                  GINT_TO_POINTER(this->height));

        this->program  = glCreateProgram();
        vertexShader   = load_shader(vertex_src, GL_VERTEX_SHADER);
        fragmentShader = load_shader(fragment_src, GL_FRAGMENT_SHADER);

        glAttachShader(this->program, vertexShader);
        glAttachShader(this->program, fragmentShader);
        glLinkProgram(this->program);
        glUseProgram(this->program);

        this->mvp_location          = glGetUniformLocation(this->program, "mvp");
        this->position_location     = glGetAttribLocation(this->program, "position");
        glEnableVertexAttribArray(this->position_location);
        this->texture_position_location = glGetAttribLocation(this->program, "texture_position");
        this->color_location            = glGetUniformLocation(this->program, "avcolor");
        this->texture_location          = glGetUniformLocation(this->program, "texture");
        this->use_texture_location      = glGetUniformLocation(this->program, "use_texture");

        glUniform1i(this->use_texture_location, 0);
        glUniform1i(this->texture_location, 0);

        win=g_new(struct window, 1);
        win->priv=this;
        win->disable_suspend=NULL;
        win->fullscreen = graphics_opengl_fullscreen;
        win->disable_suspend = graphics_opengl_disable_suspend;
        return win;
    }

    return NULL;

}

static void overlay_disable(struct graphics_priv *gr, int disable) {
    gr->overlay_enabled = !disable;
    gr->force_redraw = 1;
    draw_mode(gr, draw_mode_end);
}

// Need more testing
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
            // Reset overlay texture
            glDeleteTextures(1, &gr->overlay_texture);
            glGenTextures(1, &gr->overlay_texture);
            glBindTexture(GL_TEXTURE_2D,  gr->overlay_texture);
            glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, gr->width, gr->height, 0,GL_RGBA, GL_UNSIGNED_BYTE, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gr->overlay_texture, 0);
        }

        callback_list_call_attr_2(gr->cbl, attr_resize, GINT_TO_POINTER(gr->width), GINT_TO_POINTER(gr->height));
    }
}


static struct graphics_priv *overlay_new(struct graphics_priv *gr,
        struct graphics_methods *meth,
        struct point *p, int w, int h,
        int wraparound);

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
    NULL, /* set_attr, */
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

static void create_framebuffer_texture(struct graphics_priv *gr) {
    GLenum status;
    // Prepare a new framebuffer object
    glGenFramebuffers(1, &gr->framebuffer_name);
    glBindFramebuffer(GL_FRAMEBUFFER, gr->framebuffer_name);

    glGenTextures(1, &gr->overlay_texture);
    glBindTexture(GL_TEXTURE_2D,  gr->overlay_texture);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, gr->width, gr->height, 0,GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gr->overlay_texture, 0);

    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        printf("Error creating texture framebuffer for overlay, exiting.\n");
        SDL_Quit();
        exit(1);
    }
}

static struct graphics_priv *overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p,
        int w, int h, int wraparound) {
    struct graphics_priv *this = graphics_opengl_new_helper(meth);

    this->p.x = p->x;
    this->p.y = p->y;

    this->width = w;
    this->height = h;
    this->parent = gr;
    this->overlays = NULL;
    this->fill_poly = 1;

    // Copy shader locations parameters
    this->mvp_location              = graphics_priv_root->mvp_location;
    this->position_location         = graphics_priv_root->position_location;
    this->texture_position_location = graphics_priv_root->texture_position_location;
    this->color_location            = graphics_priv_root->color_location;
    this->texture_location          = graphics_priv_root->texture_location;
    this->use_texture_location      = graphics_priv_root->use_texture_location;

    if ((w == 0) || (h == 0)) {
        this->overlay_autodisabled = 1;
    } else {
        this->overlay_autodisabled = 0;
    }
    this->overlay_enabled = 1;

    this->next = gr->overlays;
    gr->overlays = this;

    create_framebuffer_texture(this);

    return this;
}


static gboolean graphics_sdl_idle(void *data) {
    struct graphics_priv *gr = (struct graphics_priv *)data;
    struct point p;
    SDL_Event ev;
    int ret;
    char key_mod = 0;
    char keybuf[8];
    char keycode;

    // Process SDL events (KEYS + MOUSE)
    while(1) {
        ret = SDL_PollEvent(&ev);

        if(!ret)
            break;

        switch(ev.type) {
        case SDL_MOUSEMOTION: {
            p.x = ev.motion.x;
            p.y = ev.motion.y;
            //gr->force_redraw = 1;
            callback_list_call_attr_1(gr->cbl, attr_motion, (void *)&p);
            break;
        }

        case SDL_KEYDOWN: {
            memset(keybuf, 0, sizeof(keybuf));
            switch(ev.key.keysym.sym) {
            case SDLK_F1:
                graphics_priv_root->fill_poly = !graphics_priv_root->fill_poly;
                break;
            case SDLK_F2:
                graphics_priv_root->show_overlays = !graphics_priv_root->show_overlays;
                break;
            case SDLK_LEFT: {
                keybuf[0] = NAVIT_KEY_LEFT;
                break;
            }
            case SDLK_RIGHT: {
                keybuf[0] = NAVIT_KEY_RIGHT;
                break;
            }
            case SDLK_BACKSPACE: {
                keybuf[0] = NAVIT_KEY_BACKSPACE;
                break;
            }
            case SDLK_RETURN: {
                keybuf[0] = NAVIT_KEY_RETURN;
                break;
            }
            case SDLK_DOWN: {
                keybuf[0] = NAVIT_KEY_DOWN;
                break;
            }
            case SDLK_PAGEUP: {
                keybuf[0] = NAVIT_KEY_ZOOM_OUT;
                break;
            }
            case SDLK_UP: {
                keybuf[0] = NAVIT_KEY_UP;
                break;
            }
            case SDLK_PAGEDOWN: {
                keybuf[0] = NAVIT_KEY_ZOOM_IN;
                break;
            }
            case SDLK_ESCAPE: {
                USERWANTSTOQUIT = 1;
                break;
            }
            default: {
                /* return unicode chars when they can be converted to ascii */
                // Need more work...
                keycode = ev.key.keysym.sym;
                keybuf[0] = keycode <= 127 ? keycode : 0;
                break;
            }
            }
            if (keybuf[0]) {
                callback_list_call_attr_1(gr->cbl, attr_keypress, (void *)keybuf);
            }
            break;
        }
        case SDL_KEYUP: {
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            p.x = ev.button.x;
            p.y = ev.button.y;
            graphics_priv_root->force_redraw = 1;
            callback_list_call_attr_3(gr->cbl, attr_button, GINT_TO_POINTER(1), GINT_TO_POINTER((int)ev.button.button), (void *)&p);
            break;
        }

        case SDL_MOUSEBUTTONUP: {
            p.x = ev.button.x;
            p.y = ev.button.y;
            callback_list_call_attr_3(gr->cbl, attr_button, GINT_TO_POINTER(0), GINT_TO_POINTER((int)ev.button.button), (void *)&p);
            break;
        }

        case SDL_QUIT: {
            break;
        }
        default: {
            break;
        }
        }
    }

    if (USERWANTSTOQUIT) {
        SDL_Quit();
        exit(0);
    }

    return TRUE;
}


static struct graphics_priv *graphics_opengl_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs,
        struct callback_list *cbl) {
    struct attr *attr;
    if (!event_request_system("glib", "graphics_opengl_new"))
        return NULL;

    hImageData = g_hash_table_new(g_str_hash, g_str_equal);
    struct graphics_priv *this = graphics_opengl_new_helper(meth);
    graphics_priv_root = this;

    this->nav = nav;
    this->parent = NULL;
    this->overlay_enabled = 1;
    this->framebuffer_name = 0;
    this->overlays = NULL;
    this->fill_poly = 1;
    this->show_overlays = 1;

    this->width = SCREEN_WIDTH;
    if ((attr = attr_search(attrs, attr_w)))
        this->width = attr->u.num;
    this->height = SCREEN_HEIGHT;
    if ((attr = attr_search(attrs, attr_h)))
        this->height = attr->u.num;
    this->timeout = 100;
    if ((attr = attr_search(attrs, attr_timeout)))
        this->timeout = attr->u.num;
    this->delay = 0;
    if ((attr = attr_search(attrs, attr_delay)))
        this->delay = attr->u.num;
    this->cbl = cbl;

    this->framebuffer_name = 0;

    graphics_priv_root->cbl = cbl;
    graphics_priv_root->width = this->width;
    graphics_priv_root->height = this->height;

    struct graphics_opengl_platform *ret=g_new0(struct graphics_opengl_platform,1);

    // SDL Init
    int sdl_status =  SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS);

    if (sdl_status != 0) {
        fprintf(stderr, "\nUnable to initialize SDL: %i %s\n", sdl_status, SDL_GetError() );
        exit(1);
    }

    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
    // I think it's not necessary to sync vblank, update is quite slow
    SDL_GL_SetSwapInterval(0);

    ret->eglwindow = SDL_CreateWindow(
                         "Navit EGL",                       // window title
                         SDL_WINDOWPOS_UNDEFINED,           // initial x position
                         SDL_WINDOWPOS_UNDEFINED,           // initial y position
                         this->width,                       // width, in pixels
                         this->height,                      // height, in pixels
                         flags
                     );

    if (ret->eglwindow == NULL) {
        fprintf(stderr, "\nUnable to initialize SDL window: %s\n", SDL_GetError() );
        goto error;
    }

    ret->eglcontext = SDL_GL_CreateContext(ret->eglwindow);
    if (ret->eglcontext == NULL) {
        printf("EGL context creation failed\n");
        goto error;
    }

    this->platform = ret;
    create_framebuffer_texture(this);
    g_timeout_add(G_PRIORITY_DEFAULT+10, graphics_sdl_idle, this);
    glDisable(GL_DEPTH_TEST);
    return this;
error:
    SDL_Quit();
    return NULL;
}

void plugin_init(void) {
    plugin_register_category_graphics("egl", graphics_opengl_new);
}

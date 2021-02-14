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
#include <gd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "plugin.h"
#include "callback.h"
#include "window.h"
#include "navit.h"
#include "debug.h"
#include "navit/font/freetype/font_freetype.h"


#ifdef HAVE_SHMEM
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#ifdef HAVE_SOCKET
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

struct shmem_header {
    int flag;
    int w,h,bpp;
};

#ifdef HAVE_XPM
#define NAVIT_GD_XPM_TRANSPARENCY_HACK
#endif

static void emit_callback(struct graphics_priv *priv);
static void image_setup(struct graphics_priv *gr);
static struct shmem_header *shm_next(struct graphics_priv *gr);
static void add_overlays(struct graphics_priv *overlay, gdImagePtr im);

#ifdef NAVIT_GD_XPM_TRANSPARENCY_HACK
#include <X11/xpm.h>


BGD_DECLARE(gdImagePtr) gdImageCreateFromXpm (char *filename) {
    XpmInfo info;
    XpmImage image;
    int i, j, k, number;
    char buf[5];
    gdImagePtr im = 0;
    int *pointer;
    int red = 0, green = 0, blue = 0, alpha = 0;
    int *colors;
    int ret;
    ret = XpmReadFileToXpmImage (filename, &image, &info);
    if (ret != XpmSuccess)
        return 0;

    if (!(im = gdImageCreate (image.width, image.height)))
        return 0;

    number = image.ncolors;
    if (overflow2(sizeof (int), number)) {
        return 0;
    }
    colors = (int *) gdMalloc (sizeof (int) * number);
    if (colors == NULL)
        return (0);
    for (i = 0; i < number; i++) {
        alpha = 0;
        switch (strlen (image.colorTable[i].c_color)) {
        case 4:
            if (!strcasecmp(image.colorTable[i].c_color,"none")) {
                red = 0;
                green = 0;
                blue = 0;
                alpha = 127;
            } else {
                buf[1] = '\0';
                buf[0] = image.colorTable[i].c_color[1];
                red = strtol (buf, NULL, 16);

                buf[0] = image.colorTable[i].c_color[3];
                green = strtol (buf, NULL, 16);

                buf[0] = image.colorTable[i].c_color[5];
                blue = strtol (buf, NULL, 16);
            }
            break;
        case 7:
            buf[2] = '\0';
            buf[0] = image.colorTable[i].c_color[1];
            buf[1] = image.colorTable[i].c_color[2];
            red = strtol (buf, NULL, 16);

            buf[0] = image.colorTable[i].c_color[3];
            buf[1] = image.colorTable[i].c_color[4];
            green = strtol (buf, NULL, 16);

            buf[0] = image.colorTable[i].c_color[5];
            buf[1] = image.colorTable[i].c_color[6];
            blue = strtol (buf, NULL, 16);
            break;
        case 10:
            buf[3] = '\0';
            buf[0] = image.colorTable[i].c_color[1];
            buf[1] = image.colorTable[i].c_color[2];
            buf[2] = image.colorTable[i].c_color[3];
            red = strtol (buf, NULL, 16);
            red /= 64;

            buf[0] = image.colorTable[i].c_color[4];
            buf[1] = image.colorTable[i].c_color[5];
            buf[2] = image.colorTable[i].c_color[6];
            green = strtol (buf, NULL, 16);
            green /= 64;

            buf[0] = image.colorTable[i].c_color[7];
            buf[1] = image.colorTable[i].c_color[8];
            buf[2] = image.colorTable[i].c_color[9];
            blue = strtol (buf, NULL, 16);
            blue /= 64;
            break;
        case 13:
            buf[4] = '\0';
            buf[0] = image.colorTable[i].c_color[1];
            buf[1] = image.colorTable[i].c_color[2];
            buf[2] = image.colorTable[i].c_color[3];
            buf[3] = image.colorTable[i].c_color[4];
            red = strtol (buf, NULL, 16);
            red /= 256;

            buf[0] = image.colorTable[i].c_color[5];
            buf[1] = image.colorTable[i].c_color[6];
            buf[2] = image.colorTable[i].c_color[7];
            buf[3] = image.colorTable[i].c_color[8];
            green = strtol (buf, NULL, 16);
            green /= 256;

            buf[0] = image.colorTable[i].c_color[9];
            buf[1] = image.colorTable[i].c_color[10];
            buf[2] = image.colorTable[i].c_color[11];
            buf[3] = image.colorTable[i].c_color[12];
            blue = strtol (buf, NULL, 16);
            blue /= 256;
            break;
        }


        colors[i] = gdImageColorResolveAlpha(im, red, green, blue, alpha);
        if (colors[i] == -1)
            fprintf (stderr, "ARRRGH\n");
    }

    pointer = (int *) image.data;
    for (i = 0; i < image.height; i++) {
        for (j = 0; j < image.width; j++) {
            k = *pointer++;
            gdImageSetPixel (im, j, i, colors[k]);
        }
    }
    gdFree (colors);
    return (im);
}
#endif


struct graphics_priv {
    gdImagePtr im;
    int w,h,flags,overlay,shmkey,shmsize,shmoffset;
    void *shm;
    struct shmem_header *shm_header;
    struct point p;
    struct callback *cb;
    struct callback_list *cbl;
    struct navit *nav;
    struct graphics_gc_priv *background;
    struct font_freetype_methods freetype_methods;
    struct window window;
    struct graphics_data_image image;
    struct graphics_priv *next,*overlays;
    GList *sockets;
};

struct graphics_gc_priv {
    struct graphics_priv *gr;
    int color;
    int bgcolor;
    int width;
    struct color color2;
    unsigned char *dash_list;
    int dash_count;
    int dash_list_len;
};

struct graphics_image_priv {
    gdImagePtr im;
};


static void graphics_destroy(struct graphics_priv *gr) {
    gr->freetype_methods.destroy();
    g_free(gr);
}

static void gc_destroy(struct graphics_gc_priv *gc) {
    if (gc->color != -1)
        gdImageColorDeallocate(gc->gr->im, gc->color);
    if (gc->bgcolor != -1)
        gdImageColorDeallocate(gc->gr->im, gc->bgcolor);
    g_free(gc->dash_list);
    g_free(gc);
}

static void gc_set_linewidth(struct graphics_gc_priv *gc, int w) {
    gc->width=w;
}

static void gc_set_dashes(struct graphics_gc_priv *gc, int w, int offset, unsigned char *dash_list, int n) {
    int i,count=0;
    g_free(gc->dash_list);
    gc->dash_list=g_new(unsigned char, n);
    for (i = 0 ; i < n ; i++) {
        gc->dash_list[i]=dash_list[i];
        count+=dash_list[i];
    }
    gc->dash_list_len=n;
    gc->dash_count=count;
}

static void gc_set_foreground(struct graphics_gc_priv *gc, struct color *c) {
    gc->color2=*c;
    gc->color=gdImageColorAllocate(gc->gr->im, c->r>>8, c->g>>8, c->b>>8);
}

static void gc_set_background(struct graphics_gc_priv *gc, struct color *c) {
    gc->bgcolor=gdImageColorAllocate(gc->gr->im, c->r>>8, c->g>>8, c->b>>8);
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
    ret->gr=gr;
    ret->width=1;
    ret->color=-1;
    ret->bgcolor=-1;
    *meth=gc_methods;
    return ret;
}


static struct graphics_image_priv *image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *name,
        int *w, int *h, struct point *hot, int rotation) {
    FILE *file;
    struct graphics_image_priv *ret=NULL;
    gdImagePtr im=NULL;
    int len;

    if (! name)
        return NULL;
    len=strlen(name);
    if (len < 4)
        return NULL;
    file=fopen(name,"r");
    if (file) {
        if (!strcmp(name+len-4,".png"))
#ifdef HAVE_GRAPHICS_GD_PNG
            im=gdImageCreateFromPng(file);
#else
            im=NULL;
#endif
        else if (!strcmp(name+len-4,".xpm"))
            im=gdImageCreateFromXpm(name);
        else if (!strcmp(name+len-4,".gif")) {
            im=gdImageCreateFromGif(file);
        }
        fclose(file);
    }
    if (im) {
        ret=g_new0(struct graphics_image_priv, 1);
        ret->im=im;
        *w=im->sx;
        *h=im->sy;
        hot->x=im->sx/2;
        hot->y=im->sy/2;
    }
    return ret;
}

static void draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count) {
    int color[gc->dash_count],cc;
    int i,j,k=0;

    if (gc->dash_count) {
        cc=gc->color;
        for (i = 0 ; i < gc->dash_list_len ; i++) {
            for (j = 0 ; j	< gc->dash_list[i] ; j++) {
                color[k++]=cc;
            }
            if (cc == gdTransparent)
                cc=gc->color;
            else
                cc=gdTransparent;
        }
        gdImageSetStyle(gr->im, color, gc->dash_count);
    }
    gdImageSetThickness(gr->im, gc->width);
    cc=gc->color;
    if (gr->flags & 8) {
        gdImageSetAntiAliased(gr->im, cc);
        cc=gdAntiAliased;
    }
#ifdef GD_NO_IMAGE_OPEN_POLYGON
    for (i = 0 ; i < count-1 ; i++)
        gdImageLine(gr->im, p[i].x, p[i].y, p[i+1].x, p[i+1].y, gc->dash_count ? gdStyled : cc);
#else
    gdImageOpenPolygon(gr->im, (gdPointPtr) p, count, gc->dash_count ? gdStyled : cc);
#endif
}

static void draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count) {
    int cc=gc->color;
    if (gr->flags & 8) {
        gdImageSetAntiAliased(gr->im, cc);
        cc=gdAntiAliased;
    }
    gdImageFilledPolygon(gr->im, (gdPointPtr) p, count, cc);
}

static void draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h) {
    int cc=gc->color;
    if (gr->flags & 8) {
        gdImageSetAntiAliased(gr->im, cc);
        cc=gdAntiAliased;
    }
    gdImageFilledRectangle(gr->im, p->x, p->y, p->x+w, p->y+h, cc);
}

static void draw_circle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int r) {
    int cc=gc->color;
    if (gr->flags & 8) {
        gdImageSetAntiAliased(gr->im, cc);
        cc=gdAntiAliased;
    }
    gdImageSetThickness(gr->im, gc->width);
    gdImageArc(gr->im, p->x, p->y, r, r, 0, 360, cc);
}


static void draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg,
                      struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy) {
    struct font_freetype_text *t;
    struct font_freetype_glyph *g, **gp;
    gdImagePtr im;
    int i,x,y,w,h;
    t=gr->freetype_methods.text_new(text, (struct font_freetype_font *)font, dx, dy);
    struct color fgc,bgc,transparent = {0x0, 0x0, 0x0, 0x7f7f};
    fgc=fg->color2;
    fgc.a=0;

    if (bg) {
        bgc=bg->color2;
        bgc.a=0;
        x=p->x << 6;
        y=p->y << 6;
        gp=t->glyph;
        i=t->glyph_count;
        while (i-- > 0) {
            g=*gp++;
            w=g->w;
            h=g->h;
            if (w && h) {
                im=gdImageCreateTrueColor(w+2, h+2);
                gr->freetype_methods.get_shadow(g,(unsigned char *)(im->tpixels),0,&bgc,&transparent);
                gdImageCopy(gr->im, im, ((x+g->x)>>6)-1, ((y+g->y)>>6)-1, 0, 0, w+2, h+2);
                gdImageDestroy(im);
            }
            x+=g->dx;
            y+=g->dy;
        }
    } else
        bgc=transparent;
    x=p->x << 6;
    y=p->y << 6;
    gp=t->glyph;
    i=t->glyph_count;
    while (i-- > 0) {
        g=*gp++;
        w=g->w;
        h=g->h;
        if (w && h) {
            im=gdImageCreateTrueColor(w, h);
            gr->freetype_methods.get_glyph(g,(unsigned char *)(im->tpixels),0,&fgc,&bgc,&transparent);
            gdImageCopy(gr->im, im, (x+g->x)>>6, (y+g->y)>>6, 0, 0, w, h);
            gdImageDestroy(im);
        }
        x+=g->dx;
        y+=g->dy;
    }
    gr->freetype_methods.text_destroy(t);
}

static void draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p,
                       struct graphics_image_priv *img) {
    gdImageCopy(gr->im, img->im, p->x, p->y, 0, 0, img->im->sx, img->im->sy);
}

static void draw_drag(struct graphics_priv *gr, struct point *p) {
    if (p)
        gr->p=*p;
    else {
        gr->p.x=0;
        gr->p.y=0;
    }
}


static void background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc) {
    gr->background=gc;
}

static void draw_mode(struct graphics_priv *gr, enum draw_mode_num mode) {
    FILE *pngout;
#if 0
    if (mode == draw_mode_begin && gr->background) {
        gdImageFilledRectangle(gr->im, 0, 0, gr->w, gr->h, gr->background->color);
    }
#endif
    if (mode == draw_mode_end && !gr->overlay) {
        add_overlays(gr->overlays, gr->im);
        if (!(gr->flags & 1)) {
#ifdef HAVE_GRAPHICS_GD_PNG
            rename("test.png","test.png.old");
            pngout=fopen("test.png", "wb");
            gdImagePng(gr->im, pngout);
            fclose(pngout);
#else
            rename("test.gd","test.gd.old");
            pngout=fopen("test.gd", "wb");
            gdImageGd(gr->im, pngout);
            fclose(pngout);
#endif
        }
        if (gr->flags & 2) {
            struct shmem_header *next=shm_next(gr);
            gr->shm_header->flag=1;
            dbg(lvl_debug,"next flag is %d",next->flag);
            if (!next->flag) {
                gr->shm_header=next;
                image_setup(gr);
            }
        }
#ifdef HAVE_GRAPHICS_GD_PNG
#ifdef HAVE_SOCKET
        if (gr->flags & 4) {
            int size, size_written;
            void *data=gdImagePngPtr(gr->im, &size);
            GList *sockets=gr->sockets;
            while (sockets) {
                int fd=socket(PF_INET, SOCK_STREAM, 0);
                if (fd < 0) {
                    dbg(lvl_error,"socket failed");
                } else  {
                    if (connect(fd, (struct sockaddr *)sockets->data, sizeof(struct sockaddr_in)) < 0) {
                        dbg(lvl_error,"connect failed");
                    } else {
                        size_written=write(fd, data, size);
                        dbg(lvl_debug,"size %d vs %d",size, size_written);
                        if (shutdown(fd, SHUT_RDWR) < 0)
                            dbg(lvl_error,"shutdown failed");
                    }
                    close(fd);
                }
                sockets=g_list_next(sockets);
            }
            gdFree(data);
        }
#endif
#endif
    }
}

static struct graphics_priv * overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p,
        int w, int h, int wraparound);

static void add_overlays(struct graphics_priv *overlay, gdImagePtr im) {
    while (overlay) {
        if (overlay->background) {
            gdImagePtr res,src;
            int y,x;
            int bgcol=overlay->background->color;
            res=gdImageCreateTrueColor(overlay->w,overlay->h);
            src=gdImageCreateTrueColor(overlay->w,overlay->h);
            gdImageCopy(src, im, 0, 0, overlay->p.x, overlay->p.y, overlay->w, overlay->h);
            for (y = 0 ; y < overlay->h ; y++) {
                unsigned int *res_line=res->tpixels[y];
                unsigned int *src_line=src->tpixels[y];
                unsigned int *overlay_line=overlay->im->tpixels[y];
                for (x = 0 ; x < overlay->w ; x++) {
                    if (overlay_line[x] != bgcol)
                        res_line[x]=overlay_line[x];
                    else
                        res_line[x]=src_line[x];
                }
            }
            gdImageCopy(im, res, overlay->p.x, overlay->p.y, 0, 0, overlay->w, overlay->h);
            gdImageDestroy(res);
            gdImageDestroy(src);
        } else
            gdImageCopy(im, overlay->im, overlay->p.x, overlay->p.y, 0, 0, overlay->w, overlay->h);
        overlay=overlay->next;
    }
}

static void *get_data(struct graphics_priv *this, char *type) {
    int b;
    struct point p;
    gdImagePtr im = this->im;
    dbg(lvl_debug,"type=%s",type);
    if (!strcmp(type,"window"))
        return &this->window;
    if (!strcmp(type,"image_png")) {
        if (this->overlays) {
            struct graphics_priv *overlay=this->overlays;
            im=gdImageCreateTrueColor(this->w,this->h);
            gdImageCopy(im, this->im, 0, 0, 0, 0, this->w, this->h);
            add_overlays(overlay, im);
        }
        if (this->image.data)
            gdFree(this->image.data);
#ifdef HAVE_GRAPHICS_GD_PNG
        this->image.data=gdImagePngPtr(im, &this->image.size);
#else
        this->image.data=NULL;
#endif
        if (this->overlays)
            gdImageDestroy(im);
        return &this->image;
    }
    if (sscanf(type,"click_%d_%d_%d",&p.x,&p.y,&b) == 3) {
        dbg(lvl_debug,"click %d %d %d",p.x,p.y,b);
        callback_list_call_attr_3(this->cbl, attr_button, (void *)b, (void *)1, (void *)&p);
    }
    if (sscanf(type,"move_%d_%d",&p.x,&p.y) == 2) {
        dbg(lvl_debug,"move %d %d",p.x,p.y);
        callback_list_call_attr_1(this->cbl, attr_motion, (void *)&p);
    }
    return NULL;
}


static void image_free(struct graphics_priv *gr, struct graphics_image_priv *priv) {
    gdImageDestroy(priv->im);
    g_free(priv);
}

static void overlay_disable(struct graphics_priv *gr, int disable) {
    dbg(lvl_debug,"enter");
}

static void overlay_resize(struct graphics_priv *gr, struct point *p, int w, int h, int wraparound) {
    dbg(lvl_debug,"enter");
}

struct shmem_header *
shm_next(struct graphics_priv *gr) {
    char *next=(char *)gr->shm_header+gr->shmoffset;
    if (next+gr->shmoffset > (char *)gr->shm+gr->shmsize) {
        dbg(lvl_debug,"wraparound");
        return gr->shm;
    }
    dbg(lvl_debug,"next 0x%x (offset 0x%x)",next-(char *)gr->shm,gr->shmoffset);
    return (struct shmem_header *)next;

}
static void image_setup(struct graphics_priv *gr) {
    int i,*shm=(int *)(gr->shm_header+1);
    if (!gr->shmkey)
        return;
    for (i = 0 ; i < gr->h ; i++)
        gr->im->tpixels[i]=shm+gr->w*i;
    gr->shm_header->w=gr->w;
    gr->shm_header->h=gr->h;
    gr->shm_header->bpp=32;
    gr->shm_header->flag=0;
}

static void image_create(struct graphics_priv *gr) {
    dbg(lvl_debug,"shmkey %d",gr->shmkey);
#ifdef HAVE_SHMEM
    if (gr->shmkey) {
        int size=gr->h*gr->w*sizeof(int);
        int shmid=shmget(gr->shmkey, size, 0666);
        dbg(lvl_debug,"shmid for key 0x%x is 0x%x",gr->shmkey, shmid);
        if (shmid < 0)
            dbg(lvl_debug,"shmget");
        gr->shm=shmat(shmid, NULL, 0);
        if (!gr->shm)
            dbg(lvl_debug,"shmat");
        gr->shm_header=gr->shm;
        gr->im=g_new0(gdImage,1);
        gr->im->tpixels=g_new(int *,gr->h);
        gr->im->sx=gr->w;
        gr->im->sy=gr->h;
        gr->im->transparent=-1;
        gr->im->trueColor=1;
        gr->im->alphaBlendingFlag=1;
        gr->im->thick=1;
        gr->im->cx2=gr->im->sx-1;
        gr->im->cy2=gr->im->sy-1;
        image_setup(gr);
    } else
#endif
        gr->im=gdImageCreateTrueColor(gr->w,gr->h);
}

static void image_destroy(struct graphics_priv *gr) {
#ifdef HAVE_SHMEM
    if (gr->shmkey) {
        shmdt(gr->shm);
        g_free(gr->im->tpixels);
        g_free(gr->im);
    } else
#endif
        gdImageDestroy(gr->im);
    gr->im=NULL;
}

static int set_attr_do(struct graphics_priv *gr, struct attr *attr, int init) {
    char *s,*c,*n,*p;
    switch (attr->type) {
    case attr_w:
        if (gr->w != attr->u.num) {
            gr->w=attr->u.num;
            if (!init) {
                if (gr->im)
                    image_destroy(gr);
                image_create(gr);
                emit_callback(gr);
            }
        }
        break;
    case attr_h:
        if (gr->h != attr->u.num) {
            gr->h=attr->u.num;
            if (!init) {
                if (gr->im)
                    image_destroy(gr);
                image_create(gr);
                emit_callback(gr);
            }
        }
        break;
    case attr_flags:
        gr->flags=attr->u.num;
        break;
    case attr_shmkey:
        gr->shmkey=attr->u.num;
        break;
    case attr_shmsize:
        gr->shmsize=attr->u.num;
        break;
    case attr_shmoffset:
        gr->shmoffset=attr->u.num;
        break;
#ifdef HAVE_SOCKET
    case attr_socket:
        g_list_foreach(gr->sockets, (GFunc)g_free, NULL);
        g_list_free(gr->sockets);
        gr->sockets=NULL;
        c=s=g_strdup(attr->u.str);
        dbg(lvl_debug,"s=%s",s);
        while (c) {
            n=strchr(c,',');
            if (n)
                *n++='\0';
            p=strchr(c,':');
            if (p) {
                *p++='\0';
                struct sockaddr_in *sin=g_new(struct sockaddr_in, 1);
                sin->sin_family=AF_INET;
                sin->sin_port=ntohs(atoi(p));
                if (inet_aton(c, &sin->sin_addr)) {
                    gr->sockets=g_list_append(gr->sockets, sin);
                } else {
                    dbg(lvl_error,"error in %s",c);
                    g_free(sin);
                }
                dbg(lvl_debug,"host=%s port=%s",c,p);
            } else
                dbg(lvl_error,"error in format: %s",p);
            c=n;
        }
        g_free(s);
#endif
    default:
        return 0;
    }
    return 1;
}


static int set_attr(struct graphics_priv *gr, struct attr *attr) {
    return set_attr_do(gr, attr, 0);
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
    set_attr,
    NULL, /* show_native_keyboard */
    NULL, /* hide_native_keyboard */
};

static struct graphics_priv *overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p,
        int w, int h, int wraparound) {
    struct font_priv * (*font_freetype_new)(void *meth);
    struct graphics_priv *ret;

    dbg(lvl_debug,"enter");
    ret=g_new0(struct graphics_priv, 1);
    *meth=graphics_methods;
    font_freetype_new=plugin_get_category_font("freetype");
    if (!font_freetype_new)
        return NULL;
    font_freetype_new(&ret->freetype_methods);
    ret->p=*p;
    ret->w=w;
    ret->h=h;
    ret->overlay=1;
    ret->flags=1;
    ret->im=gdImageCreateTrueColor(ret->w,ret->h);
    ret->next=gr->overlays;
    gr->overlays=ret;

    return ret;
}

static void emit_callback(struct graphics_priv *priv) {
    callback_list_call_attr_2(priv->cbl, attr_resize, (void *)priv->w, (void *)priv->h);
}


static struct graphics_priv *graphics_gd_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs,
        struct callback_list *cbl) {
    struct font_priv * (*font_freetype_new)(void *meth);
    struct graphics_priv *ret;
    event_request_system("glib","graphics_gd_new");
    font_freetype_new=plugin_get_category_font("freetype");
    if (!font_freetype_new)
        return NULL;
    *meth=graphics_methods;
    ret=g_new0(struct graphics_priv, 1);
    font_freetype_new(&ret->freetype_methods);
    meth->font_new=(struct graphics_font_priv *(*)(struct graphics_priv *, struct graphics_font_methods *, char *,  int,
                    int))ret->freetype_methods.font_new;
    meth->get_text_bbox=ret->freetype_methods.get_text_bbox;
    ret->cb=callback_new_attr_1(callback_cast(emit_callback), attr_navit, ret);
    navit_add_callback(nav, ret->cb);
    ret->cbl=cbl;
    ret->nav=nav;
    ret->w=800;
    ret->h=600;
    while (*attrs) {
        set_attr_do(ret, *attrs, 1);
        attrs++;
    }
    if (!ret->im)
        image_create(ret);
    return ret;
}

void plugin_init(void) {
    plugin_register_category_graphics("gd", graphics_gd_new);
}

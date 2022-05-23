#include <glib.h>
#include "color.h"
#include "coord.h"
#include "point.h"
#include "callback.h"
#include "graphics.h"
#include "debug.h"
#include "navit_nls.h"
#include "gui_internal.h"
#include "gui_internal_widget.h"
#include "gui_internal_priv.h"
#include "gui_internal_menu.h"

static void gui_internal_scroll_buttons_init(struct gui_priv *this, struct widget *widget, struct scroll_buttons *sb);
void gui_internal_box_pack(struct gui_priv *this, struct widget *w);

/**
 * @brief Transfer the content of one widget into another one (and optionally vice-versa)
 *
 * @param this The internal GUI instance
 * @param[in,out] first The first widget
 * @param[in,out] second The second widget
 * @param move_only If non nul, transfer only the second widget into the first widget. The second widget is then deallocated and should be be used anymore (this is a move operation)
 */
static void gui_internal_widget_transfer_content(struct gui_priv *this, struct widget *first, struct widget *second,
        int move_only) {
    struct widget *temp;

    if (!first) {
        dbg(lvl_error, "Refusing copy: first argument is NULL");
        return;
    }
    if (!second) {
        dbg(lvl_error, "Refusing copy: second argument is NULL");
        return;
    }
    temp=g_new0(struct widget, 1);
    memcpy(temp, first, sizeof(struct widget));
    memcpy(first, second, sizeof(struct widget));
    if (move_only) {
        gui_internal_widget_destroy(this, temp);
        /* gui_internal_widget_destroy() will do a g_free(temp), but will also deallocate all children widgets */
        /* Now, we also free the struct widget pointed to by second, so variable second should not be used anymore from this point */
        g_free(second);
    } else {
        memcpy(second, temp, sizeof(struct widget));
        g_free(temp);
    }
}

/**
 * @brief Swap two widgets
 *
 * @param this The internal GUI instance
 * @param[in,out] first The first widget
 * @param[in,out] second The second widget
 */
void gui_internal_widget_swap(struct gui_priv *this, struct widget *first, struct widget *second) {
    gui_internal_widget_transfer_content(this, first, second, 0);
}

/**
 * @brief Move the content of one widget into another one (the @p src widget is then deallocated and should be be used anymore)
 *
 * @param this The internal GUI instance
 * @param[in,out] dst The destination widget
 * @param[in,out] src The source widget
 */
void gui_internal_widget_move(struct gui_priv *this, struct widget *dst, struct widget *src) {
    gui_internal_widget_transfer_content(this, dst, src, 1);
}

static void gui_internal_background_render(struct gui_priv *this, struct widget *w) {
    struct point pnt=w->p;
    if (w->state & STATE_HIGHLIGHTED)
        graphics_draw_rectangle(this->gra, this->highlight_background, &pnt, w->w, w->h);
    else {
        if (w->background)
            graphics_draw_rectangle(this->gra, w->background, &pnt, w->w, w->h);
    }
}

struct widget *
gui_internal_label_font_new(struct gui_priv *this, const char *text, int font) {
    struct point p[4];
    int w=0;
    int h=0;

    struct widget *widget=g_new0(struct widget, 1);
    widget->type=widget_label;
    widget->font_idx=font;
    if (text) {
        widget->text=g_strdup(text);
        graphics_get_text_bbox(this->gra, this->fonts[font], widget->text, 0x10000, 0x0, p, 0);
        w=p[2].x-p[0].x;
        h=p[0].y-p[2].y;
    }
    widget->h=h+this->spacing;
    widget->texth=h;
    widget->w=w+this->spacing;
    widget->textw=w;
    widget->flags=gravity_center;
    widget->foreground=this->text_foreground;
    widget->text_background=this->text_background;

    return widget;
}

struct widget *
gui_internal_label_new(struct gui_priv *this, const char *text) {
    return gui_internal_label_font_new(this, text, 0);
}

struct widget *
gui_internal_label_new_abbrev(struct gui_priv *this, const char *text, int maxwidth) {
    struct widget *ret=NULL;
    char *tmp=g_malloc(strlen(text)+3);
    const char *p;
    p=text+strlen(text);
    while ((p=g_utf8_find_prev_char(text, p)) >= text) {
        int i=p-text;
        strcpy(tmp, text);
        strcpy(tmp+i,"..");
        ret=gui_internal_label_new(this, tmp);
        if (ret->w < maxwidth)
            break;
        gui_internal_widget_destroy(this, ret);
        ret=NULL;
    }
    if(!ret)
        ret=gui_internal_label_new(this, "");
    g_free(tmp);
    return ret;
}

struct widget *
gui_internal_image_new(struct gui_priv *this, struct graphics_image *image) {
    struct widget *widget=g_new0(struct widget, 1);
    widget->type=widget_image;
    widget->img=image;
    if (image) {
        widget->w=image->width;
        widget->h=image->height;
    }
    return widget;
}

/**
 * @brief Renders an image or icon, preparing it for drawing on the display
 *
 * @param this The internal GUI instance
 * @param w The widget to render
 */
static void gui_internal_image_render(struct gui_priv *this, struct widget *w) {
    struct point pnt;

    gui_internal_background_render(this, w);
    if (w->img) {
        pnt=w->p;
        pnt.x+=w->w/2-w->img->hot.x;
        pnt.y+=w->h/2-w->img->hot.y;
        graphics_draw_image(this->gra, this->foreground, &pnt, w->img);
    }
}

/**
 * @brief Renders a label, preparing it for drawing on the display
 *
 * @param this The internal GUI instance
 * @param w The widget to render
 */
static void gui_internal_label_render(struct gui_priv *this, struct widget *w) {
    struct point pnt=w->p;
    gui_internal_background_render(this, w);
    if (w->state & STATE_EDIT)
        graphics_draw_rectangle(this->gra, this->highlight_background, &pnt, w->w, w->h);
    if (w->text) {
        char *text;
        char *startext=(char*)g_alloca(strlen(w->text)+1);
        text=w->text;
        if (w->flags2 & 1) {
            int i;
            for (i = 0 ; i < strlen(text); i++)
                startext[i]='*';
            startext[i]='\0';
            text=startext;
        }
        if (w->flags & gravity_right) {
            pnt.y+=w->h-this->spacing;
            pnt.x+=w->w-w->textw-this->spacing;
            graphics_draw_text(this->gra, w->foreground, w->text_background, this->fonts[w->font_idx], text, &pnt, 0x10000, 0x0);
        } else {
            pnt.y+=w->h-this->spacing;
            graphics_draw_text(this->gra, w->foreground, w->text_background, this->fonts[w->font_idx], text, &pnt, 0x10000, 0x0);
        }
    }
}

/**
 * @brief Creates a text box.
 *
 * A text box is a widget that renders a text string containing newlines.
 * The string will be broken up into label widgets at each newline with a vertical layout.
 *
 * @param this The internal GUI instance
 * @param text The text to be displayed in the text box
 * @param font The font to use for the text
 * @param flags
 */
struct widget *
gui_internal_text_font_new(struct gui_priv *this, const char *text, int font, enum flags flags) {
    char *s=g_strdup(text),*s2,*tok;
    struct widget *ret=gui_internal_box_new(this, flags);
    s2=s;
    while ((tok=strtok(s2,"\n"))) {
        gui_internal_widget_append(ret, gui_internal_label_font_new(this, tok, font));
        s2=NULL;
    }
    gui_internal_widget_pack(this,ret);
    g_free(s);
    return ret;
}

struct widget *
gui_internal_text_new(struct gui_priv *this, const char *text, enum flags flags) {
    return gui_internal_text_font_new(this, text, 0, flags);
}


struct widget *
gui_internal_button_font_new_with_callback(struct gui_priv *this, const char *text, int font,
        struct graphics_image *image, enum flags flags, void(*func)(struct gui_priv *priv, struct widget *widget, void *data),
        void *data) {
    struct widget *ret=NULL;
    ret=gui_internal_box_new(this, flags);
    if (ret) {
        if (image && !(flags & flags_swap))
            gui_internal_widget_append(ret, gui_internal_image_new(this, image));
        if (text)
            gui_internal_widget_append(ret, gui_internal_text_font_new(this, text, font, gravity_center|orientation_vertical));
        if (image && (flags & flags_swap))
            gui_internal_widget_append(ret, gui_internal_image_new(this, image));
        ret->func=func;
        ret->data=data;
        if (func) {
            ret->state |= STATE_SENSITIVE;
            ret->speech=g_strdup(text);
        }
    }
    return ret;

}

struct widget *
gui_internal_button_new_with_callback(struct gui_priv *this, const char *text, struct graphics_image *image,
                                      enum flags flags, void(*func)(struct gui_priv *priv, struct widget *widget, void *data), void *data) {
    return gui_internal_button_font_new_with_callback(this, text, 0, image, flags, func, data);
}

struct widget *
gui_internal_button_new(struct gui_priv *this, const char *text, struct graphics_image *image, enum flags flags) {
    return gui_internal_button_new_with_callback(this, text, image, flags, NULL, NULL);
}

struct widget *
gui_internal_find_widget(struct widget *wi, struct point *p, int flags) {
    struct widget *ret,*child;
    GList *l=wi->children;

    if (p) {
        if (wi->p.x > p->x )
            return NULL;
        if (wi->p.y > p->y )
            return NULL;
        if ( wi->p.x + wi->w < p->x)
            return NULL;
        if ( wi->p.y + wi->h < p->y)
            return NULL;
    }
    if (wi->state & flags)
        return wi;
    while (l) {
        child=l->data;
        ret=gui_internal_find_widget(child, p, flags);
        if (ret) {
            return ret;
        }
        l=g_list_next(l);
    }
    return NULL;

}

void gui_internal_highlight_do(struct gui_priv *this, struct widget *found) {
    if (found == this->highlighted)
        return;

    graphics_draw_mode(this->gra, draw_mode_begin);
    if (this->highlighted) {
        this->highlighted->state &= ~STATE_HIGHLIGHTED;
        if (this->root.children && this->highlighted_menu == g_list_last(this->root.children)->data)
            gui_internal_widget_render(this, this->highlighted);
        this->highlighted=NULL;
        this->highlighted_menu=NULL;
    }
    if (found) {
        this->highlighted=found;
        this->highlighted_menu=g_list_last(this->root.children)->data;
        this->highlighted->state |= STATE_HIGHLIGHTED;
        gui_internal_widget_render(this, this->highlighted);
        dbg(lvl_debug,"%d,%d %dx%d", found->p.x, found->p.y, found->w, found->h);
    }
    graphics_draw_mode(this->gra, draw_mode_end);
}
//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
void gui_internal_highlight(struct gui_priv *this) {
    struct widget *menu,*found=NULL;
    if (this->current.x > -1 && this->current.y > -1) {
        menu=g_list_last(this->root.children)->data;
        found=gui_internal_find_widget(menu, &this->current, STATE_SENSITIVE);
        if (!found) {
            found=gui_internal_find_widget(menu, &this->current, STATE_EDITABLE);
            if (found) {
                if (this->editable && this->editable != found) {
                    this->editable->state &= ~ STATE_EDIT;
                    gui_internal_widget_render(this, this->editable);
                }
                found->state |= STATE_EDIT;
                gui_internal_widget_render(this, found);
                this->editable=found;
                found=NULL;
            }
        }
    }
    gui_internal_highlight_do(this, found);
    this->motion_timeout_event=NULL;
}


struct widget *
gui_internal_box_new_with_label(struct gui_priv *this, enum flags flags, const char *label) {
    struct widget *widget=g_new0(struct widget, 1);

    if (label)
        widget->text=g_strdup(label);
    widget->type=widget_box;
    widget->flags=flags;
    widget->on_resize=gui_internal_box_resize;
    return widget;
}

struct widget *
gui_internal_box_new(struct gui_priv *this, enum flags flags) {
    return gui_internal_box_new_with_label(this, flags, NULL);
}

/**
 * @brief Renders a box widget, preparing it for drawing on the display
 *
 * @param this The internal GUI instance
 * @param w The box widget to render
 */
static void gui_internal_box_render(struct gui_priv *this, struct widget *w) {
    struct widget *wc;
    GList *l;

    int visual_debug = 0;

#if defined(GUI_INTERNAL_VISUAL_DBG)
    static struct graphics_gc *debug_gc=NULL;
    static struct color gui_box_debug_color= {0xffff,0x0400,0x0400,0xffff}; /* Red */
    visual_debug = (debug_level_get("gui_internal_visual_layout") >= lvl_debug);
#endif

    if (visual_debug)
        dbg(lvl_debug, "Internal layout visual debugging is enabled");

#if defined(GUI_INTERNAL_VISUAL_DBG)
    if (visual_debug && !debug_gc) {
        debug_gc = graphics_gc_new(this->gra);
        graphics_gc_set_foreground(debug_gc, &gui_box_debug_color);
        graphics_gc_set_linewidth(debug_gc, 1);
    }
#endif

    gui_internal_background_render(this, w);
    if ((w->foreground && w->border) || visual_debug) {
        struct point pnt[5];
        pnt[0]=w->p;
        pnt[1].x=pnt[0].x+w->w;
        pnt[1].y=pnt[0].y;
        pnt[2].x=pnt[0].x+w->w;
        pnt[2].y=pnt[0].y+w->h;
        pnt[3].x=pnt[0].x;
        pnt[3].y=pnt[0].y+w->h;
        pnt[4]=pnt[0];
        if (!visual_debug) {
            graphics_gc_set_linewidth(w->foreground, w->border ? w->border : 1);
            graphics_draw_lines(this->gra, w->foreground, pnt, 5);
            graphics_gc_set_linewidth(w->foreground, 1);
        }
#if defined(GUI_INTERNAL_VISUAL_DBG)
        else
            graphics_draw_lines(this->gra, debug_gc, pnt, 5);	/* Force highlighting box borders in debug more */
#endif
    }

    l=w->children;
    while (l) {
        wc=l->data;
        gui_internal_widget_render(this, wc);
        l=g_list_next(l);
    }
    if (w->scroll_buttons)
        gui_internal_widget_render(this, w->scroll_buttons->button_box);
}

/**
 * @brief Computes the size and location for a box widget
 *
 * @param this The internal GUI instance
 * @param w The widget to pack
 */
void gui_internal_box_pack(struct gui_priv *this, struct widget *w) {
    struct widget *wc;
    int x0,x=0,y=0,width=0,height=0,owidth=0,oheight=0,expand=0,expandd=1,count=0,rows=0,cols=w->cols ? w->cols : 0;
    int hb=w->scroll_buttons?w->scroll_buttons->button_box->h:0;
    GList *l;
    int orientation=w->flags & 0xffff0000;

    if (!cols)
        cols=this->cols;
    if (!cols) {
        if ( this->root.w > this->root.h )
            cols=3;
        else
            cols=2;
        width=0;
        height=0;
    }


    /*
     * count the number of children
     */
    l=w->children;
    while (l) {
        count++;
        l=g_list_next(l);
    }
    if (orientation == orientation_horizontal_vertical && count <= cols)
        orientation=orientation_horizontal;
    switch (orientation) {
    case orientation_horizontal:
        /*
         * For horizontal orientation:
         * pack each child and find the largest height and
         * compute the total width. x spacing (spx) is considered
         *
         * If any children want to be expanded
         * we keep track of this
         */
        l=w->children;
        while (l) {
            wc=l->data;
            gui_internal_widget_pack(this, wc);
            if (height < wc->h)
                height=wc->h;
            width+=wc->w;
            if (wc->flags & flags_expand)
                expand+=wc->w ? wc->w : 1;
            l=g_list_next(l);
            if (l)
                width+=w->spx;
        }
        owidth=width;
        if (expand && w->w) {
            expandd=w->w-width+expand;
            owidth=w->w;
        } else
            expandd=expand=1;
        break;
    case orientation_vertical:
        /*
         * For vertical layouts:
         * We pack each child and compute the largest width and
         * the total height.  y spacing (spy) is considered
         *
         * If the expand flag is set then teh expansion amount
         * is computed.
         */
        l=w->children;
        while (l) {
            wc=l->data;
            gui_internal_widget_pack(this, wc);
            if (width < wc->w)
                width=wc->w;
            height+=wc->h;
            if (wc->flags & flags_expand)
                expand+=wc->h ? wc->h : 1;
            l=g_list_next(l);
            if (l)
                height+=w->spy;
        }
        oheight=height;
        if (expand && w->h) {
            expandd=w->h-hb-height+expand;
            oheight=w->h-hb;
        } else
            expandd=expand=1;
        break;
    case orientation_horizontal_vertical:
        /*
         * For horizontal_vertical orientation
         * pack the children.
         * Compute the largest height and largest width.
         * Layout the widgets based on having the
         * number of columns specified by (cols)
         */
        count=0;
        l=w->children;
        while (l) {
            wc=l->data;
            gui_internal_widget_pack(this, wc);
            if (height < wc->h)
                height=wc->h;
            if (width < wc->w)
                width=wc->w;
            l=g_list_next(l);
            count++;
        }
        if (count < cols)
            cols=count;
        rows=(count+cols-1)/cols;
        width*=cols;
        height*=rows;
        width+=w->spx*(cols-1);
        height+=w->spy*(rows-1);
        owidth=width;
        oheight=height;
        expandd=expand=1;
        break;
    default:
        /*
         * No orientation was specified.
         * width and height are both 0.
         * The width & height of this widget
         * will be used.
         */
        if(!w->w && !w->h)
            dbg(lvl_error,"Warning width and height of a widget are 0");
        break;
    }
    if (! w->w && ! w->h) {
        w->w=w->bl+w->br+width;
        w->h=w->bt+w->bb+height+hb;
        w->packed=1;
    }
#if 0
    if (expand < 100)
        expand=100;
#endif

    /*
     * At this stage the width and height of this
     * widget has been computed.
     * We now make a second pass assigning heights,
     * widths and coordinates to each child widget.
     */

    if (w->flags & gravity_left)
        x=w->p.x+w->bl;
    if (w->flags & gravity_xcenter)
        x=w->p.x+w->w/2-owidth/2;
    if (w->flags & gravity_right)
        x=w->p.x+w->w-w->br-owidth;
    if (w->flags & gravity_top)
        y=w->p.y+w->bt;
    if (w->flags & gravity_ycenter)
        y=w->p.y+(w->h-hb)/2-oheight/2;
    if (w->flags & gravity_bottom)
        y=w->p.y+(w->h-hb)-w->bb-oheight;
    l=w->children;
    switch (orientation) {
    case orientation_horizontal:
        l=w->children;
        while (l) {
            wc=l->data;
            wc->p.x=x;
            if (wc->flags & flags_fill)
                wc->h=w->h-hb;
            if (wc->flags & flags_expand) {
                if (! wc->w)
                    wc->w=1;
                wc->w=wc->w*expandd/expand;
            }
            if (w->flags & gravity_top)
                wc->p.y=y;
            if (w->flags & gravity_ycenter)
                wc->p.y=y-wc->h/2;
            if (w->flags & gravity_bottom)
                wc->p.y=y-wc->h;
            x+=wc->w+w->spx;
            l=g_list_next(l);
        }
        break;
    case orientation_vertical:
        l=w->children;
        while (l) {
            wc=l->data;
            wc->p.y=y;
            if (wc->flags & flags_fill)
                wc->w=w->w;
            if (wc->flags & flags_expand) {
                if (! wc->h)
                    wc->h=1;
                wc->h=wc->h*expandd/expand;
            }
            if (w->flags & gravity_left)
                wc->p.x=x;
            if (w->flags & gravity_xcenter)
                wc->p.x=x-wc->w/2;
            if (w->flags & gravity_right)
                wc->p.x=x-wc->w;
            y+=wc->h+w->spy;
            l=g_list_next(l);
        }
        break;
    case orientation_horizontal_vertical:
        l=w->children;
        x0=x;
        count=0;
        width/=cols;
        height/=rows;
        while (l) {
            wc=l->data;
            wc->p.x=x;
            wc->p.y=y;
            if (wc->flags & flags_fill) {
                wc->w=width;
                wc->h=height;
            }
            if (w->flags & gravity_left)
                wc->p.x=x;
            if (w->flags & gravity_xcenter)
                wc->p.x=x+(width-wc->w)/2;
            if (w->flags & gravity_right)
                wc->p.x=x+width-wc->w;
            if (w->flags & gravity_top)
                wc->p.y=y;
            if (w->flags & gravity_ycenter)
                wc->p.y=y+(height-wc->h)/2;
            if (w->flags & gravity_bottom)
                wc->p.y=y-height-wc->h;
            x+=width;
            if (++count == cols) {
                count=0;
                x=x0;
                y+=height;
            }
            l=g_list_next(l);
        }
        break;
    default:
        break;
    }
    if ((w->flags & flags_scrolly) && y > w->h+w->p.y && !w->scroll_buttons) {
        w->scroll_buttons=g_new0(struct scroll_buttons, 1);
        gui_internal_scroll_buttons_init(this, w, w->scroll_buttons);
        w->scroll_buttons->button_box->w=w->w;
        w->scroll_buttons->button_box->p.x=w->p.x;
        w->scroll_buttons->button_box->p.y=w->p.y+w->h-w->scroll_buttons->button_box->h;
        gui_internal_widget_pack(this, w->scroll_buttons->button_box);
        dbg(lvl_debug,"needs buttons %d vs %d",y,w->h);
        gui_internal_box_pack(this, w);
        return;
    }
    /*
     * Call pack again on each child,
     * the child has now had its size and coordinates
     * set so they can repack their children.
     */
    l=w->children;
    while (l) {
        wc=l->data;
        gui_internal_widget_pack(this, wc);
        l=g_list_next(l);
    }
}

/**
 * @brief Resize a box widget.
 *
 * @param this The internal GUI instance
 * @param w The widget to resize
 * @param wnew The new width of the widget
 * @param hnew THe new height of the widget
 */
void gui_internal_box_resize(struct gui_priv *this, struct widget *w, void *data, int wnew, int hnew) {
    GList *l;
    struct widget *wb;

    w->w = wnew;
    w->h = hnew;

    l=w->children;
    while (l) {
        wb=l->data;
        if (wb->on_resize) {
            int orientation=w->flags & 0xffff0000;
            switch(orientation) {
            case orientation_horizontal:
            case orientation_vertical:
            case orientation_horizontal_vertical:
                wb->h = 0;
                wb->w = 0;
                gui_internal_widget_pack(this, wb);
                break;
            default:
                wb->w = w->w;
                wb->h = w->h;
            }
            wb->on_resize(this, wb, NULL, wb->w, wb->h);
        }
        l=g_list_next(l);
    }

    /* Note: this widget and its children have been resized, a call to gui_internal_box_render() needs to be done by the caller */
}

void gui_internal_widget_reset_pack(struct gui_priv *this, struct widget *w) {
    struct widget *wc;
    GList *l;

    l=w->children;
    while (l) {
        wc=l->data;
        gui_internal_widget_reset_pack(this, wc);
        l=g_list_next(l);
    }
    if (w->packed) {
        w->w=0;
        w->h=0;
    }
}

/**
 * @brief Adds a child widget to a parent widget, making it the last child.
 *
 * @param parent The parent widget
 * @param child The child widget
 */
void gui_internal_widget_append(struct widget *parent, struct widget *child) {
    if (! child)
        return;
    if (! child->background)
        child->background=parent->background;
    parent->children=g_list_append(parent->children, child);
    child->parent=parent;
}

/**
 * @brief Adds a child widget to a parent widget, making it the first child.
 *
 * @param parent The parent widget
 * @param child The child widget
 */
void gui_internal_widget_prepend(struct widget *parent, struct widget *child) {
    if (! child->background)
        child->background=parent->background;
    parent->children=g_list_prepend(parent->children, child);
    child->parent=parent;
}

/**
 * @brief Adds a child widget to a parent widget.
 *
 * Placement of the child widget among its siblings depends on the comparison function {@code func}.
 *
 * @param parent The parent widget
 * @param child The child widget
 * @param func The comparison function
 */
void gui_internal_widget_insert_sorted(struct widget *parent, struct widget *child, GCompareFunc func) {
    if (! child->background)
        child->background=parent->background;

    parent->children=g_list_insert_sorted(parent->children, child, func);
    child->parent=parent;
}


/**
 * @brief Destroys all child widgets.
 *
 * This function is recursive, destroying all widgets in the child hierarchy of {@code w}.
 *
 * @param this The internal GUI instance
 * @param w The widget whose children are to be destroyed
 */
void gui_internal_widget_children_destroy(struct gui_priv *this, struct widget *w) {
    GList *l;
    struct widget *wc;

    l=w->children;
    while (l) {
        wc=l->data;
        gui_internal_widget_destroy(this, wc);
        l=g_list_next(l);
    }
    g_list_free(w->children);
    w->children=NULL;
}


/**
 * @brief Destroys a widget.
 *
 * This function also takes care of recursively destroying the entire child widget hierarchy of
 * {@code w} prior to destroying {@code w} itself.
 *
 * @param this The internal GUI instance
 * @param w The widget to destroy
 */
void gui_internal_widget_destroy(struct gui_priv *this, struct widget *w) {
    gui_internal_widget_children_destroy(this, w);
    g_free(w->command);
    g_free(w->speech);
    g_free(w->text);
    if (w->img)
        graphics_image_free(this->gra, w->img);
    if (w->prefix)
        g_free(w->prefix);
    if (w->name)
        g_free(w->name);
    if (w->data_free)
        w->data_free(w->data);
    if (w->cb && w->remove_cb)
        w->remove_cb(w->instance, w->cb);
    if (w==this->highlighted)
        this->highlighted=NULL;
    if(w->wfree)
        w->wfree(this,w);
    else
        g_free(w);
}


/**
 * @brief Renders widgets, preparing it for drawing on the display
 *
 * The appropriate render function will be called, depending on the type of widget
 *
 * @param this The internal GUI instance
 * @param w The widget to render
 */
void gui_internal_widget_render(struct gui_priv *this, struct widget *w) {
    if(w->p.x > this->root.w || w->p.y > this->root.h || w->state & STATE_INVISIBLE)
        return;

    switch (w->type) {
    case widget_box:
        gui_internal_box_render(this, w);
        break;
    case widget_label:
        gui_internal_label_render(this, w);
        break;
    case widget_image:
        gui_internal_image_render(this, w);
        break;
    case widget_table:
        gui_internal_table_render(this,w);
        break;
    default:
        break;
    }
}

void gui_internal_widget_pack(struct gui_priv *this, struct widget *w) {
    switch (w->type) {
    case widget_box:
        gui_internal_box_pack(this, w);
        break;
    case widget_table:
        gui_internal_table_pack(this,w);
    default:
        break;
    }
}

struct widget *
gui_internal_button_label(struct gui_priv *this, const char *label, int mode) {
    struct widget *wl,*wlb;
    struct widget *wb=gui_internal_menu_data(this)->button_bar;
    wlb=gui_internal_box_new(this, gravity_right_center|orientation_vertical);
    wl=gui_internal_label_new(this, label);
    wlb->border=1;
    wlb->foreground=this->text_foreground;
    wlb->bl=20;
    wlb->br=20;
    wlb->bb=6;
    wlb->bt=6;
    gui_internal_widget_append(wlb, wl);
    if (mode == 1)
        gui_internal_widget_prepend(wb, wlb);
    if (mode == -1)
        gui_internal_widget_append(wb, wlb);

    return wlb;
}

static void gui_internal_scroll_buttons_init(struct gui_priv *this, struct widget *widget, struct scroll_buttons *sb) {
    sb->next_button =  gui_internal_button_new_with_callback(this, _("Next"), image_new_xs(this, "gui_arrow_right"),
                       gravity_center|orientation_horizontal|flags_swap, gui_internal_table_button_next, widget);
    sb->prev_button =  gui_internal_button_new_with_callback(this, _("Prev"), image_new_xs(this, "gui_arrow_left"),
                       gravity_center|orientation_horizontal, gui_internal_table_button_prev, widget);

    sb->button_box=gui_internal_box_new(this, gravity_center|orientation_horizontal);
    sb->button_box->background=this->background;
    if(this->hide_keys) {
        sb->prev_button->state |= STATE_INVISIBLE;
        sb->next_button->state |= STATE_INVISIBLE;
    }
    sb->prev_button->state &= ~STATE_SENSITIVE;
    sb->next_button->state &= ~STATE_SENSITIVE;
    gui_internal_widget_append(sb->button_box, sb->prev_button);
    gui_internal_widget_append(sb->button_box, sb->next_button);

    sb->button_box->bl=this->spacing;
    gui_internal_widget_pack(this,sb->button_box);
}

/**
 * @brief Creates a new table widget.
 *
 * Creates and returns a new table widget.  This function will
 * setup next/previous buttons as children.
 *
 * @param this The graphics context.
 * @param flags widget sizing flags.
 * @return The newly created widget
 */
struct widget * gui_internal_widget_table_new(struct gui_priv * this, enum flags flags, int buttons) {
    struct widget * widget = g_new0(struct widget,1);
    struct table_data * data = NULL;
    widget->type=widget_table;
    widget->flags=flags;
    widget->state=STATE_SCROLLABLE;
    widget->data=g_new0(struct table_data,1);
    widget->data_free=gui_internal_table_data_free;

    // We have to set background here explicitly
    // because it will be used by inner elements created later in this
    // function (navigation buttons). Else that elements won't have
    // any background.
    widget->background=this->background;
    data = (struct table_data*)widget->data;

    if (buttons) {
        gui_internal_scroll_buttons_init(this, widget, &data->scroll_buttons);
        gui_internal_widget_append(widget, data->scroll_buttons.button_box);
    }

    return widget;

}

/**
 * @brief Clears all the rows from the table.
 *
 * This function removes all rows from a table.
 * New rows can later be added to the table.
 *
 * @param this The internal GUI instance
 * @param table The table widget
 */
void gui_internal_widget_table_clear(struct gui_priv * this,struct widget * table) {
    GList * iter;
    struct table_data * table_data = (struct table_data* ) table->data;

    iter = table->children;
    while(iter ) {
        if(iter->data != table_data->scroll_buttons.button_box) {
            struct widget * child = (struct widget*)iter->data;
            gui_internal_widget_destroy(this,child);
            if(table->children == iter) {
                table->children = g_list_remove(iter,iter->data);
                iter=table->children;
            } else
                iter = g_list_remove(iter,iter->data);
        } else {
            iter = g_list_next(iter);
        }

    }
    table_data->top_row=NULL;
    table_data->bottom_row=NULL;
}

/**
 * @brief Moves GList pointer to the next table row, skipping other table children (button box, for example).
 *
 * @param row GList pointer into the children list
 *
 * @return GList pointer to the next row in the children list, or NULL if there are no any rows left.
 */
GList *gui_internal_widget_table_next_row(GList * row) {
    while((row=g_list_next(row))!=NULL) {
        if(row->data && ((struct widget *)(row->data))->type == widget_table_row)
            break;
    }
    return row;
}

/**
 * @brief Moves GList pointer to the previous table row, skipping other table children (button box, for example).
 *
 * @param row GList pointer into the children list
 *
 * @return GList pointer to the previous row in the children list, or NULL if there are no any rows left.
 */
GList *gui_internal_widget_table_prev_row(GList * row) {
    while((row=g_list_previous(row))!=NULL) {
        if(row->data && ((struct widget *)(row->data))->type == widget_table_row)
            break;
    }
    return row;
}

/**
 * @brief Moves GList pointer to the first table row, skipping other table children (button box, for example).
 *
 * @param row GList pointer into the children list
 *
 * @return GList pointer to the first row in the children list, or NULL if table is empty.
 */
GList *gui_internal_widget_table_first_row(GList * row) {
    if(!row)
        return NULL;

    if(row->data && ((struct widget *)(row->data))->type == widget_table_row)
        return row;

    return gui_internal_widget_table_next_row(row);
}

/**
 * @brief Gets GList pointer to the table row drawn on the top of the screen.
 *
 * @return GList pointer to the top row in the children list, or NULL.
 */
GList *gui_internal_widget_table_top_row(struct gui_priv *this, struct widget * table) {
    if(table && table->type==widget_table) {
        struct table_data *d=table->data;
        return gui_internal_widget_table_first_row(d->top_row);
    }
    return NULL;
}

/**
 * @brief Sets internal top row pointer of the table to point to a given row widget.
 *
 * @return GList pointer to the top row in the children list of the table.
 */
GList *gui_internal_widget_table_set_top_row(struct gui_priv *this, struct widget * table, struct widget *row) {
    if(table && table->type==widget_table) {
        struct table_data *d=table->data;
        d->top_row=table->children;
        while(d->top_row && d->top_row->data!=row)
            d->top_row=g_list_next(d->top_row);
        if(!d->top_row)
            d->top_row=gui_internal_widget_table_first_row(table->children);
        return d->top_row;
    }
    return NULL;
}


/**
 * @brief Creates a new table_row widget.
 *
 * @param this The graphics context
 * @param flags Sizing flags for the row
 *
 * @return The new table_row widget.
 */
struct widget *
gui_internal_widget_table_row_new(struct gui_priv * this, enum flags flags) {
    struct widget * widget = g_new0(struct widget,1);
    widget->type=widget_table_row;
    widget->flags=flags;
    return widget;
}



/**
 * @brief Computes the column dimensions for the table.
 *
 * @param this The internal GUI instance
 * @param w The table widget to compute dimensions for.
 *
 * This function examines all of the rows and columns for the table w
 * and returns a list (GList) of table_column_desc elements that
 * describe each column of the table.
 *
 * The caller is responsible for freeing the returned list.
 */
static GList *gui_internal_compute_table_dimensions(struct gui_priv * this,struct widget * w) {

    GList * column_desc = NULL;
    GList * current_desc=NULL;
    GList * cur_row = w->children;
    struct widget * cur_row_widget=NULL;
    GList * cur_column=NULL;
    struct widget * cell_w=NULL;
    struct table_column_desc * current_cell=NULL;
    struct table_data * table_data=NULL;
    int height=0;
    int width=0;
    int total_width=0;
    int column_count=0;

    /*
     * Scroll through the the table and
     * 1. Compute the maximum width + height of each column across all rows.
     */
    table_data = (struct table_data*) w->data;
    for(cur_row=w->children;  cur_row ; cur_row = g_list_next(cur_row) ) {
        cur_row_widget = (struct widget*) cur_row->data;
        current_desc = column_desc;
        if(cur_row_widget == table_data->scroll_buttons.button_box) {
            continue;
        }
        column_count=0;
        for(cur_column = cur_row_widget->children; cur_column;
                cur_column=g_list_next(cur_column)) {
            cell_w = (struct widget*) cur_column->data;
            gui_internal_widget_pack(this,cell_w);
            if(current_desc == 0) {
                current_cell = g_new0(struct table_column_desc,1);
                column_desc = g_list_append(column_desc,current_cell);
                current_desc = g_list_last(column_desc);
                current_cell->height=cell_w->h;
                current_cell->width=cell_w->w;
                total_width+=cell_w->w;

            } else {
                current_cell = current_desc->data;
                height = cell_w->h;
                width = cell_w->w;
                if(current_cell->height < height ) {
                    current_cell->height = height;
                }
                if(current_cell->width < width) {
                    total_width += (width-current_cell->width);
                    current_cell->width = width;



                }
                current_desc = g_list_next(current_desc);
            }
            column_count++;

        }/* column loop */

    } /*row loop */


    /*
     * If the width of all columns is less than the width off
     * the table expand each cell proportionally.
     *
     */
    if(total_width+(this->spacing*column_count) < w->w ) {
        for(current_desc=column_desc; current_desc; current_desc=g_list_next(current_desc)) {
            current_cell = (struct table_column_desc*) current_desc->data;
            current_cell->width= ( (current_cell->width+this->spacing)/(float)total_width) * w->w ;
        }
    }

    return column_desc;
}

/* to adapt g_free to GFunc */
static void g_free_helper(void * data, void * user_data) {
    g_free(data);
}

/**
 * @brief Computes the height and width for the table.
 *
 * The height and width are computed to display all cells in the table
 * at the requested height/width.
 *
 * @param this The graphics context
 * @param w The widget to pack.
 */
void gui_internal_table_pack(struct gui_priv * this, struct widget * w) {

    int height=0;
    int width=0;
    int count=0;
    GList * column_data = gui_internal_compute_table_dimensions(this,w);
    GList * current=0;
    struct table_column_desc * cell_desc=0;
    struct table_data * table_data = (struct table_data*)w->data;

    for(current = column_data; current; current=g_list_next(current)) {
        if(table_data->scroll_buttons.button_box == current->data ) {
            continue;
        }
        cell_desc = (struct table_column_desc *) current->data;
        width = width + cell_desc->width + this->spacing;
        if(height < cell_desc->height) {
            height = cell_desc->height ;
        }
    }



    for(current=w->children; current; current=g_list_next(current)) {
        if(current->data!= table_data->scroll_buttons.button_box) {
            count++;
        }
    }

    w->w = width;
    if(w->w + w->c.x > this->root.w) {
        w->w = this->root.w - w->c.x;
    }


    if(w->h + w->c.y   > this->root.h   ) {
        /*
         * Do not allow the widget to exceed the screen.
         *
         */
        w->h = this->root.h- w->c.y  - height;
    }

    if (table_data->scroll_buttons.button_box) {
        gui_internal_widget_pack(this,table_data->scroll_buttons.button_box);
    }


    /*
     * Deallocate column descriptions.
     */
    g_list_foreach(column_data,(GFunc)g_free_helper,NULL);
    g_list_free(column_data);
}




/**
 * @brief Invalidates coordinates for previously rendered table widget rows.
 *
 * @param table_data Data from the table object.
 */
void gui_internal_table_hide_rows(struct table_data * table_data) {
    GList*cur_row;
    for(cur_row=table_data->top_row; cur_row ; cur_row = g_list_next(cur_row)) {
        struct widget * cur_row_widget = (struct widget*)cur_row->data;
        if (cur_row_widget->type!=widget_table_row)
            continue;
        cur_row_widget->p.x=0;
        cur_row_widget->p.y=0;
        cur_row_widget->w=0;
        cur_row_widget->h=0;
        if(cur_row==table_data->bottom_row)
            break;
    }
}


/**
 * @brief Renders a table widget, preparing it for drawing on the display
 *
 * @param this The internal GUI instance
 * @param w The widget to render
 */
void gui_internal_table_render(struct gui_priv * this, struct widget * w) {

    int x;
    int y;
    GList * column_desc=NULL;
    GList * cur_row = NULL;
    GList * current_desc=NULL;
    struct table_data * table_data = (struct table_data*)w->data;
    int drawing_space_left=1;
    int is_first_page;
    struct table_column_desc * dim=NULL;

    dbg_assert(table_data);
    column_desc = gui_internal_compute_table_dimensions(this,w);
    if(!column_desc)
        return;
    y=w->p.y;
    gui_internal_table_hide_rows(table_data);
    /*
     * Skip rows that are on previous pages.
     */
    if(table_data->top_row && table_data->top_row != w->children && !table_data->scroll_buttons.button_box_hide) {
        cur_row = table_data->top_row;
        is_first_page=0;
    } else {
        cur_row = w->children;
        table_data->top_row=NULL;
        is_first_page=1;
    }

    /* First, let's mark all columns as off-screen that are in rows which are *before*
     * our current page.
     * */
    GList *row = w->children;
    while (row != cur_row) {
        struct widget * cur_row_widget = (struct widget*)row->data;
        GList * cur_column=NULL;
        if(cur_row_widget == table_data->scroll_buttons.button_box) {
            row = g_list_next(row);
            continue;
        }
        for(cur_column = cur_row_widget->children; cur_column;
                cur_column=g_list_next(cur_column)) {
            struct  widget * cur_widget = (struct widget*) cur_column->data;
            if(this->hide_keys) {
                cur_widget->state |= STATE_INVISIBLE;
                cur_widget->state &= ~STATE_SENSITIVE;
            } else {
                cur_widget->state |= STATE_OFFSCREEN;
            }
        }
        row = g_list_next(row);
    }

    /*
     * Loop through each row.  Drawing each cell with the proper sizes,
     * at the proper positions.
     */
    for(table_data->top_row=cur_row; cur_row; cur_row = g_list_next(cur_row)) {
        int max_height=0, bbox_height=0;
        struct widget * cur_row_widget;
        GList * cur_column=NULL;
        current_desc = column_desc;
        cur_row_widget = (struct widget*)cur_row->data;
        x =w->p.x+this->spacing;
        if(cur_row_widget == table_data->scroll_buttons.button_box) {
            continue;
        }
        dim = (struct table_column_desc*)current_desc->data;

        if (table_data->scroll_buttons.button_box && !table_data->scroll_buttons.button_box_hide)
            bbox_height=table_data->scroll_buttons.button_box->h;

        if( y + dim->height + bbox_height + this->spacing >= w->p.y + w->h ) {
            drawing_space_left=0;
        }
        for(cur_column = cur_row_widget->children; cur_column;
                cur_column=g_list_next(cur_column)) {
            struct  widget * cur_widget = (struct widget*) cur_column->data;
            if (drawing_space_left) {
                cur_widget->p.x=x;
                cur_widget->w=dim->width;
                cur_widget->p.y=y;
                cur_widget->h=dim->height;
                x=x+cur_widget->w;
                max_height = dim->height;
                /* We pack the widget before rendering to ensure that the x and y
                 * coordinates get pushed down.
                 */
                if(this->hide_keys) {
                    cur_widget->state &= ~STATE_INVISIBLE;
                    cur_widget->state |= STATE_SENSITIVE;
                } else {
                    cur_widget->state &= ~STATE_OFFSCREEN;
                }

#if defined(GUI_INTERNAL_VISUAL_DBG)

                static struct graphics_gc *debug_gc=NULL;
                static struct color gui_table_debug_color= {0x0000,0xffff,0x0400,0xffff}; /* Green */
                int visual_debug = (debug_level_get("gui_internal_visual_layout") >= lvl_debug);

                if (visual_debug)
                    dbg(lvl_debug, "Internal layout visual debugging is enabled");

                if (visual_debug && !debug_gc) {
                    debug_gc = graphics_gc_new(this->gra);
                    graphics_gc_set_foreground(debug_gc, &gui_table_debug_color);
                    graphics_gc_set_linewidth(debug_gc, 1);
                }
#endif

                gui_internal_widget_pack(this,cur_widget);
                gui_internal_widget_render(this,cur_widget);

#if defined(GUI_INTERNAL_VISUAL_DBG)
                if (visual_debug) {
                    struct point pnt[5];
                    pnt[0]=cur_widget->p;
                    pnt[1].x=pnt[0].x+cur_widget->w;
                    pnt[1].y=pnt[0].y;
                    pnt[2].x=pnt[0].x+cur_widget->w;
                    pnt[2].y=pnt[0].y+cur_widget->h;
                    pnt[3].x=pnt[0].x;
                    pnt[3].y=pnt[0].y+cur_widget->h;
                    pnt[4]=pnt[0];
                    graphics_draw_lines(this->gra, debug_gc, pnt, 5);	/* Force highlighting table borders in debug more */
                }
#endif

                if(dim->height > max_height) {
                    max_height = dim->height;
                }
            } else {
                /* Deactivate contents that we don't have space for. */
                if(this->hide_keys) {
                    cur_widget->state |= STATE_INVISIBLE;
                    cur_widget->state &= ~STATE_SENSITIVE;
                } else {
                    cur_widget->state |= STATE_OFFSCREEN;
                }
            }
        }

        if (drawing_space_left) {
            /* Row object should have its coordinates in actual
             * state to be able to pass mouse clicks to Column objects
             */
            cur_row_widget->p.x=w->p.x;
            cur_row_widget->w=w->w;
            cur_row_widget->p.y=y;
            cur_row_widget->h=max_height;
            y = y + max_height;
            table_data->bottom_row=cur_row;
            current_desc = g_list_next(current_desc);
        }
    }

    /* By default, hide all scroll buttons. */
    if(this->hide_keys) {
        table_data->scroll_buttons.next_button->state|= STATE_INVISIBLE;
        table_data->scroll_buttons.prev_button->state|= STATE_INVISIBLE;
    }
    table_data->scroll_buttons.next_button->state&= ~STATE_SENSITIVE;
    table_data->scroll_buttons.prev_button->state&= ~STATE_SENSITIVE;

    if(table_data->scroll_buttons.button_box && (!drawing_space_left || !is_first_page)
            && !table_data->scroll_buttons.button_box_hide ) {
        table_data->scroll_buttons.button_box->p.y =w->p.y+w->h-table_data->scroll_buttons.button_box->h -
                this->spacing;
        if(table_data->scroll_buttons.button_box->p.y < y ) {
            table_data->scroll_buttons.button_box->p.y=y;
        }
        table_data->scroll_buttons.button_box->p.x = w->p.x;
        table_data->scroll_buttons.button_box->w = w->w;
        gui_internal_widget_pack(this,table_data->scroll_buttons.button_box);
        if(table_data->scroll_buttons.next_button->p.y > w->p.y + w->h + table_data->scroll_buttons.next_button->h) {
            table_data->scroll_buttons.button_box->p.y = w->p.y + w->h -
                    table_data->scroll_buttons.button_box->h;
        }
        if(!drawing_space_left) {
            table_data->scroll_buttons.next_button->state|= STATE_SENSITIVE;
            table_data->scroll_buttons.next_button->state&= ~STATE_INVISIBLE;
        }

        if(table_data->top_row != w->children) {
            table_data->scroll_buttons.prev_button->state|= STATE_SENSITIVE;
            table_data->scroll_buttons.prev_button->state&= ~STATE_INVISIBLE;
        }
        gui_internal_widget_render(this,table_data->scroll_buttons.button_box);
    }

    /*
     * Deallocate column descriptions.
     */
    g_list_foreach(column_desc,(GFunc)g_free_helper,NULL);
    g_list_free(column_desc);
}

/**
 * @brief Handles the 'next page' table event.
 *
 * A callback function that is invoked when the 'next page' button is pressed
 * to advance the contents of a table widget.
 *
 * @param this The graphics context.
 * @param wm The button widget that was pressed.
 * @param data
 */
void gui_internal_table_button_next(struct gui_priv * this, struct widget * wm, void *data) {
    struct widget * table_widget=NULL;
    struct table_data * table_data = NULL;

    if(wm)
        table_widget = (struct widget * ) wm->data;
    else
        table_widget = data;

    if(table_widget && table_widget->type==widget_table)
        table_data = (struct table_data*) table_widget->data;

    if(table_data) {
        GList *l=g_list_next(table_data->bottom_row);
        if(l)	{
            gui_internal_table_hide_rows(table_data);
            table_data->top_row=l;
        }
    }

    if(wm)
        wm->state&= ~STATE_HIGHLIGHTED;

    gui_internal_menu_render(this);
}

/**
 * @brief Handles the 'previous page' table event.
 *
 * A callback function that is invoked when the 'previous page' button is pressed
 * to go back in the contents of a table widget.
 *
 * @param this The graphics context.
 * @param wm The button widget that was pressed.
 */
void gui_internal_table_button_prev(struct gui_priv * this, struct widget * wm, void *data) {
    struct widget * table_widget = NULL;
    struct table_data * table_data = NULL;

    if(wm)
        table_widget=(struct widget * ) wm->data;
    else
        table_widget=(struct widget * ) data;

    if(table_widget && table_widget->type==widget_table)
        table_data = (struct table_data*) table_widget->data;

    if(table_data)	{
        int bottomy=table_widget->p.y+table_widget->h;
        int n;
        GList *top=table_data->top_row;
        struct widget *w=(struct widget*)top->data;
        if(table_data->scroll_buttons.button_box->p.y!=0) {
            bottomy=table_data->scroll_buttons.button_box->p.y;
        }
        n=(bottomy-w->p.y)/w->h;
        while(n-- > 0 && (top=g_list_previous(top))!=NULL);
        gui_internal_table_hide_rows(table_data);
        table_data->top_row=top;
    }
    if(wm)
        wm->state&= ~STATE_HIGHLIGHTED;
    gui_internal_menu_render(this);
}


/**
 * @brief Deallocates a table_data structure.
 *
 * @note button_box and its children (next_button,prev_button)
 * have their memory managed by the table itself.
 *
 * @param p The table_data structure
 */
void gui_internal_table_data_free(void * p) {
    g_free(p);
}

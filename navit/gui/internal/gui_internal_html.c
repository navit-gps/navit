#include <glib.h>
#include <stdlib.h>
#include <ctype.h>
#include "callback.h"
#include "debug.h"
#include "coord.h"
#include "point.h"
#include "color.h"
#include "graphics.h"
#include "xmlconfig.h"
#include "navit_nls.h"
#include "gui.h"
#include "command.h"
struct gui_priv;
#include "gui_internal.h"
#include "gui_internal_widget.h"
#include "gui_internal_priv.h"
#include "gui_internal_html.h"
#include "gui_internal_keyboard.h"
#include "gui_internal_menu.h"



struct form {
    char *onsubmit;
};

struct html_tag_map {
    char *tag_name;
    enum html_tag tag;
} html_tag_map[] = {
    {"a",html_tag_a},
    {"h1",html_tag_h1},
    {"html",html_tag_html},
    {"img",html_tag_img},
    {"script",html_tag_script},
    {"form",html_tag_form},
    {"input",html_tag_input},
    {"div",html_tag_div},
};


static const char *find_attr(const char **names, const char **values, const char *name) {
    while (*names) {
        if (!g_ascii_strcasecmp(*names, name))
            return *values;
        names+=XML_ATTR_DISTANCE;
        values+=XML_ATTR_DISTANCE;
    }
    return NULL;
}

static char *find_attr_dup(const char **names, const char **values, const char *name) {
    return g_strdup(find_attr(names, values, name));
}

void gui_internal_html_main_menu(struct gui_priv *this) {
    gui_internal_prune_menu(this, NULL);
    gui_internal_html_load_href(this, "#Main Menu", 0);
}

static void gui_internal_html_command(struct gui_priv *this, struct widget *w, void *data) {
    gui_internal_evaluate(this,w->command);
}

static void gui_internal_html_submit_set(struct gui_priv *this, struct widget *w, struct form *form) {
    GList *l;
    if (w->form == form && w->name) {
        struct attr *attr=attr_new_from_text(w->name, w->text?w->text:"");
        if (attr)
            gui_set_attr(this->self.u.gui, attr);
        attr_free(attr);
    }
    l=w->children;
    while (l) {
        w=l->data;
        gui_internal_html_submit_set(this, w, form);
        l=g_list_next(l);
    }

}

static void gui_internal_html_submit(struct gui_priv *this, struct widget *w, void *data) {
    struct widget *menu;
    GList *l;

    dbg(lvl_debug,"enter form %p %s",w->form,w->form->onsubmit);
    l=g_list_last(this->root.children);
    menu=l->data;
    graphics_draw_mode(this->gra, draw_mode_begin);
    gui_internal_highlight_do(this, NULL);
    gui_internal_menu_render(this);
    graphics_draw_mode(this->gra, draw_mode_end);
    gui_internal_html_submit_set(this, menu, w->form);
    gui_internal_evaluate(this,w->form->onsubmit);
}

void gui_internal_html_load_href(struct gui_priv *this, char *href, int replace) {
    if (replace)
        gui_internal_prune_menu_count(this, 1, 0);
    if (href && href[0] == '#') {
        dbg(lvl_debug,"href=%s",href);
        g_free(this->href);
        this->href=g_strdup(href);
        gui_internal_html_menu(this, this->html_text, href+1);
    }
}

void gui_internal_html_href(struct gui_priv *this, struct widget *w, void *data) {
    gui_internal_html_load_href(this, w->command, 0);
}

struct div_flags_map {
    char *attr;
    char *val;
    enum flags flags;
} div_flags_map[] = {
    {"gravity","none",gravity_none},
    {"gravity","left",gravity_left},
    {"gravity","xcenter",gravity_xcenter},
    {"gravity","right",gravity_right},
    {"gravity","top",gravity_top},
    {"gravity","ycenter",gravity_ycenter},
    {"gravity","bottom",gravity_bottom},
    {"gravity","left_top",gravity_left_top},
    {"gravity","top_center",gravity_top_center},
    {"gravity","right_top",gravity_right_top},
    {"gravity","left_center",gravity_left_center},
    {"gravity","center",gravity_center},
    {"gravity","right_center",gravity_right_center},
    {"gravity","left_bottom",gravity_left_bottom},
    {"gravity","bottom_center",gravity_bottom_center},
    {"gravity","right_bottom",gravity_right_bottom},
    {"expand","1",flags_expand},
    {"fill","1",flags_fill},
    {"orientation","horizontal",orientation_horizontal},
    {"orientation","vertical",orientation_vertical},
    {"orientation","horizontal_vertical",orientation_horizontal_vertical},
};

static enum flags div_flag(const char **names, const char **values, char *name) {
    int i;
    enum flags ret=0;
    const char *value=find_attr(names, values, name);
    if (!value)
        return ret;
    for (i = 0 ; i < sizeof(div_flags_map)/sizeof(struct div_flags_map); i++) {
        if (!strcmp(div_flags_map[i].attr,name) && !strcmp(div_flags_map[i].val,value))
            ret|=div_flags_map[i].flags;
    }
    return ret;
}

static enum flags div_flags(const char **names, const char **values) {
    enum flags flags;
    flags = div_flag(names, values, "gravity");
    flags |= div_flag(names, values, "orientation");
    flags |= div_flag(names, values, "expand");
    flags |= div_flag(names, values, "fill");
    return flags;
}

static struct widget *html_image(struct gui_priv *this, const char **names, const char **values) {
    const char *src, *size;
    struct graphics_image *img=NULL;

    src=find_attr(names, values, "src");
    if (!src)
        return NULL;
    size=find_attr(names, values, "size");
    if (!size) {
        const char *class=find_attr(names, values, "class");
        if (class && !strcasecmp(class,"centry"))
            size="xs";
        else
            size="l";
    }
    if (!strcmp(size,"l"))
        img=image_new_l(this, src);
    else if (!strcmp(size,"s"))
        img=image_new_s(this, src);
    else if (!strcmp(size,"xs"))
        img=image_new_xs(this, src);
    if (!img)
        return NULL;
    return gui_internal_image_new(this, img);
}

static void gui_internal_html_start(xml_context *dummy, const char *tag_name, const char **names, const char **values,
                                    void *data,
                                    GError **error) {
    struct gui_priv *this=data;
    int i;
    enum html_tag tag=html_tag_none;
    struct html *html=&this->html[this->html_depth];
    const char *cond, *type, *font_size;

    if (!g_ascii_strcasecmp(tag_name,"text") || !g_ascii_strcasecmp(tag_name,"p"))
        return;
    html->skip=0;
    html->command=NULL;
    html->name=NULL;
    html->href=NULL;
    html->refresh_cond=NULL;
    html->w=NULL;
    html->container=NULL;
    html->font_size=0;
    cond=find_attr(names, values, "cond");

    if (cond && !this->html_skip) {
        if (!command_evaluate_to_boolean(&this->self, cond, NULL))
            html->skip=1;
    }

    for (i=0 ; i < sizeof(html_tag_map)/sizeof(struct html_tag_map); i++) {
        if (!g_ascii_strcasecmp(html_tag_map[i].tag_name, tag_name)) {
            tag=html_tag_map[i].tag;
            break;
        }
    }
    html->tag=tag;
    html->class=find_attr_dup(names, values, "class");
    if (!this->html_skip && !html->skip) {
        switch (tag) {
        case html_tag_a:
            html->name=find_attr_dup(names, values, "name");
            if (html->name) {
                html->skip=this->html_anchor ? strcmp(html->name,this->html_anchor) : 0;
                if (!html->skip)
                    this->html_anchor_found=1;
            }
            html->command=find_attr_dup(names, values, "onclick");
            html->href=find_attr_dup(names, values, "href");
            html->refresh_cond=find_attr_dup(names, values, "refresh_cond");
            break;
        case html_tag_img:
            html->command=find_attr_dup(names, values, "onclick");
            html->w=html_image(this, names, values);
            break;
        case html_tag_form:
            this->form=g_new0(struct form, 1);
            this->form->onsubmit=find_attr_dup(names, values, "onsubmit");
            break;
        case html_tag_input:
            type=find_attr_dup(names, values, "type");
            if (!type)
                break;
            if (!strcmp(type,"image")) {
                html->w=html_image(this, names, values);
                if (html->w) {
                    html->w->state|=STATE_SENSITIVE;
                    html->w->func=gui_internal_html_submit;
                }
            }
            if (!strcmp(type,"text") || !strcmp(type,"password")) {
                const char *value=find_attr(names, values, "value");
                html->w=gui_internal_label_new(this, value);
                html->w->background=this->background;
                html->w->flags |= div_flags(names, values);
                html->w->state|=STATE_EDITABLE;
                if (!this->editable) {
                    this->editable=html->w;
                    html->w->state|=STATE_EDIT;
                }
                this->keyboard_required=1;
                if (!strcmp(type,"password"))
                    html->w->flags2 |= 1;
            }
            if (html->w) {
                html->w->form=this->form;
                html->w->name=find_attr_dup(names, values, "name");
            }
            break;
        case html_tag_div:
            html->w=gui_internal_box_new(this, div_flags(names, values));
            font_size=find_attr(names, values, "font");
            if (font_size)
                html->font_size=atoi(font_size);
            html->container=this->html_container;
            this->html_container=html->w;
            break;
        default:
            break;
        }
    }
    this->html_skip+=html->skip;
    this->html_depth++;
}

static void gui_internal_html_end(xml_context *dummy, const char *tag_name, void *data, GError **error) {
    struct gui_priv *this=data;
    struct html *html;
    struct html *parent=NULL;

    if (!g_ascii_strcasecmp(tag_name,"text") || !g_ascii_strcasecmp(tag_name,"p"))
        return;
    this->html_depth--;
    html=&this->html[this->html_depth];
    if (this->html_depth > 0)
        parent=&this->html[this->html_depth-1];


    if (!this->html_skip) {
        if (html->command && html->w) {
            html->w->state |= STATE_SENSITIVE;
            html->w->command=html->command;
            html->w->func=gui_internal_html_command;
            html->command=NULL;
        }
        if (parent && (parent->href || parent->command) && html->w) {
            html->w->state |= STATE_SENSITIVE;
            if (parent->command) {
                html->w->command=g_strdup(parent->command);
                html->w->func=gui_internal_html_command;
            } else {
                html->w->command=g_strdup(parent->href);
                html->w->func=gui_internal_html_href;
            }
        }
        switch (html->tag) {
        case html_tag_div:
            this->html_container=html->container;
        case html_tag_img:
        case html_tag_input:
            gui_internal_widget_append(this->html_container, html->w);
            break;
        case html_tag_form:
            this->form=NULL;
            break;
        default:
            break;
        }
    }
    this->html_skip-=html->skip;
    g_free(html->command);
    g_free(html->name);
    g_free(html->href);
    g_free(html->class);
    g_free(html->refresh_cond);
}

static void gui_internal_refresh_callback_called(struct gui_priv *this, struct menu_data *menu_data) {
    if (gui_internal_menu_data(this) == menu_data) {
        char *href=g_strdup(menu_data->href);
        gui_internal_html_load_href(this, href, 1);
        g_free(href);
    }
}

static void gui_internal_set_refresh_callback(struct gui_priv *this, char *cond) {
    dbg(lvl_info,"cond=%s",cond);
    if (cond) {
        enum attr_type type;
        struct object_func *func;
        struct menu_data *menu_data=gui_internal_menu_data(this);
        dbg(lvl_info,"navit=%p",this->nav);
        type=command_evaluate_to_attr(&this->self, cond, NULL, &menu_data->refresh_callback_obj);
        if (type == attr_none) {
            dbg(lvl_error,"can't get type of '%s'",cond);
            return;
        }
        func=object_func_lookup(menu_data->refresh_callback_obj.type);
        if (!func)
            dbg(lvl_error,"'%s' has no functions",cond);
        if (func && !func->add_attr)
            dbg(lvl_error,"'%s' has no add_attr function",cond);
        if (!func || !func->add_attr)
            return;
        menu_data->refresh_callback.type=attr_callback;
        menu_data->refresh_callback.u.callback=callback_new_attr_2(callback_cast(gui_internal_refresh_callback_called),type,
                                               this,menu_data);
        func->add_attr(menu_data->refresh_callback_obj.u.data, &menu_data->refresh_callback);
    }
}

static void gui_internal_html_text(xml_context *dummy, const char *text, gsize len, void *data, GError **error) {
    struct gui_priv *this=data;
    struct widget *w;
    int depth=this->html_depth-1;
    struct html *html=&this->html[depth];
    gchar *text_stripped;

    if (this->html_skip)
        return;
    while (isspace(text[0])) {
        text++;
        len--;
    }
    while (len > 0 && isspace(text[len-1]))
        len--;

    text_stripped = g_strndup(text, len);
    if (html->tag == html_tag_html && depth > 2) {
        if (this->html[depth-1].tag == html_tag_script) {
            html=&this->html[depth-2];
        }
    }
    switch (html->tag) {
    case html_tag_a:
        if (html->name && len) {
            if (html->class && !strcasecmp(html->class,"clist"))
                this->html_container=gui_internal_box_new(this,
                                     gravity_left_top|orientation_vertical|flags_expand|flags_fill /* |flags_scrolly */);
            else
                this->html_container=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_expand|flags_fill);
            gui_internal_widget_append(gui_internal_menu(this, _(text_stripped)), this->html_container);
            gui_internal_menu_data(this)->href=g_strdup(this->href);
            gui_internal_set_refresh_callback(this, html->refresh_cond);
            this->html_container->spx=this->spacing*10;
        }
        break;
    case html_tag_h1:
        if (!this->html_container) {
            this->html_container=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_expand|flags_fill);
            gui_internal_widget_append(gui_internal_menu(this, _(text_stripped)), this->html_container);
            this->html_container->spx=this->spacing*10;
        }
        break;
    case html_tag_img:
        if (len) {
            if (html->class && !strcasecmp(html->class, "centry"))
                w=gui_internal_box_new(this, gravity_left_top|orientation_horizontal|flags_fill);
            else
                w=gui_internal_box_new(this, gravity_center|orientation_vertical);
            gui_internal_widget_append(w, html->w);
            gui_internal_widget_append(w, gui_internal_text_new(this, _(text_stripped),
                                       gravity_left_top|orientation_vertical|flags_fill));
            html->w=w;
        }
        break;
    case html_tag_div:
        if (len) {
            gui_internal_widget_append(html->w, gui_internal_text_font_new(this, _(text_stripped), html->font_size,
                                       gravity_center|orientation_vertical));
        }
        break;
    case html_tag_script:
        dbg(lvl_debug,"execute %s",text_stripped);
        gui_internal_evaluate(this,text_stripped);
        break;
    default:
        break;
    }
    g_free(text_stripped);
}

void gui_internal_html_parse_text(struct gui_priv *this, char *doc) {
    xml_parse_text(doc, this, gui_internal_html_start, gui_internal_html_end, gui_internal_html_text);
}

void gui_internal_html_menu(struct gui_priv *this, const char *document, char *anchor) {
    char *doc=g_strdup(document);
    graphics_draw_mode(this->gra, draw_mode_begin);
    this->html_container=NULL;
    this->html_depth=0;
    this->html_anchor=anchor;
    this->html_anchor_found=0;
    this->form=NULL;
    this->keyboard_required=0;
    this->editable=NULL;
    callback_list_call_attr_2(this->cbl,attr_gui,anchor,&doc);
    gui_internal_html_parse_text(this, doc);
    g_free(doc);
    if (this->keyboard_required) {
        this->html_container->flags=gravity_center|orientation_vertical|flags_expand|flags_fill;
        if (this->keyboard)
            gui_internal_widget_append(this->html_container, gui_internal_keyboard(this,
                                       VKBD_FLAG_2 | gui_internal_keyboard_init_mode(getenv("LANG"))));
        else
            gui_internal_keyboard_show_native(this, this->html_container,
                                              VKBD_FLAG_2 | gui_internal_keyboard_init_mode(getenv("LANG")), getenv("LANG"));
    }
    gui_internal_menu_render(this);
    graphics_draw_mode(this->gra, draw_mode_end);
}


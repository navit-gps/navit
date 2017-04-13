#include <glib.h>
#include <QQmlApplicationEngine>

#include "config.h"
#include "plugin.h"
#include "item.h"
#include "attr.h"
#include "xmlconfig.h"
#include "color.h"
#include "gui.h"
#include "callback.h"
#include "debug.h"
#include "navit.h"
#include "point.h"
#include "graphics.h"
#include "event.h"
#include "map.h"
#include "coord.h"
#include "vehicle.h"
#include "coord.h"
#include "transform.h"
#include "mapset.h"
#include "route.h"
#include "country.h"
#include "track.h"
#include "search.h"
#include "bookmarks.h"
#include "command.h"
#include "keys.h"

#include "layout.h"

struct gui_priv {
    /* navit internal handle */
    struct navit *nav;
    /* gui handle */
    struct gui *gui;

    /* attributes given to us */
    struct attr attributes;

    /* list of callbacks to navit */
    struct callback_list* callbacks;
    /* own callbacks *
     * TODO: Why do we need them as members? */
    struct callback *button_cb;
    struct callback *motion_cb;
    struct callback *resize_cb;
    struct callback *keypress_cb;
    struct callback *window_closed_cb;

    /* current graphics */
    struct graphics * gra;
    /* root window */
    struct window *win;
    /* navit root widget dimesnions*/
    int w;
    int h;

    /* Qt application instance */
    QQmlApplicationEngine * engine;
    QObject * loader; /* Loader QML component to load our QML parts to the QML engine */

    /* configuration */
    int menu_on_map_click;
};

//Meta object
#include "gui_qt5_qml.moc"
static void gui_qt5_qml_button(void *data, int pressed, int button, struct point *p)
{
    struct gui_priv *gui_priv=(struct gui_priv*)data;

    /* check if navit wants to handle this */
    if (!navit_handle_button(gui_priv->nav, pressed, button, p, NULL)) {
        dbg(lvl_debug,"navit has handled button\n");
        return;
    }
    dbg(lvl_debug,"enter %d %d\n", pressed, button);

    /* check if user requested menu */
    if ( button == 1 && gui_priv->menu_on_map_click ) {
        dbg(lvl_debug,"navit wants us to enter menu\n");
	/*TODO: want to emit a signal somewhere? */
    }
}

static void gui_qt5_qml_motion(void *data, struct point *p)
{
    struct gui_priv *gui_priv=(struct gui_priv*)data;
    dbg(lvl_debug,"enter (%d, %d)\n",p->x, p->y);
    /* forward this to navit  */
    navit_handle_motion(gui_priv->nav, p);
}

static void gui_qt5_qml_resize(void *data, int w, int h)
{
    struct gui_priv *gui_priv=(struct gui_priv*)data;
    dbg(lvl_debug,"enter\n");
    /* forward this to navit */
    navit_handle_resize(gui_priv->nav, w, h);
}

static int gui_qt5_qml_set_graphics(struct gui_priv *gui_priv, struct graphics *gra)
{
    struct transformation * trans;
    dbg(lvl_debug,"enter\n");

    /* get navit transition */
    trans=navit_get_trans(gui_priv->nav);

    /* Tell navit to ignore events from graphics. We will hook the ones being supported.*/
    navit_ignore_graphics_events(gui_priv->nav, 1);

    /* remeber graphics */
    gui_priv->gra = gra;

    /* hook button callback */
    gui_priv->button_cb=callback_new_attr_1(callback_cast(gui_qt5_qml_button), attr_button, gui_priv);
    graphics_add_callback(gra, gui_priv->button_cb);

    /* hook motion callback */
    gui_priv->motion_cb=callback_new_attr_1(callback_cast(gui_qt5_qml_motion), attr_motion, gui_priv);
    graphics_add_callback(gra, gui_priv->motion_cb);

    /* hook resize callback. Will be called imediately!*/
    gui_priv->resize_cb=callback_new_attr_1(callback_cast(gui_qt5_qml_resize), attr_resize, gui_priv);
    graphics_add_callback(gra, gui_priv->resize_cb);

    /* get main navit window */
    gui_priv->win=(struct window *)graphics_get_data(gra, "window");
    if (! gui_priv->win) {
        dbg(lvl_error, "failed to obtain window from graphics plugin, cannot set graphics\n");
        return 1;
    }

    /* expect to have qt5 graphics. So get the qml engine prepared by graphics */
    gui_priv->engine=(QQmlApplicationEngine *)graphics_get_data(gra, "engine");
    if (gui_priv->engine == NULL) {
        dbg(lvl_error, "Graphics doesn't seem to be qt5, or doesn't have QML. Cannot set graphics\n");
        return 1;
    }
    
    /* find the loader component */
    gui_priv->loader = gui_priv->engine->rootObjects().value(0)->findChild<QObject*>("navit_loader");
    if(gui_priv->loader != NULL)
    {
        dbg(lvl_debug, "navit_loader found\n");
        /* load our root window into the loader component */
        gui_priv->loader->setProperty("source", "qrc:///gui_qt5_qml.qml");
    }

    transform_get_size(trans, &gui_priv->w, &gui_priv->h);
    dbg(lvl_debug, "navit provided geometry: (%d, %d)\n", gui_priv->w, gui_priv->h);

    /* Was resize callback already issued? */
//    if (navit_get_ready(gui_priv->nav) & 2)
//        gui_internal_setup(this);

    /* allow navit to draw */
    navit_draw(gui_priv->nav);
    return 0;
}

static int
gui_qt5_qml_get_attr(struct gui_priv *gui_priv, enum attr_type type, struct attr *attr)
{
    dbg(lvl_debug,"enter\n");
    return 1;
}

static int
gui_qt5_qml_set_attr(struct gui_priv *gui_priv, struct attr *attr)
{
    dbg(lvl_debug,"enter\n");
    return 1;
}

struct gui_methods gui_qt5_qml_methods = {
    NULL,
    NULL,
    gui_qt5_qml_set_graphics,
    NULL,
    NULL,
    NULL,
    NULL,
    gui_qt5_qml_get_attr,
    NULL,
    gui_qt5_qml_set_attr,
};

static struct gui_priv * gui_qt5_qml_new(struct navit *nav, struct gui_methods *meth, struct attr **attrs, struct gui *gui)
{
    struct gui_priv * gui_priv;
    struct attr *attr;

    dbg(lvl_debug,"enter\n");

    /* tell navit our methods */
    *meth=gui_qt5_qml_methods;

    /* allocate gui private structure */
    gui_priv=g_new0(struct gui_priv, 1);
   
    /* default config */
    gui_priv->menu_on_map_click=1;

    /* read config */
    if( (attr=attr_search(attrs,NULL,attr_menu_on_map_click)))
    gui_priv->menu_on_map_click=attr->u.num;

    /* remember navit internal handle */
    gui_priv->nav=nav;
    /* remember our gui handle */
    gui_priv->gui=gui;

    /* remember the attributes given to us */
    gui_priv->attributes.type=attr_gui;
    gui_priv->attributes.u.gui=gui;	

    /* create new callbacks */
    gui_priv->callbacks = callback_list_new();

    /* return self */
    return gui_priv;
}

void plugin_init(void) {
    Q_INIT_RESOURCE(gui_qt5_qml);
    plugin_register_category_gui("qt5_qml",gui_qt5_qml_new);
}

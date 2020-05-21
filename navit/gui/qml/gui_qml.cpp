#include <glib.h>
#include <QtCore>
#include <QtGui>
#include <QtDeclarative>
#include <QtXml>
#include "config.h"
#ifdef HAVE_API_WIN32_BASE
#include <windows.h>
#endif
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
    struct navit *nav;
    struct gui *gui;
    struct attr self;
    struct vehicle* currVehicle;

    //configuration items
    int fullscreen;
    int menu_on_map_click;
    int signal_on_map_click;
    int w;
    int h;
    char *source;
    char *skin;
    char* icon_src;
    int radius;
    int pitch;
    int lazy; //When TRUE - menu state will not be changed during map/menu switches, FALSE - menu will be always reset to main.qml

    //Interface stuff
    struct callback_list *cbl;
    QCoreApplication *app;
    struct window *win;
    struct graphics *gra;
    QWidget *mainWindow;
    QWidget *graphicsWidget;
    QDeclarativeView *guiWidget;
    QDeclarativeView *prevGuiWidget;
    QStackedLayout *switcherWidget;
    struct callback *button_cb;
    struct callback *motion_cb;
    struct callback *resize_cb;
    struct callback *keypress_cb;
    struct callback *window_closed_cb;

    //Proxy objects
    class NGQProxyGui* guiProxy;
    class NGQProxyNavit* navitProxy;
    class NGQProxyVehicle* vehicleProxy;
    class NGQProxySearch* searchProxy;
    class NGQProxyBookmarks* bookmarksProxy;
    class NGQProxyRoute* routeProxy;
    class NGQPoint* currentPoint;
};

#include "proxy.h"
#include "ngqpoint.h"
#include "searchProxy.h"
#include "routeProxy.h"
#include "bookmarksProxy.h"
#include "vehicleProxy.h"
#include "navitProxy.h"
#include "guiProxy.h"

//Main window class for resizeEvent handling
#ifdef Q_WS_X11
#include <QX11EmbedWidget>
class NGQMainWindow : public QX11EmbedWidget {
#else
class NGQMainWindow : public QWidget {
#endif /* Q_WS_X11 */
public:
#ifdef Q_WS_X11
    NGQMainWindow(struct gui_priv* this_,QWidget *parent) : QX11EmbedWidget(parent) {
#else
    NGQMainWindow(struct gui_priv* this_,QWidget *parent) : QWidget(parent) {
#endif /* Q_WS_X11  */
        this->object=this_;
    }
protected:
    void resizeEvent(QResizeEvent *) {
        this->object->w=this->width();
        this->object->h=this->height();
        //YES, i KNOW about signal/slot thing
        this->object->guiProxy->setWidth(this->width());
        this->object->guiProxy->setHeight(this->height());
    }
    void closeEvent(QCloseEvent* event) {
        this->object->graphicsWidget->close();
    }
private:
    struct gui_priv* object;
};

//Meta object
#include "gui_qml.moc"

static void gui_qml_dbus_signal(struct gui_priv *this_, struct point *p) {
    struct displaylist_handle *dlh;
    struct displaylist *display;
    struct displayitem *di;

    display=navit_get_displaylist(this_->nav);
    dlh=graphics_displaylist_open(display);
    while ((di=graphics_displaylist_next(dlh))) {
        struct item *item=graphics_displayitem_get_item(di);
        if (item_is_point(*item) && graphics_displayitem_get_displayed(di) &&
                graphics_displayitem_within_dist(display, di, p, 10)) {
            struct map_rect *mr=map_rect_new(item->map, NULL);
            struct item *itemo=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
            struct attr attr;
            if (item_attr_get(itemo, attr_data, &attr)) {
                struct attr cb,*attr_list[2];
                int valid=0;
                attr.type=attr_data;
                attr_list[0]=&attr;
                attr_list[1]=NULL;
                if (navit_get_attr(this_->nav, attr_callback_list, &cb, NULL))
                    callback_list_call_attr_4(cb.u.callback_list, attr_command, "dbus_send_signal", attr_list, NULL, &valid);
            }
            map_rect_destroy(mr);
        }
    }
    graphics_displaylist_close(dlh);
}

static void gui_qml_button(void *data, int pressed, int button, struct point *p) {
    struct gui_priv *this_=(struct gui_priv*)data;

    // check whether the position of the mouse changed during press/release OR if it is the scrollwheel
    if (!navit_handle_button(this_->nav, pressed, button, p, NULL)) {
        dbg(lvl_debug,"navit has handled button");
        return;
    }

    dbg(lvl_debug,"enter %d %d", pressed, button);
    if (this_->signal_on_map_click) {
        gui_qml_dbus_signal(this_, p);
        return;
    }

    if ( button == 1 && this_->menu_on_map_click ) {
        this_->guiProxy->switchToMenu(p);
    }
}

static void gui_qml_motion(void *data, struct point *p) {
    struct gui_priv *this_=(struct gui_priv*)data;
    navit_handle_motion(this_->nav, p);
    return;
}
static void gui_qml_resize(void *data, int w, int h) {
    struct gui_priv *this_=(struct gui_priv*)data;
    navit_handle_resize(this_->nav, w, h);
}

static void gui_qml_keypress(void *data, char *key) {
    struct gui_priv *this_=(struct gui_priv*) data;
    int w,h;
    struct point p;
    transform_get_size(navit_get_trans(this_->nav), &w, &h);
    switch (*key) {
    case NAVIT_KEY_UP:
        p.x=w/2;
        p.y=0;
        navit_set_center_screen(this_->nav, &p, 1);
        break;
    case NAVIT_KEY_DOWN:
        p.x=w/2;
        p.y=h;
        navit_set_center_screen(this_->nav, &p, 1);
        break;
    case NAVIT_KEY_LEFT:
        p.x=0;
        p.y=h/2;
        navit_set_center_screen(this_->nav, &p, 1);
        break;
    case NAVIT_KEY_RIGHT:
        p.x=w;
        p.y=h/2;
        navit_set_center_screen(this_->nav, &p, 1);
        break;
    case NAVIT_KEY_ZOOM_IN:
        navit_zoom_in(this_->nav, 2, NULL);
        break;
    case NAVIT_KEY_ZOOM_OUT:
        navit_zoom_out(this_->nav, 2, NULL);
        break;
    case NAVIT_KEY_RETURN:
    case NAVIT_KEY_MENU:
        p.x=w/2;
        p.y=h/2;
        this_->guiProxy->switchToMenu(&p);
        break;
    }
    return;
}

static void gui_qml_window_closed(struct gui_priv *data) {
    struct gui_priv *this_=(struct gui_priv*) data;
    this_->navitProxy->quit();
}
//GUI interface calls
static int argc=1;
static char *argv[]= {(char *)"navit",NULL};

static int gui_qml_set_graphics(struct gui_priv *this_, struct graphics *gra) {
    QString xid;
    NGQMainWindow* _mainWindow;
    bool ok;

    this_->gra=gra;

    //Check if we are already in Qt environment
    if (QApplication::instance()==NULL) {
        //Not yet
        this_->app=new QApplication(argc,argv);
    } else {
        this_->app=QApplication::instance();
    }

    //Link graphics events
    this_->button_cb=callback_new_attr_1(callback_cast(gui_qml_button), attr_button, this_);
    graphics_add_callback(gra, this_->button_cb);
    this_->motion_cb=callback_new_attr_1(callback_cast(gui_qml_motion), attr_motion, this_);
    graphics_add_callback(gra, this_->motion_cb);
    this_->resize_cb=callback_new_attr_1(callback_cast(gui_qml_resize), attr_resize, this_);
    graphics_add_callback(gra, this_->resize_cb);
    this_->keypress_cb=callback_new_attr_1(callback_cast(gui_qml_keypress), attr_keypress, this_);
    graphics_add_callback(gra, this_->keypress_cb);
    this_->window_closed_cb=callback_new_attr_1(callback_cast(gui_qml_window_closed), attr_window_closed, this_);
    graphics_add_callback(gra, this_->window_closed_cb);


    //Create main window
    this_->switcherWidget = new QStackedLayout();
    _mainWindow = new NGQMainWindow(this_, NULL);
#ifdef Q_WS_X11
    xid=getenv("NAVIT_XID");
    if (xid.length()>0) {
        _mainWindow->embedInto(xid.toULong(&ok,0));
    } else {
        dbg(lvl_error, "FATAL: Environment variable NAVIT_XID not set."
            "       Please set NAVIT_XID to the window ID of the window to embed into.\n");
        exit(1);
    }
#endif /* Q_WS_X11  */
    this_->mainWindow=_mainWindow;
    if ( this_->w && this_->h ) {
        this_->mainWindow->resize(this_->w,this_->h);
    }
    if ( this_->fullscreen ) {
        this_->mainWindow->showFullScreen();
    }

    this_->mainWindow->setLayout(this_->switcherWidget);

    //Create proxy object and bind them to gui widget
    this_->guiProxy = new NGQProxyGui(this_,this_->mainWindow);
    this_->navitProxy = new NGQProxyNavit(this_,this_->mainWindow);
    this_->vehicleProxy = new NGQProxyVehicle(this_,this_->mainWindow);
    this_->searchProxy = new NGQProxySearch(this_,this_->mainWindow);
    this_->bookmarksProxy = new NGQProxyBookmarks(this_,this_->mainWindow);
    this_->routeProxy = new NGQProxyRoute(this_,this_->mainWindow);

    //Check, if we have compatible graphics
    this_->graphicsWidget = (QWidget*)graphics_get_data(gra,"qt_widget");
    if (this_->graphicsWidget == NULL ) {
        this_->graphicsWidget = new QLabel(QString("Sorry, current graphics type is incompatible with this gui."));
    }
    this_->switcherWidget->addWidget(this_->graphicsWidget);

    //Instantiate qml components
    this_->guiWidget = new QDeclarativeView(NULL);
    this_->guiWidget->setResizeMode(QDeclarativeView::SizeRootObjectToView);

    this_->guiWidget->rootContext()->setContextProperty("gui",this_->guiProxy);
    this_->guiWidget->rootContext()->setContextProperty("navit",this_->navitProxy);
    this_->guiWidget->rootContext()->setContextProperty("vehicle",this_->vehicleProxy);
    this_->guiWidget->rootContext()->setContextProperty("search",this_->searchProxy);
    this_->guiWidget->rootContext()->setContextProperty("bookmarks",this_->bookmarksProxy);
    this_->guiWidget->rootContext()->setContextProperty("route",this_->routeProxy);
    this_->guiWidget->rootContext()->setContextProperty("point",this_->currentPoint);

    QString mainQml = QString(this_->source)+"/"+this_->skin+"/main.qml";
    if (!QFile(mainQml).exists()) {
        dbg(lvl_error, "FATAL: QML file %s not found. Navit is not installed correctly.", mainQml.toAscii().constData());
        exit(1);
    }
    this_->guiWidget->setSource(QUrl::fromLocalFile(mainQml));
    this_->switcherWidget->addWidget(this_->guiWidget);

    //Switch to graphics
    navit_draw(this_->nav);
    this_->switcherWidget->setCurrentWidget(this_->graphicsWidget);

    this_->mainWindow->show();

    return 0;
}

static int gui_qml_get_attr(struct gui_priv *this_, enum attr_type type, struct attr *attr) {
    switch (type) {
    case attr_fullscreen:
        attr->u.num=this_->fullscreen;
        break;
    case attr_skin:
        attr->u.str=this_->skin;
        break;
    case attr_pitch:
        attr->u.num=this_->pitch;
        break;
    case attr_radius:
        attr->u.num=this_->radius;
        break;
    default:
        return 0;
    }
    attr->type=type;
    return 1;
}

static int gui_qml_set_attr(struct gui_priv *this_, struct attr *attr) {
    switch (attr->type) {
    case attr_fullscreen:
        if (!(this_->fullscreen) && (attr->u.num)) {
            this_->mainWindow->showFullScreen();
        }
        if ((this_->fullscreen) && !(attr->u.num)) {
            this_->mainWindow->showNormal();
        }
        this_->fullscreen=attr->u.num;
        return 1;
    case attr_pitch:
        this_->pitch=attr->u.num;
        return 1;
    case attr_radius:
        this_->radius=attr->u.num;
        return 1;
    default:
        dbg(lvl_error,"unknown attr: %s",attr_to_name(attr->type));
        return 1;
    }
}

struct gui_methods gui_qml_methods = {
    NULL,
    NULL,
    gui_qml_set_graphics,
    NULL,
    NULL,
    NULL,
    NULL,
    gui_qml_get_attr,
    NULL,
    gui_qml_set_attr,
};

static void gui_qml_command(struct gui_priv *this_, char *function, struct attr **in, struct attr ***out, int *valid) {
    this_->guiProxy->processCommand(function);
}

static struct command_table commands[] = {
    {"*",command_cast(gui_qml_command)},
};

static struct gui_priv * gui_qml_new(struct navit *nav, struct gui_methods *meth, struct attr **attrs,
                                     struct gui *gui) {
    struct gui_priv *this_;
    struct attr *attr;
    *meth=gui_qml_methods;
    this_=g_new0(struct gui_priv, 1);

    this_->nav=nav;
    this_->gui=gui;

    this_->self.type=attr_gui;
    this_->self.u.gui=gui;

    navit_ignore_graphics_events(this_->nav, 1);

    this_->fullscreen = 0; //NO by default
    if( (attr=attr_search(attrs,NULL,attr_fullscreen)))
        this_->fullscreen=attr->u.num;
    this_->menu_on_map_click = 1; //YES by default;
    if( (attr=attr_search(attrs,NULL,attr_menu_on_map_click)))
        this_->menu_on_map_click=attr->u.num;
    this_->signal_on_map_click = 0; //YES by default;
    if( (attr=attr_search(attrs,NULL,attr_signal_on_map_click)))
        this_->signal_on_map_click=attr->u.num;
    this_->radius = 10; //Default value
    if( (attr=attr_search(attrs,NULL,attr_radius)))
        this_->radius=attr->u.num;
    this_->pitch = 20; //Default value
    if( (attr=attr_search(attrs,NULL,attr_pitch)))
        this_->pitch=attr->u.num;
    this_->lazy = 1; //YES by default
    if( (attr=attr_search(attrs,NULL,attr_lazy)))
        this_->lazy=attr->u.num;
    this_->w=800; //Default value
    if( (attr=attr_search(attrs,NULL,attr_width)))
        this_->w=attr->u.num;
    this_->h=600; //Default value
    if( (attr=attr_search(attrs,NULL,attr_height)))
        this_->h=attr->u.num;
    if( (attr=attr_search(attrs,NULL,attr_source)))
        this_->source=attr->u.str;
    if( (attr=attr_search(attrs,NULL,attr_skin)))
        this_->skin=attr->u.str;
    if( (attr=attr_search(attrs,NULL,attr_icon_src)))
        this_->icon_src=attr->u.str;

    if ( this_->source==NULL ) {
        this_->source=g_strjoin(NULL,getenv("NAVIT_SHAREDIR"),"/gui/qml/skins",NULL);
    }
    if ( this_->skin==NULL ) {
        this_->skin=g_strdup("navit");
    }
    if ( this_->icon_src==NULL ) {
        this_->icon_src=g_strjoin(NULL,getenv("NAVIT_SHAREDIR"),"/icons/",NULL);
    }

    if ((attr=attr_search(attrs, NULL, attr_callback_list))) {
        command_add_table(attr->u.callback_list, commands, sizeof(commands)/sizeof(struct command_table), this_);
    }

    this_->cbl=callback_list_new();

    return this_;
}

void plugin_init(void) {
    plugin_register_category_gui("qml",gui_qml_new);
}

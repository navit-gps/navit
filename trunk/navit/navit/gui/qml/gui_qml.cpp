#include <glib.h>
#include <QtCore>
#include <QtGui>
#include <QtDeclarative>
#include <QtXml>
#ifdef HAVE_API_WIN32_BASE
#include <windows.h>
#endif
#include "config.h"
#include "plugin.h"
#include "item.h"
#include "attr.h"
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

//WORKAOUND for the c/c++ compatibility issues.
//range is defined inside of struct attr so it is invisible in c++
struct range {
	short min, max;
} range;

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
	struct QMainWindow *mainWindow;
	QWidget *graphicsWidget;
#if QT_VERSION < 0x040700
 	QmlView *guiWidget;
	QmlView *prevGuiWidget;
#else
	QDeclarativeView *guiWidget;
	QDeclarativeView *prevGuiWidget;
#endif
	QStackedWidget *switcherWidget;
	struct callback *button_cb;

	//Proxy objects
	class NGQProxyGui* guiProxy;
	class NGQProxyNavit* navitProxy;
	class NGQProxyVehicle* vehicleProxy;
	class NGQProxySearch* searchProxy;
	class NGQProxyBookmarks* bookmarksProxy;
	class NGQPoint* currentPoint;
};

#include "proxy.h"
#include "ngqpoint.h"
#include "searchProxy.h"
#include "bookmarksProxy.h"
#include "vehicleProxy.h"
#include "navitProxy.h"
#include "guiProxy.h"

//Main window class for resizeEvent handling
class NGQMainWindow : public QMainWindow {
public:
	NGQMainWindow(struct gui_priv* this_,QWidget *parent) : QMainWindow(parent) {
		this->object=this_;
	}
protected:
	virtual void resizeEvent(QResizeEvent *) {
		this->object->w=this->width();
		this->object->h=this->height();
		//YES, i KNOW about signal/slot thing
		this->object->guiProxy->setWidth(this->width());
		this->object->guiProxy->setHeight(this->height());
	}
private:
	struct gui_priv* object;
};

//Meta object
#include "gui_qml.moc"

static void gui_qml_dbus_signal(struct gui_priv *this_, struct point *p)
{
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

static void gui_qml_button(void *data, int pressed, int button, struct point *p)
{
	struct gui_priv *this_=(struct gui_priv*)data;

	// check whether the position of the mouse changed during press/release OR if it is the scrollwheel
	if (!navit_handle_button(this_->nav, pressed, button, p, NULL)) {
		dbg(1,"navit has handled button\n");
		return;
	}
	dbg(1,"enter %d %d\n", pressed, button);
	if (this_->signal_on_map_click) {
		gui_qml_dbus_signal(this_, p);
		return;
	}

	if ( button == 1 && this_->menu_on_map_click ) {
		if (!this_->lazy) {
			this_->guiProxy->setReturnSource(QString(""));
			this_->guiProxy->setPage("point.qml");
		}
		this_->guiProxy->setNewPoint(p,MapPoint);
		this_->guiWidget->setFocus(Qt::ActiveWindowFocusReason);
		this_->switcherWidget->setCurrentWidget(this_->guiWidget);
	}
}

//GUI interface calls
static int argc=1;
static char *argv[]={(char *)"navit",NULL};

static int gui_qml_set_graphics(struct gui_priv *this_, struct graphics *gra)
{
	struct window *win;
	struct transformation *trans=navit_get_trans(this_->nav);

	this_->gra=gra;

	//Check if we are already in Qt environment
	if (QApplication::instance()==NULL) {
	    //Not yet
	    this_->app=new QApplication(argc,argv);
	} else {
	    this_->app=QApplication::instance();
	}
	
	//Create main window
	this_->switcherWidget = new QStackedWidget(this_->mainWindow);
	this_->mainWindow = new NGQMainWindow(this_, NULL);
	if ( this_->w && this_->h ) {
	    this_->mainWindow->resize(this_->w,this_->h);
	}
	if ( this_->fullscreen ) {
	    this_->mainWindow->showFullScreen();
	}
	this_->mainWindow->setCentralWidget(this_->switcherWidget);
	
	//Create proxy object and bind them to gui widget
	this_->guiProxy = new NGQProxyGui(this_,this_->mainWindow);
	this_->navitProxy = new NGQProxyNavit(this_,this_->mainWindow);
	this_->vehicleProxy = new NGQProxyVehicle(this_,this_->mainWindow);
	this_->searchProxy = new NGQProxySearch(this_,this_->mainWindow);
	this_->bookmarksProxy = new NGQProxyBookmarks(this_,this_->mainWindow);
		
	//Check, if we have compatible graphics
	this_->graphicsWidget = (QWidget*)graphics_get_data(gra,"qt_widget");
	if (this_->graphicsWidget == NULL ) {
	    this_->graphicsWidget = new QLabel(QString("Sorry, current graphics type is incompatible with this gui."));
	}
        this_->switcherWidget->addWidget(this_->graphicsWidget);
	
	//Link graphics events
	this_->button_cb=callback_new_attr_1(callback_cast(gui_qml_button), attr_button, this_);
	graphics_add_callback(gra, this_->button_cb);

	//Instantiate qml components
	this_->guiProxy->setPage(QString("point.qml"));

	//Switch to graphics
	this_->switcherWidget->setCurrentWidget(this_->graphicsWidget);

	this_->mainWindow->show();

	return 0;
}

static int
gui_qml_get_attr(struct gui_priv *this_, enum attr_type type, struct attr *attr)
{
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

static int
gui_qml_set_attr(struct gui_priv *this_, struct attr *attr)
{
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
		dbg(0,"unknown attr: %s\n",attr_to_name(attr->type));
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

static void
gui_qml_command(struct gui_priv *this_, char *function, struct attr **in, struct attr ***out, int *valid) {
	struct attr **curr=in;
	struct attr *attr;
	this_->guiProxy->processCommand(function);
}

static struct command_table commands[] = {
	{"*",command_cast(gui_qml_command)},
};

static struct gui_priv * gui_qml_new(struct navit *nav, struct gui_methods *meth, struct attr **attrs, struct gui *gui)
{
	struct gui_priv *this_;
	struct attr *attr;
	*meth=gui_qml_methods;
	this_=g_new0(struct gui_priv, 1);

	this_->nav=nav;
	this_->gui=gui;

	this_->self.type=attr_gui;
	this_->self.u.gui=gui;	

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
		this_->icon_src=g_strjoin(NULL,getenv("NAVIT_SHAREDIR"),"/xpm/",NULL);
	}

	if ((attr=attr_search(attrs, NULL, attr_callback_list))) {
		command_add_table(attr->u.callback_list, commands, sizeof(commands)/sizeof(struct command_table), this_);
	}

	this_->cbl=callback_list_new();

	return this_;
}

void plugin_init(void) {
    plugin_register_gui_type("qml",gui_qml_new);
}

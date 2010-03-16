#include <glib.h>
#include <QtCore>
#include <QtGui>
#include <QtDeclarative>
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
	
	//Interface stuff
	struct callback_list *cbl;
	QCoreApplication *app;
	struct window *win;
	struct graphics *gra;
	struct QMainWindow *mainWindow;
	QWidget *graphicsWidget;
	QmlView *guiWidget;
	QStackedWidget *switcherWidget;
	struct callback *button_cb;

	//Proxy objects
	class NGQProxyGui* guiProxy;
	class NGQProxyNavit* navitProxy;
};

#include "proxy.h"

//Proxy classes
class NGQProxyGui : public NGQProxy {
    Q_OBJECT;

	Q_PROPERTY(QString iconPath READ iconPath CONSTANT);
	Q_PROPERTY(QString returnSource READ returnSource WRITE setReturnSource);

	Q_PROPERTY(QString localeName READ localeName CONSTANT);
	Q_PROPERTY(QString langName READ langName CONSTANT);
	Q_PROPERTY(QString ctryName READ ctryName CONSTANT);

public:
    NGQProxyGui(struct gui_priv* this_,QObject *parent) : NGQProxy(this_, parent) {
		this->source=QString("NoReturnTicket");
    }

public slots:
	void setPage(QString page) {
		this->object->guiWidget->setUrl(QUrl::fromLocalFile(QString(this->object->source)+"/"+this->object->skin+"/"+page));
		this->object->guiWidget->execute();
		this->object->guiWidget->show();
		dbg(0,"Page is: %s\n",page.toStdString().c_str());
	}
	void backToMap() {
        if (this->object->graphicsWidget) {
                this->object->switcherWidget->setCurrentWidget(this->object->graphicsWidget);
        }
    }

	//Properties
	QString iconPath() {
		return QString(this->object->icon_src);
	}
	QString returnSource() {
		return this->source;
	}
	void setReturnSource(QString returnSource) {
		this->source=returnSource;
	}

	//Locale properties
	QString localeName() {
		return QString()+"LANG="+getenv("LANG");
	}
	QString langName() {
#ifdef HAVE_API_WIN32_BASE
		wchar_t wstr[32];
		char str[32];

		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVLANGNAME, wstr, sizeof(wstr));
		WideCharToMultiByte(CP_ACP,0,wstr,-1,str,sizeof(str),NULL,NULL);
		return QString()+"LOCALE_SABBREVLANGNAME="+str;
#else
		return QString();
#endif
	}
	QString ctryName() {
#ifdef HAVE_API_WIN32_BASE
		wchar_t wstr[32];
		char str[32];

		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVCTRYNAME, wstr, sizeof(wstr));
		WideCharToMultiByte(CP_ACP,0,wstr,-1,str,sizeof(str),NULL,NULL);
		return QString()+"LOCALE_SABBREVCTRYNAME="+str;
#else
		return QString();
#endif
	}
protected:
	int getAttrFunc(enum attr_type type, struct attr* attr, struct attr_iter* iter) { return gui_get_attr(this->object->gui, type, attr, iter); }
	int setAttrFunc(struct attr* attr) {return gui_set_attr(this->object->gui,attr); }
private:
	QString source;
};

class NGQProxyNavit : public NGQProxy {
    Q_OBJECT;

public:
	NGQProxyNavit(struct gui_priv* object, QObject* parent) : NGQProxy(object,parent) { };

public slots:
	void quit() {
			struct attr navit;
			navit.type=attr_navit;
			navit.u.navit=this->object->nav;
			navit_destroy(navit.u.navit);
			event_main_loop_quit();
			this->object->mainWindow->close();
	}
	void setObjectByName(const QString& attr_name,const QString& attr_value) {
		if (attr_name=="layout") {
			navit_set_layout_by_name(this->object->nav,attr_value.toStdString().c_str());
		}
		return;
	}
	
protected:
	int getAttrFunc(enum attr_type type, struct attr* attr, struct attr_iter* iter) { return navit_get_attr(this->object->nav, type, attr, iter); }
	int setAttrFunc(struct attr* attr) {return navit_set_attr(this->object->nav,attr); }
	struct attr_iter* getIterFunc() { return navit_attr_iter_new(); };
	void dropIterFunc(struct attr_iter* iter) { navit_attr_iter_destroy(iter); };

private:

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
			graphics_displayitem_within_dist(display, di, p, this_->radius)) {
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
	struct QmlView *view;

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
	this_->mainWindow = new QMainWindow(NULL);
	if ( this_->w && this_->h ) {
	    this_->mainWindow->setFixedSize(this_->w,this_->h);
	}
	if ( this_->fullscreen ) {
	    this_->mainWindow->showFullScreen();
	}
	this_->mainWindow->setCentralWidget(this_->switcherWidget);
	
	//Create gui widget
	view = new QmlView(NULL);
	this_->guiWidget = view;
	
	//Create proxy object and bind them to gui widget
	this_->guiProxy = new NGQProxyGui(this_,this_->mainWindow);
	view->rootContext()->setContextProperty("gui",this_->guiProxy);
	this_->navitProxy = new NGQProxyNavit(this_,this_->mainWindow);
	view->rootContext()->setContextProperty("navit",this_->navitProxy);
		
	//Check, if we have compatible graphics
	this_->graphicsWidget = (QWidget*)graphics_get_data(gra,"qt_widget");
	if (this_->graphicsWidget == NULL ) {
	    this_->graphicsWidget = new QLabel(QString("Sorry, current graphics type is incompatible with this gui."));
	}
        this_->switcherWidget->addWidget(this_->graphicsWidget);
	
	//Switch to graphics
	this_->switcherWidget->setCurrentWidget(this_->graphicsWidget);

	//Link graphics events
	this_->button_cb=callback_new_attr_1(callback_cast(gui_qml_button), attr_button, this_);
	graphics_add_callback(gra, this_->button_cb);

	//Instantiate qml components
	this_->guiProxy->setPage(QString("main.qml"));
	this_->switcherWidget->addWidget(this_->guiWidget);

	this_->mainWindow->show();

	return 0;
}

static void gui_qml_disable_suspend(struct gui_priv *this_)
{
/*	if (this->win->disable_suspend)
		this->win->disable_suspend(this->win);*/
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
	gui_qml_disable_suspend,
	gui_qml_get_attr,
	NULL,
	gui_qml_set_attr,
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
	if( (attr=attr_search(attrs,NULL,attr_width)))
    	      this_->w=attr->u.num;
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

	this_->cbl=callback_list_new();

	return this_;
}

void plugin_init(void) {
    plugin_register_gui_type("qml",gui_qml_new);
}

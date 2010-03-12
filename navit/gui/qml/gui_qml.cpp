#include <glib.h>
#include <QtCore>
#include <QtGui>
#include <QtDeclarative>
#include "config.h"
#include "plugin.h"
#include "item.h"
#include "attr.h"
#include "gui.h"
#include "callback.h"
#include "debug.h"
#include "navit.h"
#include "point.h"
#include "graphics.h"
#include "event.h"

struct gui_priv {
	struct navit *nav;
	struct attr self;
	
	//configuration items
	int fullscreen;
	int w;
	int h;
	char *source;
	char *skin;
	char* icon_src;
	
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

//Proxy classes
class NGQProxyGui : public QObject {
    Q_OBJECT;

	Q_PROPERTY(QString iconPath READ iconPath CONSTANT);
	Q_PROPERTY(QString returnSource READ returnSource WRITE setReturnSource);

public:
    NGQProxyGui(struct gui_priv* this_,QObject *parent) : QObject(parent) {
        this->gui=this_;
		this->source=QString("NoReturnTicket");
    }

public slots:
	void setPage(QString page) {
		this->gui->guiWidget->setUrl(QUrl::fromLocalFile(QString(this->gui->source)+"/"+this->gui->skin+"/"+page));
		this->gui->guiWidget->execute();
		this->gui->guiWidget->show();
		dbg(0,"Page is: %s\n",page.toStdString().c_str());
	}
	void backToMap() {
        if (this->gui->graphicsWidget) {
                this->gui->switcherWidget->setCurrentWidget(this->gui->graphicsWidget);
        }
    }

	//Properties
	QString iconPath() {
		return QString(this->gui->icon_src);
	}
	QString returnSource() {
		return this->source;
	}
	void setReturnSource(QString returnSource) {
		this->source=returnSource;
	}

private:
    struct gui_priv* gui;
	QString source;
};

class NGQProxyNavit : public QObject {
    Q_OBJECT;

public:
    NGQProxyNavit(struct gui_priv* this_,QObject *parent) : QObject(parent) {
        this->gui=this_;
    }

public slots:
	void quit() {
			struct attr navit;
			navit.type=attr_navit;
			navit.u.navit=this->gui->nav;
			navit_destroy(navit.u.navit);
			event_main_loop_quit();
			this->gui->mainWindow->close();
	}
	//Attribute read/write
	QString getAttr(const QString &attr_name) {
		QString ret;
		struct attr attr;
		navit_get_attr(this->gui->nav, attr_from_name(attr_name.toStdString().c_str()), &attr, NULL);
		if (ATTR_IS_INT(attr.type)) {
			ret.setNum(attr.u.num);
		}
		if (ATTR_IS_DOUBLE(attr.type)) {
			ret.setNum(*attr.u.numd);
		}
		if (ATTR_IS_STRING(attr.type)) {
			ret=attr.u.str;
		}
		return ret;
	}
	void setAttr(const QString &attr_name, const QString &attr_string) {
			struct attr attr_value;
			double *helper;

			navit_get_attr(this->gui->nav, attr_from_name(attr_name.toStdString().c_str()), &attr_value, NULL);

			if (ATTR_IS_INT(attr_value.type)) {
				attr_value.u.num=attr_string.toInt();
			}
			if (ATTR_IS_DOUBLE(attr_value.type)) {
				helper = g_new0(double,1);
				*helper=attr_string.toDouble();
				attr_value.u.numd=helper;
			}
			if (ATTR_IS_STRING(attr_value.type)) {
				attr_value.u.str=(char*)attr_string.toStdString().c_str();
			}

			navit_set_attr(this->gui->nav, &attr_value);

			return;
	}
	
protected:

private:
    struct gui_priv* gui;

};

//Meta objects
#include "gui_qml.moc"

static void gui_qml_button(void *data, int pressed, int button, struct point *p)
{
	struct gui_priv *this_=(struct gui_priv*)data;

	dbg(1,"enter %d %d\n", pressed, button);
	if (pressed == 0) {
	    if ( button == 1 ) {
		if (this_->switcherWidget->currentWidget() == this_->graphicsWidget) {
		    this_->switcherWidget->setCurrentWidget(this_->guiWidget);
		} else {
		    this_->switcherWidget->setCurrentWidget(this_->graphicsWidget);
		}
	    }
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
	default:
		dbg(0,"%s\n",attr_to_name(attr->type));
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

	this_->self.type=attr_gui;
	this_->self.u.gui=gui;	

	if( (attr=attr_search(attrs,NULL,attr_fullscreen)))
    	      this_->fullscreen=attr->u.num;
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
	if ( this_->icon_src==NULL ) {
		this_->icon_src=g_strjoin(NULL,getenv("NAVIT_SHAREDIR"),"/xpm/",NULL);
	}

	this_->cbl=callback_list_new();

	return this_;
}

void plugin_init(void) {
    plugin_register_gui_type("qml",gui_qml_new);
}

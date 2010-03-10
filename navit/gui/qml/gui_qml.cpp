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

struct gui_priv {
	struct navit *nav;
	struct attr self;
	
	//configuration items
	int fullscreen;
	int w;
	int h;
	char *source;
	char *skin;
	
	//Interface stuff
	struct callback_list *cbl;
	QCoreApplication *app;
	struct window *win;
	struct graphics *gra;
	struct QMainWindow *mainWindow;
	QWidget *graphicsWidget;
	QWidget *guiWidget;
	QStackedWidget *switcherWidget;
	struct callback *button_cb;
};

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
	
	view->setUrl(QUrl::fromLocalFile(QString(this_->source)+"/"+this_->skin+"/main.qml"));
	view->execute();
	this_->switcherWidget->addWidget(this_->guiWidget);
	
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

	if ( this_->source==NULL ) {
	    this_->source=g_strjoin(NULL,getenv("NAVIT_SHAREDIR"),"/gui/qml/skins",NULL);
	}

	this_->cbl=callback_list_new();

	plugin_init();

	return this_;
}

void plugin_init(void) {
    plugin_register_gui_type("qml",gui_qml_new);
}

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
#include "vehicle.h"
#include "coord.h"
#include "transform.h"
#include "mapset.h"
#include "route.h"
#include "country.h"
#include "track.h"
#include "search.h"

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
	QmlView *guiWidget;
	QStackedWidget *switcherWidget;
	struct callback *button_cb;

	//Proxy objects
	class NGQProxyGui* guiProxy;
	class NGQProxyNavit* navitProxy;
	class NGQProxyVehicle* vehicleProxy;
	class NGQProxySearch* searchProxy;
	class NGQPoint* currentPoint;
};

#include "proxy.h"
#include "ngqpoint.h"

//Proxy classes
class NGQProxySearch : public NGQProxy {
	Q_OBJECT;

	Q_PROPERTY(QString countryName READ countryName WRITE setCountryName NOTIFY countryNameSignal);
	Q_PROPERTY(QString countryISO2 READ countryISO2 WRITE setCountryISO2 NOTIFY countryISO2Signal);
	Q_PROPERTY(QString townName READ townName WRITE setTownName NOTIFY townNameSignal);

	Q_PROPERTY(QString searchContext READ searchContext WRITE setSearchContext);

public:
	NGQProxySearch(struct gui_priv* this_,QObject* parent) : NGQProxy(this_,parent) {
		struct attr search_attr, country_name, country_iso2, *country_attr;
		struct item *item;
		struct country_search *cs;
		struct tracking *tracking;
		struct search_list_result *res;

		this->sl=search_list_new(navit_get_mapset(this->object->nav));
		this->search_context="country";
		this->town_name="Select a town";

		country_attr=country_default();
		tracking=navit_get_tracking(this->object->nav);
		if (tracking && tracking_get_attr(tracking, attr_country_id, &search_attr, NULL))
			country_attr=&search_attr;
		if (country_attr) {
			cs=country_search_new(country_attr, 0);
			item=country_search_get_item(cs);
			if (item && item_attr_get(item, attr_country_name, &country_name)) {
				search_attr.type=attr_country_all;
				dbg(0,"country %s\n", country_name.u.str);
				this->country_name=QString::fromLocal8Bit(country_name.u.str);
				search_attr.u.str=country_name.u.str;
				search_list_search(this->sl, &search_attr, 0);
				while((res=search_list_get_result(this->sl)));
				if (item_attr_get(item, attr_country_iso2, &country_iso2)) {
					this->country_iso2=QString::fromLocal8Bit(country_iso2.u.str);
				}
			}
			country_search_destroy(cs);
		} else {
			dbg(0,"warning: no default country found\n");
			if (!this->country_iso2.isEmpty()) {
				dbg(0,"attempting to use country '%s'\n",this->country_iso2.toStdString().c_str());
				search_attr.type=attr_country_iso2;
				search_attr.u.str=(char*)this->country_iso2.toStdString().c_str();
				search_list_search(this->sl, &search_attr, 0);
				while((res=search_list_get_result(this->sl)));
			}
		}		
	}
	~NGQProxySearch() {
		search_list_destroy(this->sl);
	}

signals:
	void countryNameSignal(QString);
	void countryISO2Signal(QString);
	void townNameSignal(QString);

public slots:
	QString getAttrList(const QString &attr_name) {
		NGQStandardItemModel* ret=new NGQStandardItemModel(this);
		struct attr attr;
		struct search_list_result *res;
		int counter=0;
		QString currentValue, retId;

		if (this->search_context=="country") {
			currentValue=this->country_name;
			attr.type=attr_country_name;
			attr.u.str="";
		}
		if (this->search_context=="town") {
			currentValue=this->town_name;
			attr.type=attr_town_or_district_name;
			attr.u.str="";
		}
		if (this->search_context=="street") {
			currentValue=this->street_name;
			attr.type=attr_street_name;
			attr.u.str="";
		}

		search_list_search(this->sl,&attr,1);

		while ((res=search_list_get_result(this->sl))) {
			QStandardItem* curItem=new QStandardItem();
			//Result processing depends on search type
			if (this->search_context=="country") {
				curItem->setData(QVariant(counter),NGQStandardItemModel::ItemId);
				curItem->setData(QString::fromLocal8Bit(res->country->name),NGQStandardItemModel::ItemName);
				curItem->setData(QString("country_%1%2").arg(res->country->iso2).arg(".svgz"),NGQStandardItemModel::ItemIcon);
				if (this->country_name==QString::fromLocal8Bit(res->country->name)) {
					retId.setNum(counter);
				}
			}
			if (this->search_context=="town") {
				curItem->setData(QVariant(counter),NGQStandardItemModel::ItemId);
				if (res->town->common.town_name) {
					curItem->setData(QString::fromLocal8Bit(res->town->common.town_name),NGQStandardItemModel::ItemName);
				}
				if (res->town->common.district_name) {
					curItem->setData(QString::fromLocal8Bit(res->town->common.district_name),NGQStandardItemModel::ItemName);
				} 
				if (this->town_name==QString::fromLocal8Bit(res->town->common.town_name) || this->town_name==QString::fromLocal8Bit(res->town->common.district_name)) {
					retId.setNum(counter);
				}
			}
			if (this->search_context=="street") {
				curItem->setData(QVariant(counter),NGQStandardItemModel::ItemId);
				curItem->setData(QString::fromLocal8Bit(res->street->name),NGQStandardItemModel::ItemName);
				if (this->street_name==QString::fromLocal8Bit(res->street->name)) {
					retId.setNum(counter);
				}
			}
			counter++;
			ret->appendRow(curItem);
		}

		this->object->guiWidget->rootContext()->setContextProperty("listModel",static_cast<QObject*>(ret));

		return retId;
	}
	QString countryName() {
		return this->country_name;
	}
	void setCountryName(QString countryName) {
		this->country_name=countryName;
		struct attr attr;
		struct search_list_result *res;
		
		//We need to update ISO2 
		attr.type=attr_country_name;
		attr.u.str=countryName.toLocal8Bit().data();
		search_list_search(this->sl,&attr,0);
		while ((res=search_list_get_result(this->sl))) {
				this->setCountryISO2(QString::fromLocal8Bit(res->country->iso2));
		}
		//...and current town
		this->town_name="Select a town";

		countryNameSignal(countryName);
	}
	QString countryISO2() {
		return this->country_iso2;
	}
	void setCountryISO2(QString countryISO2) {
		this->country_iso2=countryISO2;
		countryISO2Signal(countryISO2);
	}
	QString townName() {
		return this->town_name;
	}
	void setTownName(QString townName) {
		struct attr attr;
		struct search_list_result *res;

		this->town_name=townName;

		//Specialize search
		attr.type=attr_town_or_district_name;
		attr.u.str=townName.toLocal8Bit().data();
		search_list_search(this->sl,&attr,0);

		townNameSignal(townName);
	}
	QString searchContext() {
		return this->search_context;
	}
	void setSearchContext(QString searchContext) {
		this->search_context=searchContext;
	}

protected:
	virtual int getAttrFunc(enum attr_type type, struct attr *attr, struct attr_iter *iter) {
		return 0;
	}
	virtual int setAttrFunc(struct attr *attr) {
		return 0;
	}
private:
	struct search_list *sl;
	QString search_context;
	QString country_iso2,country_name,town_name,street_name;
};

class NGQProxyGui : public NGQProxy {
    Q_OBJECT;

	Q_PROPERTY(QString iconPath READ iconPath CONSTANT);
	Q_PROPERTY(QString returnSource READ returnSource WRITE setReturnSource);

	Q_PROPERTY(QString localeName READ localeName CONSTANT);
	Q_PROPERTY(QString langName READ langName CONSTANT);
	Q_PROPERTY(QString ctryName READ ctryName CONSTANT);

	Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthSignal STORED false);
	Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightSignal STORED false);

public:
    NGQProxyGui(struct gui_priv* this_,QObject *parent) : NGQProxy(this_, parent) {
		this->source=QString("");
    }

	void setNewPoint(struct point* p,NGQPointTypes type) {
		if (this->object->currentPoint!=NULL) {
			delete this->object->currentPoint;
		}
		this->object->currentPoint = new NGQPoint(this->object,p,type,NULL);
		this->object->guiWidget->rootContext()->setContextProperty("point",this->object->currentPoint);
	}
signals:
	void widthSignal(int);
	void heightSignal(int);
public slots:
	void setPage(QString page) {
		dbg(0,"Page is: %s\n",page.toStdString().c_str());
		this->source+="/"+page;
		this->object->guiWidget->setUrl(QUrl::fromLocalFile(QString(this->object->source)+"/"+this->object->skin+"/"+page));
		this->object->guiWidget->execute();
		this->object->guiWidget->show();
	}
	void backToMap() {
        if (this->object->graphicsWidget) {
				this_->graphicsWidget->setFocus(Qt::ActiveWindowFocusReason);
                this->object->switcherWidget->setCurrentWidget(this->object->graphicsWidget);
        }
    }
	void backToPrevPage() {
		QStringList returnList=this->source.split(QString("/"), QString::SkipEmptyParts);
		QString returnPage;
		if (returnList.size()>1) {
			returnList.takeLast();//Remove current element
			returnPage=returnList.takeLast(); //Set previous element as return page and remove it from the list
		}
		this->source=returnList.join(QString("/"));
		this->source.prepend(QString("/"));
		this->setPage(returnPage);
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
	int width() {
		return this->object->w;
	}
	void setWidth(int w) {
		this->object->w=w;
		this->widthSignal(w);
	}
	int height() {
		return this->object->h;
	}
	void setHeight(int h) {
		this->object->h=h;
		this->heightSignal(h);
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


class NGQProxyVehicle : public NGQProxy {
    Q_OBJECT;

public:
	NGQProxyVehicle(struct gui_priv* object, QObject* parent) : NGQProxy(object,parent) { };

public slots:

protected:
	int getAttrFunc(enum attr_type type, struct attr* attr, struct attr_iter* iter) { return vehicle_get_attr(this->object->currVehicle, type, attr, iter); }
	int setAttrFunc(struct attr* attr) {return vehicle_set_attr(this->object->currVehicle,attr); }
	struct attr_iter* getIterFunc() { return vehicle_attr_iter_new(); };
	void dropIterFunc(struct attr_iter* iter) { vehicle_attr_iter_destroy(iter); };

private:

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
		if (attr_name=="vehicle") {
			navit_set_vehicle_by_name(this->object->nav,attr_value.toStdString().c_str());
		}
		return;
	}
	QString getAttrList(const QString &attr_name) {
		NGQStandardItemModel* ret=new NGQStandardItemModel(this);
		struct attr attr;
		struct attr_iter *iter;
		int counter=0;
		QString currentValue, retId;

		//Find current value
		getAttrFunc(attr_from_name(attr_name.toStdString().c_str()), &attr, NULL) ;
		if (attr.type==attr_layout) {
			currentValue=attr.u.layout->name;
		}

		//Fill da list
		iter=getIterFunc();
		if (iter == NULL) {
			return retId;
		}

		while (getAttrFunc(attr_from_name(attr_name.toStdString().c_str()), &attr, iter) ) {
			QStandardItem* curItem=new QStandardItem();
			//Listed attributes are usualy have very complex structure	
			if (attr.type==attr_layout) {
				curItem->setData(QVariant(counter),NGQStandardItemModel::ItemId);
				curItem->setData(QVariant(attr.u.layout->name),NGQStandardItemModel::ItemName);
				if (currentValue==attr.u.layout->name) {
					retId.setNum(counter);
				}
			}
			if (attr.type==attr_vehicle) {
				this->object->currVehicle=attr.u.vehicle;
				curItem->setData(QVariant(counter),NGQStandardItemModel::ItemId);
				curItem->setData(QVariant(this->object->vehicleProxy->getAttr("name")),NGQStandardItemModel::ItemName);
				retId.setNum(0);
			}
			counter++;
			ret->appendRow(curItem);
		}

		dropIterFunc(iter);

		this->object->guiWidget->rootContext()->setContextProperty("listModel",static_cast<QObject*>(ret));

		dbg(0,"retId %s \n",retId.toStdString().c_str());

		return retId;
	}
	void setDestination() {
		navit_set_destination(this->object->nav,this->object->currentPoint->pc(),this->object->currentPoint->coordString().toStdString().c_str(),1);
	}
	void setPosition() {
		navit_set_position(this->object->nav,this->object->currentPoint->pc());
	}
protected:
	int getAttrFunc(enum attr_type type, struct attr* attr, struct attr_iter* iter) { return navit_get_attr(this->object->nav, type, attr, iter); }
	int setAttrFunc(struct attr* attr) {return navit_set_attr(this->object->nav,attr); }
	struct attr_iter* getIterFunc() { return navit_attr_iter_new(); };
	void dropIterFunc(struct attr_iter* iter) { navit_attr_iter_destroy(iter); };

private:

};

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

	/* There is a special 'popup' feature in navit, that makes all 'click-on-point' related stuff
	   but it looks VERY unflexible, so i'm not able to use it. I believe we need
	   to re-design the popup feature or remove it at all */
	if ( button == 3 ) {
		this_->guiProxy->setNewPoint(p,MapPoint);
		this_->guiWidget->setFocus(Qt::ActiveWindowFocusReason);
		this_->guiWidget->reset();
		this_->guiProxy->setReturnSource(QString(""));
		this_->guiProxy->setPage("point.qml");
		this_->switcherWidget->setCurrentWidget(this_->guiWidget);
	}

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
			this_->guiWidget->clearItems();
			this_->guiProxy->setReturnSource(QString(""));
			this_->guiProxy->setPage("main.qml");
		}
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
	    this_->mainWindow->setFixedSize(this_->w,this_->h);
	}
	if ( this_->fullscreen ) {
	    this_->mainWindow->showFullScreen();
	}
	this_->mainWindow->setCentralWidget(this_->switcherWidget);
	
	//Create gui widget
	this_->guiWidget = new QmlView(NULL);
	this_->guiWidget->setContentResizable(true);
	
	//Create proxy object and bind them to gui widget
	this_->guiProxy = new NGQProxyGui(this_,this_->mainWindow);
	this_->guiWidget->rootContext()->setContextProperty("gui",this_->guiProxy);
	this_->navitProxy = new NGQProxyNavit(this_,this_->mainWindow);
	this_->guiWidget->rootContext()->setContextProperty("navit",this_->navitProxy);
	this_->vehicleProxy = new NGQProxyVehicle(this_,this_->mainWindow);
	this_->guiWidget->rootContext()->setContextProperty("vehicle",this_->vehicleProxy);
	this_->searchProxy = new NGQProxySearch(this_,this_->mainWindow);
	this_->guiWidget->rootContext()->setContextProperty("search",this_->searchProxy);
		
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

	this_->cbl=callback_list_new();

	return this_;
}

void plugin_init(void) {
    plugin_register_gui_type("qml",gui_qml_new);
}

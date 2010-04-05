#ifndef NAVIT_GUI_QML_GUIPROXY_H
#define NAVIT_GUI_QML_GUIPROXY_H

class NGQProxyGui : public NGQProxy {
    Q_OBJECT;

	Q_PROPERTY(QString iconPath READ iconPath CONSTANT);
	Q_PROPERTY(QString returnSource READ returnSource WRITE setReturnSource);

	Q_PROPERTY(QString commandFunction READ commandFunction CONSTANT);

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
	void setNewPoint(struct coord* c,NGQPointTypes type) {
		if (this->object->currentPoint!=NULL) {
			delete this->object->currentPoint;
		}
		this->object->currentPoint = new NGQPoint(this->object,c,type,NULL);
		this->object->guiWidget->rootContext()->setContextProperty("point",this->object->currentPoint);
	}
	void processCommand(QString function) {
		this->function=function;
		this->setPage("command.qml",true);
	}
signals:
	void widthSignal(int);
	void heightSignal(int);
public slots:
	void setPage(QString page,bool hidden=false) {
		dbg(0,"Page is: %s\n",page.toStdString().c_str());
		this->source+="/"+page;

#if QT_VERSION < 0x040700
    if (this->object->guiWidget==NULL) {
	   this->object->guiWidget = new QmlView(NULL);
       this->object->guiWidget->setContentResizable(true);
    }
#else
	//Reload widget
	if (this->object->guiWidget) {
		this->object->switcherWidget->removeWidget(this->object->guiWidget);
		if (this->object->prevGuiWidget) {
			delete this->object->prevGuiWidget;
		}
		this->object->prevGuiWidget=this->object->guiWidget;
	}
	this->object->guiWidget = new QDeclarativeView(NULL);
	this->object->guiWidget->setResizeMode(QDeclarativeView::SizeRootObjectToView);
#endif

	this->object->guiWidget->rootContext()->setContextProperty("gui",this->object->guiProxy);
	this->object->guiWidget->rootContext()->setContextProperty("navit",this->object->navitProxy);
	this->object->guiWidget->rootContext()->setContextProperty("vehicle",this->object->vehicleProxy);
	this->object->guiWidget->rootContext()->setContextProperty("search",this->object->searchProxy);
	this->object->guiWidget->rootContext()->setContextProperty("bookmarks",this->object->bookmarksProxy);
	this->object->guiWidget->rootContext()->setContextProperty("point",this->object->currentPoint);

#if QT_VERSION < 0x040700
 		this->object->guiWidget->setUrl(QUrl::fromLocalFile(QString(this->object->source)+"/"+this->object->skin+"/"+page));
 		this->object->guiWidget->execute();
#else
		this->object->guiWidget->setSource(QUrl::fromLocalFile(QString(this->object->source)+"/"+this->object->skin+"/"+page));
#endif
		if (!hidden) {
			//we render commands page hidden, so the screen doesn't flicks.
			this->object->guiWidget->show();
			this->object->switcherWidget->addWidget(this->object->guiWidget);
			this->object->guiWidget->setFocus(Qt::ActiveWindowFocusReason);
			this->object->switcherWidget->setCurrentWidget(this->object->guiWidget);
		}
	}
	void backToMap() {
        if (this->object->graphicsWidget) {
				this->object->graphicsWidget->setFocus(Qt::ActiveWindowFocusReason);
				this->object->switcherWidget->setCurrentWidget(this->object->graphicsWidget);
				this->object->graphicsWidget->show();
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
	QString commandFunction() {
		return this->function;
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
	QString function;
};

void __setNewPoint(struct gui_priv* this_,struct coord* c, NGQPointTypes type) {
	this_->guiProxy->setNewPoint(c,type);
}

#include "guiProxy.moc"

#endif /* NAVIT_GUI_QML_GUIPROXY_H */

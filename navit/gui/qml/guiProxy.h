#ifndef NAVIT_GUI_QML_GUIPROXY_H
#define NAVIT_GUI_QML_GUIPROXY_H

class NGQProxyGui : public NGQProxy {
    Q_OBJECT;

	Q_PROPERTY(QString iconPath READ iconPath CONSTANT);

	Q_PROPERTY(QString commandFunction READ commandFunction CONSTANT);

	Q_PROPERTY(QString localeName READ localeName CONSTANT);
	Q_PROPERTY(QString langName READ langName CONSTANT);
	Q_PROPERTY(QString ctryName READ ctryName CONSTANT);

	Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthSignal STORED false);
	Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightSignal STORED false);

private:
	QStringList returnPath;

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
	void setNewPoint(struct pcoord* pc,NGQPointTypes type) {
		if (this->object->currentPoint!=NULL) {
			delete this->object->currentPoint;
		}
		this->object->currentPoint = new NGQPoint(this->object,pc,type,NULL);
		this->object->guiWidget->rootContext()->setContextProperty("point",this->object->currentPoint);
	}
	void processCommand(QString function) {
		//QDeclarativeExpression commandJS(this->object->guiWidget->rootContext(),QString(),qobject_cast<QObject*>(this->object->guiWidget->rootObject()));
		//commandJS.setSourceLocation("command.js",0);
		//this->function=function;
		//commandJS.eval(qobject_cast<QObject*>(this->object->guiWidget->rootObject()));
	}
signals:
	void widthSignal(int);
	void heightSignal(int);
public slots:
	void pushPage(QString page) {
		returnPath.push_front(page);
	}
	QString popPage() {
		if (!returnPath.empty()) {
			if (returnPath.length()>1) {
				returnPath.pop_front();
			}
			return returnPath.first();
		}
		return QString();
	}
	int lengthPage() {
		return returnPath.length();
	}
	void backToMap() {
        if (this->object->graphicsWidget) {
				this->object->graphicsWidget->setFocus(Qt::ActiveWindowFocusReason);
				this->object->switcherWidget->setCurrentWidget(this->object->graphicsWidget);
				this->object->graphicsWidget->show();
        }
    }
	void switchToMenu(struct point* p) {
		if (!this->object->lazy) {
			this->returnPath.clear();
			this->object->guiWidget->setSource(QUrl::fromLocalFile(QString(this->object->source)+"/"+this->object->skin+"/main.qml"));
		}
		this->setNewPoint(p,MapPoint);
		this->object->guiWidget->setFocus(Qt::ActiveWindowFocusReason);
		this->object->switcherWidget->setCurrentWidget(this->object->guiWidget);

	}
	//Properties
	QString iconPath() {
		return QString(this->object->icon_src);
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
		char str[32];

		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVLANGNAME, str, sizeof(str));
		return QString()+"LOCALE_SABBREVLANGNAME="+str;
#else
		return QString();
#endif
	}
	QString ctryName() {
#ifdef HAVE_API_WIN32_BASE
		char str[32];

		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVCTRYNAME, str, sizeof(str));
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
void __setNewPoint(struct gui_priv* this_,struct pcoord* pc, NGQPointTypes type) {
	this_->guiProxy->setNewPoint(pc,type);
}

#include "guiProxy.moc"

#endif /* NAVIT_GUI_QML_GUIPROXY_H */

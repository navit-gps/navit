#ifndef NAVIT_GUI_QML_ROUTEPROXY_H
#define NAVIT_GUI_QML_ROUTEPROXY_H


class NGQProxyRoute : public NGQProxy {
    Q_OBJECT;

public:
    NGQProxyRoute(struct gui_priv* this_,QObject* parent) : NGQProxy(this_,parent) { };

public slots:
	void addDestination() {
		int counter=0;
		QList<struct attr> destinations=this->_routeDestinations();
		struct pcoord *coords=(struct pcoord*)malloc(sizeof(struct pcoord)*(destinations.size()+1)); //Additional destination included

		for (QList<struct attr>::const_iterator iter=destinations.begin();iter!=destinations.end();iter++) {
			coords[counter]=*(iter->u.pcoord);
			counter++;
		}

		//Add new one
		coords[counter]=*(this->object->currentPoint->pc());

		//Propagate to route engine
		route_set_destinations(navit_get_route(this->object->nav),coords,counter+1,1);
	}
    QString getDestinations() {

		QList<struct attr> destinations=this->_routeDestinations();
		for (QList<struct attr>::const_iterator iter=destinations.begin();iter!=destinations.end();iter++) {
			NGQPoint helperPoint(this->object,iter->u.pcoord,MapPoint);
			dbg(lvl_debug,"Added destination %s",helperPoint.coordString().toLocal8Bit().constData());
		}

		//dbg(lvl_debug,QString::number(_itemId).toStdString().c_str());

		//return retDoc.toString();
		return QString();
	}

protected:
    int getAttrFunc(enum attr_type type, struct attr* attr, struct attr_iter* iter) {return route_get_attr(navit_get_route(this->object->nav), type, attr, iter); }
	int setAttrFunc(struct attr* attr) {return route_set_attr(navit_get_route(this->object->nav),attr); }
    struct attr_iter* getIterFunc() { return route_attr_iter_new(); };
	void dropIterFunc(struct attr_iter* iter) { route_attr_iter_destroy(iter); };

	QList<struct attr> _routeDestinations() {
		QList<struct attr> ret;
		struct attr attr;
		struct attr_iter *iter;

		//Fill da list
		iter=getIterFunc();
		if (iter == NULL) {
			return ret;
		}

		while (getAttrFunc(attr_destination, &attr, iter) ) {
			struct attr c_attr=attr;
			ret.push_front(c_attr); //List is reversed in route engine
		}

		dropIterFunc(iter);

		return ret;
	}
};

#include "routeProxy.moc"

#endif /* NAVIT_GUI_QML_ROUTEPROXY_H */

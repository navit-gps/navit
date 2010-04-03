#ifndef NAVIT_GUI_QML_NAVIT_H
#define NAVIT_GUI_QML_NAVIT_H

void __setNewPoint(struct gui_priv* this_,struct coord* c, NGQPointTypes type);

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
				curItem->setData(QVariant(attr.u.layout->name),NGQStandardItemModel::ItemValue);
				if (currentValue==attr.u.layout->name) {
					retId.setNum(counter);
				}
			}
			if (attr.type==attr_vehicle) {
				this->object->currVehicle=attr.u.vehicle;
				curItem->setData(QVariant(counter),NGQStandardItemModel::ItemId);
				curItem->setData(QVariant(this->object->vehicleProxy->getAttr("name")),NGQStandardItemModel::ItemName);
				curItem->setData(QVariant(this->object->vehicleProxy->getAttr("name")),NGQStandardItemModel::ItemValue);
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
	QString getDestination() {
		struct attr attr;
		struct coord c;

		if (getAttrFunc(attr_destination, &attr, NULL) ) {
			c.x=attr.u.pcoord->x;
			c.y=attr.u.pcoord->y;
			__setNewPoint(this->object,&c,Destination);
			return this->object->currentPoint->pointName();
		}
		return QString();
	}
	void setDestination() {
		navit_set_destination(this->object->nav,this->object->currentPoint->pc(),this->object->currentPoint->coordString().toStdString().c_str(),1);
	}
	QString getPosition() {
		struct attr attr;
		struct pcoord pc;
		struct coord c;
		struct transformation *trans;

		trans=navit_get_trans(this->object->nav);

		getAttrFunc(attr_vehicle, &attr, NULL);
		this->object->currVehicle=attr.u.vehicle;

		if (vehicle_get_attr(this->object->currVehicle, attr_position_coord_geo, &attr, NULL)) {
		   pc.pro=transform_get_projection(trans);
		   transform_from_geo(pc.pro, attr.u.coord_geo, &c);
		   __setNewPoint(this->object,&c,Position);
		   return this->object->currentPoint->pointName();
		}
		return QString();
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

#include "navitProxy.moc"

#endif /* NAVIT_GUI_QML_NAVITPROXY_H */

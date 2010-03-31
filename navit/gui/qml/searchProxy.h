#ifndef NAVIT_GUI_QML_SEARCHPROXY_H
#define NAVIT_GUI_QML_SEARCHPROXY_H

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

#include "searchProxy.moc"

#endif /* NAVIT_GUI_QML_SEARCHPROXY_H */

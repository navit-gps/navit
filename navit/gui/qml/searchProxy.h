#ifndef NAVIT_GUI_QML_SEARCHPROXY_H
#define NAVIT_GUI_QML_SEARCHPROXY_H

void __setNewPoint(struct gui_priv* this_,struct pcoord* pc, NGQPointTypes type);

class NGQProxySearch : public NGQProxy {
	Q_OBJECT;

	Q_PROPERTY(QString countryName READ countryName WRITE setCountryName NOTIFY countryNameSignal);
	Q_PROPERTY(QString countryISO2 READ countryISO2 WRITE setCountryISO2 NOTIFY countryISO2Signal);
	Q_PROPERTY(QString townName READ townName WRITE setTownName NOTIFY townNameSignal);
	Q_PROPERTY(QString streetName READ streetName WRITE setStreetName NOTIFY streetNameSignal);

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

		country_attr=country_default();
		tracking=navit_get_tracking(this->object->nav);
		if (tracking && tracking_get_attr(tracking, attr_country_id, &search_attr, NULL))
			country_attr=&search_attr;
		if (country_attr) {
			cs=country_search_new(country_attr, 0);
			item=country_search_get_item(cs);
			if (item && item_attr_get(item, attr_country_name, &country_name)) {
				search_attr.type=attr_country_all;
				dbg(lvl_debug,"country %s", country_name.u.str);
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
			dbg(lvl_error,"warning: no default country found");
			if (!this->country_iso2.isEmpty()) {
				dbg(lvl_debug,"attempting to use country '%s'",this->country_iso2.toStdString().c_str());
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
	void streetNameSignal(QString);

public slots:
	void setPointToResult() {
		struct attr attr;
		struct search_list_result *res;

		if (this->street_name.length()>0) {
			attr.type=attr_street_name;
			attr.u.str=this->street_name.toLocal8Bit().data();
		} else if (this->town_name.length()>0) {
			attr.type=attr_town_or_district_name;
			attr.u.str=this->town_name.toLocal8Bit().data();
		} else if (this->country_name.length()>0) {
			attr.type=attr_country_name;
			attr.u.str=this->country_name.toLocal8Bit().data();
		}
		search_list_search(this->sl,&attr,0);
		if ((res=search_list_get_result(this->sl))) {
			__setNewPoint(this->object,res->c,PointOfInterest);
		}
		return;
	}
	QString searchXml() {
		NGQStandardItemModel* ret=new NGQStandardItemModel(this);
		struct attr attr;
		struct search_list_result *res;
		int counter=0;
		QDomDocument retDoc;
		QDomElement entries;

		entries=retDoc.createElement("search");
		retDoc.appendChild(entries);

		if (this->search_context=="country") {
			attr.type=attr_country_name;
			attr.u.str=this->country_name.toLocal8Bit().data();
		}
		if (this->search_context=="town") {
			if (this->town_name.length()<3) {
				return retDoc.toString();
			}
			attr.type=attr_town_or_district_name;
			attr.u.str=this->town_name.toLocal8Bit().data();
		}
		if (this->search_context=="street") {
			attr.type=attr_street_name;
			attr.u.str=this->street_name.toLocal8Bit().data();
		}

		search_list_search(this->sl,&attr,1);

		while ((res=search_list_get_result(this->sl))) {
			QStandardItem* curItem=new QStandardItem();
			QDomElement entry=retDoc.createElement("item");
			entries.appendChild(entry);
			//Result processing depends on search type
			if (this->search_context=="country") {
					entry.appendChild(this->_fieldValueHelper(retDoc,QString("id"), QString::number(counter)));
					entry.appendChild(this->_fieldValueHelper(retDoc,QString("name"), QString::fromLocal8Bit(res->country->name)));
					entry.appendChild(this->_fieldValueHelper(retDoc,QString("icon"), QString("country_%1%2").arg(res->country->iso2).arg(".svgz")));
			}
			if (this->search_context=="town") {
				entry.appendChild(this->_fieldValueHelper(retDoc,QString("id"), QString::number(counter)));
				if (res->town->common.town_name) {
					entry.appendChild(this->_fieldValueHelper(retDoc,QString("name"),QString::fromLocal8Bit(res->town->common.town_name)));
				}
				if (res->town->common.district_name) {
					entry.appendChild(this->_fieldValueHelper(retDoc,QString("name"), QString::fromLocal8Bit(res->town->common.district_name)));
				}
			}
			if (this->search_context=="street") {
				entry.appendChild(this->_fieldValueHelper(retDoc,QString("id"), QString::number(counter)));
				entry.appendChild(this->_fieldValueHelper(retDoc,QString("name"),QString::fromLocal8Bit(res->street->name)));
			}
			counter++;
			ret->appendRow(curItem);
		}

		return retDoc.toString();
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
		this->town_name="";
		this->street_name="";

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

		this->town_name=townName;

		//Specialize search
		attr.type=attr_town_or_district_name;
		attr.u.str=townName.toLocal8Bit().data();
		search_list_search(this->sl,&attr,0);

		//...and street
		this->street_name="";

		townNameSignal(townName);
	}
	QString streetName() {
		return this->street_name;
	}
	void setStreetName(QString streetName) {
		struct attr attr;

		this->street_name=streetName;

		//Specialize search
		attr.type=attr_street_name;
		attr.u.str=streetName.toLocal8Bit().data();
		search_list_search(this->sl,&attr,0);

		streetNameSignal(streetName);
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

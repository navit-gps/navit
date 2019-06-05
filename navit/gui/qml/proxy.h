#ifndef NAVIT_GUI_QML_PROXY_H
#define NAVIT_GUI_QML_PROXY_H

class NGQStandardItemModel  : public QStandardItemModel {
public:
	NGQStandardItemModel(QObject* parent) : QStandardItemModel(parent) {
		//Populate role list
		roleNames.insert(NGQStandardItemModel::ItemId, "itemId");
		roleNames.insert(NGQStandardItemModel::ItemName, "itemName");
		roleNames.insert(NGQStandardItemModel::ItemIcon, "itemIcon");
		roleNames.insert(NGQStandardItemModel::ItemPath, "itemPath");
		roleNames.insert(NGQStandardItemModel::ItemValue, "itemValue");
		this->setRoleNames(roleNames);
	}

	enum listRoles {ItemId=Qt::UserRole+1,ItemName=Qt::UserRole+2,ItemIcon=Qt::UserRole+3,ItemPath=Qt::UserRole+4,ItemValue=Qt::UserRole+5};
private:
	QHash<int, QByteArray> roleNames;
};

class NGQProxy : public QObject {
	Q_OBJECT;

	Q_PROPERTY(int itemId READ itemId  NOTIFY itemIdSignal);
public:
    NGQProxy(struct gui_priv* this_,QObject *parent) : QObject(parent) {
        this->object=this_;
    }

signals:
	void itemIdSignal(int itemId);

public slots:
	//Attribute read/write
	QString getAttr(const QString &attr_name) {
		QString ret;
		struct attr attr;

		getAttrFunc(attr_from_name(attr_name.toStdString().c_str()), &attr, NULL);
		if (ATTR_IS_INT(attr.type)) {
			ret.setNum(attr.u.num);
		}
		if (ATTR_IS_DOUBLE(attr.type)) {
			ret.setNum(*attr.u.numd);
		}
		if (ATTR_IS_STRING(attr.type)) {
			ret=attr.u.str;
		}
		if (attr.type==attr_layout) {
			ret=attr.u.layout->name;
		}
		return ret;
	}
	void setAttr(const QString &attr_name, const QString &attr_string) {
			struct attr attr_value;
			double *helper;

			dbg(lvl_debug,"Setting %s to %s",attr_name.toStdString().c_str(),attr_string.toStdString().c_str());
			getAttrFunc(attr_from_name(attr_name.toStdString().c_str()), &attr_value, NULL);

			if (ATTR_IS_INT(attr_value.type)) {
				//Special handling for "true"/"false"
				if (attr_string=="true") {
					attr_value.u.num=1;
				} else if (attr_string=="false") {
					attr_value.u.num=0;
				} else {
					attr_value.u.num=attr_string.toInt();
				}
			}
			if (ATTR_IS_DOUBLE(attr_value.type)) {
				helper = g_new0(double,1);
				*helper=attr_string.toDouble();
				attr_value.u.numd=helper;
			}
			if (ATTR_IS_STRING(attr_value.type)) {
				attr_value.u.str=(char*)attr_string.toStdString().c_str();
			}

			setAttrFunc(&attr_value);

			return;
	}

	int itemId() {
		return _itemId;
	}
protected:
    struct gui_priv* object;

	int _itemId;

	virtual int setAttrFunc(struct attr *attr)=0;
	virtual int getAttrFunc(enum attr_type type, struct attr *attr, struct attr_iter *iter)=0;
	virtual struct attr_iter* getIterFunc() { return NULL; };
	virtual void dropIterFunc(struct attr_iter*) { return; };

	QDomElement _fieldValueHelper(QDomDocument doc, QString field,QString value) {
			QDomElement fieldTag=doc.createElement(field);
			QDomText valueText=doc.createTextNode(value);
			fieldTag.appendChild(valueText);
			return fieldTag;
	}

};

#include "proxy.moc"

#endif /* NAVIT_GUI_QML_PROXY_H */

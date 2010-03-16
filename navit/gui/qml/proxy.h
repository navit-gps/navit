#ifndef NAVIT_GUI_QML_PROXY_H
#define NAVIT_GUI_QML_PROXY_H

class NGQStandardItemModel  : public QStandardItemModel {
public:
	NGQStandardItemModel(QObject* parent) : QStandardItemModel(parent) {
		//Populate role list
		roleNames.insert(NGQStandardItemModel::ItemId, "itemId");
		roleNames.insert(NGQStandardItemModel::ItemName, "itemName");
		this->setRoleNames(roleNames);
	}

	enum listRoles {ItemId=Qt::UserRole+1,ItemName=Qt::UserRole+2};
private:
	QHash<int, QByteArray> roleNames;
};

class NGQProxy : public QObject {
	Q_OBJECT;

public:
    NGQProxy(struct gui_priv* this_,QObject *parent) : QObject(parent) {
        this->object=this_;
    }

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
				counter++;
			}
			ret->appendRow(curItem);
		}

		dropIterFunc(iter);

		this->object->guiWidget->rootContext()->setContextProperty("listModel",static_cast<QObject*>(ret));

		dbg(0,"retId %s \n",retId.toStdString().c_str());

		return retId;
	}
	void setAttr(const QString &attr_name, const QString &attr_string) {
			struct attr attr_value;
			double *helper;

			dbg(1,"Setting %s to %s\n",attr_name.toStdString().c_str(),attr_string.toStdString().c_str());
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
	
protected:
    struct gui_priv* object;

	virtual int setAttrFunc(struct attr *attr)=0;
	virtual int getAttrFunc(enum attr_type type, struct attr *attr, struct attr_iter *iter)=0;
	virtual struct attr_iter* getIterFunc() { return NULL; };
	virtual void dropIterFunc(struct attr_iter*) { return; };
};

#include "proxy.moc"

#endif /* NAVIT_GUI_QML_PROXY_H */

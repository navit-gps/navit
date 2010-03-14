#ifndef NAVIT_GUI_QML_PROXY_H
#define NAVIT_GUI_QML_PROXY_H

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
		return ret;
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
};

#include "proxy.moc"

#endif /* NAVIT_GUI_QML_PROXY_H */

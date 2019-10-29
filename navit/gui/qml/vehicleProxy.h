#ifndef NAVIT_GUI_QML_VEHICLEPROXY_H
#define NAVIT_GUI_QML_VEHICLEPROXY_H

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

#include "vehicleProxy.moc"

#endif /* NAVIT_GUI_QML_VEHICLEPROXY_H */

#ifndef NAVIT_GUI_QML_VOICEPROXY_H
#define NAVIT_GUI_QML_VOICEPROXY_H

class NGQProxyVoice : public NGQProxy {
    Q_OBJECT;

public:
	NGQProxyVoice(struct gui_priv* object, QObject* parent) : NGQProxy(object,parent) { };

public slots:

protected:
	int getAttrFunc(enum attr_type type, struct attr* attr, struct attr_iter* iter) { return voice_get_attr(this->object->currVoice, type, attr, iter); }
	int setAttrFunc(struct attr* attr) {return voice_set_attr(this->object->currVoice,attr); }
	struct attr_iter* getIterFunc() { return voice_attr_iter_new(NULL); };
	void dropIterFunc(struct attr_iter* iter) { voice_attr_iter_destroy(iter); };

private:
};

#include "voiceProxy.moc"

#endif /* NAVIT_GUI_QML_VOICEPROXY_H */

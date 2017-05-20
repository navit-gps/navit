#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQmlListProperty>

#include "qml_map.h"
#include "qml_poi.h"

#include "coord.h"

class Backend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QObject> pois READ getPois NOTIFY poisChanged)
    Q_PROPERTY(QQmlListProperty<QObject> maps READ getMaps NOTIFY mapsChanged)
    Q_PROPERTY(PoiObject * activePoi READ activePoi NOTIFY activePoiChanged)

public:
    explicit Backend(QObject *parent = 0);

    void showMenu(struct point* p);
    void set_navit(struct navit* nav);
    void set_engine(QQmlApplicationEngine* engine);

    QList < PoiObject * > pois;
    QQmlListProperty<QObject> getPois();
    QList < MapObject * > maps;
    QQmlListProperty<QObject> getMaps();
    PoiObject * activePoi();


signals:
    void displayMenu();
    void hideMenu();
    void poisChanged();
    void activePoiChanged();
    void mapsChanged();

public slots:
    void get_maps();
    void get_pois();
    QString get_icon_path();
    void setActivePoi(int index);
    void setActivePoiAsDestination();

private:
    struct navit *nav;
    struct point *p;
    struct coord_geo g;
    struct pcoord c;
    int filter_pois(struct item *item);
    QQmlApplicationEngine* engine;
    QList<QObject *> _pois;
    QList<QObject *> _maps;
    PoiObject * m_activePoi;

};

#endif // BACKEND_H

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

public:
    explicit Backend(QObject *parent = 0);

    void showMenu(struct point* p);
    void set_navit(struct navit* nav);
    void set_engine(QQmlApplicationEngine* engine);
    QList < PoiObject * > pois;
    QQmlListProperty<QObject> getPois();
    QList < MapObject * > maps;
    QQmlListProperty<QObject> getMaps();


signals:
    void displayMenu();
    void poisChanged();
    void mapsChanged();

public slots:
    void get_maps();
    void get_pois();
    void show_poi(int index);
    QString get_icon_path();

private:
    struct navit *nav;
    struct point *p;
    struct coord_geo g;
    struct pcoord c;
    QQmlApplicationEngine* engine;
    QList<QObject *> _pois;
    QList<QObject *> _maps;

};

#endif // BACKEND_H

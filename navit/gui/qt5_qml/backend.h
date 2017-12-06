#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlListProperty>

#include "qml_map.h"
#include "qml_poi.h"
#include "qml_bookmark.h"
#include "qml_vehicle.h"

#include "coord.h"
#include "item.h"
#include "attr.h"

class Backend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QObject> pois READ getPois NOTIFY poisChanged)
    Q_PROPERTY(QQmlListProperty<QObject> bookmarks READ getBookmarks NOTIFY bookmarksChanged)
    Q_PROPERTY(QQmlListProperty<QObject> maps READ getMaps NOTIFY mapsChanged)
    Q_PROPERTY(QQmlListProperty<QObject> vehicles READ getVehicles NOTIFY vehiclesChanged)
    Q_PROPERTY(PoiObject * activePoi READ activePoi NOTIFY activePoiChanged)
    Q_PROPERTY(BookmarkObject * currentBookmark READ currentBookmark NOTIFY currentBookmarkChanged)
    Q_PROPERTY(VehicleObject * currentVehicle READ currentVehicle NOTIFY currentVehicleChanged)
    Q_PROPERTY(QQmlListProperty<QObject> searchresults READ getSearchResults NOTIFY searchResultsChanged)
    // Search properties
    Q_PROPERTY(QString currentCountry READ currentCountry NOTIFY currentCountryChanged)
    Q_PROPERTY(QString currentCountryIso2 READ currentCountryIso2 NOTIFY currentCountryIso2Changed)
    Q_PROPERTY(QString currentTown READ currentTown NOTIFY currentTownChanged)
    Q_PROPERTY(QString currentStreet READ currentStreet NOTIFY currentStreetChanged)

public:
    explicit Backend(QObject *parent = 0);

    void showMenu(struct point* p);
    void set_navit(struct navit* nav);
    void set_engine(QQmlApplicationEngine* engine);

    QList < PoiObject * > pois;
    QQmlListProperty<QObject> getPois();
    QList < BookmarkObject * > bookmarks;
    QQmlListProperty<QObject> getBookmarks();
    QList < MapObject * > maps;
    QQmlListProperty<QObject> getMaps();
    QList < MapObject * > vehicles;
    QQmlListProperty<QObject> getVehicles();
    PoiObject * activePoi();
    BookmarkObject * currentBookmark();
    VehicleObject * currentVehicle();
    QQmlListProperty<QObject> getSearchResults();
    QString currentCountry();
    QString currentCountryIso2();
    QString currentTown();
    QString currentStreet();

signals:
    void displayMenu(QString source);
    void hideMenu();
    void poisChanged();
    void bookmarksChanged();
    void activePoiChanged();
    void currentBookmarkChanged();
    void currentVehicleChanged();
    void mapsChanged();
    void vehiclesChanged();
    void searchResultsChanged();
    void currentCountryChanged();
    void currentCountryIso2Changed();
    void currentTownChanged();
    void currentStreetChanged();

public slots:
    void get_maps();
    void get_pois();
    void get_bookmarks();
    void get_vehicles();
    QString get_icon_path();
    QString get_country_icon(char * country_iso_code);
    void setActivePoi(int index);
    void setCurrentBookmark(int index);
    void setCurrentVehicle(int index);
    void setActivePoiAsDestination();
    void updateSearch(QString text);
    void searchValidateResult(int index);
    void resize(int width, int height);
    void setSearchContext(QString text);
    void block_draw();

private:
    struct navit *nav;
    struct point *p;
    struct coord_geo g;
    struct pcoord c;
    int filter_pois(struct item *item);
    QQmlApplicationEngine* engine;
    QList<QObject *> _pois;
    QList<QObject *> _bookmarks;
    QList<QObject *> _maps;
    QList<QObject *> _vehicles;
    PoiObject * m_activePoi;
    BookmarkObject * m_currentBookmark;
    VehicleObject * m_currentVehicle;
    QList<QObject *> _search_results;
    void set_default_country();
    char * _country_iso2;
    char * _current_country;
    char * _current_town;
    char * _current_street;
    struct search_param *search;
    enum attr_type _search_context;
};

#endif // BACKEND_H

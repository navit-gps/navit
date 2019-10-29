#include <glib.h>

#include "item.h"
#include "attr.h"
#include "navit.h"
#include "xmlconfig.h" // for NAVIT_OBJECT
#include "layout.h"
#include "map.h"
#include "transform.h"
#include "vehicle.h"
#include "bookmarks.h"
#include "backend.h"

#include "qml_map.h"
#include "qml_poi.h"
#include "qml_search.h"

#include "mapset.h"

#include <QQmlContext>

#include "search.h"

extern "C" {
#include "proxy.h"
}

Backend::Backend(QObject * parent):QObject(parent) {
    set_default_country();
    this->search = NULL;
    _current_town = NULL;
    _current_street = NULL;
}

/**
 * @brief Set some variables and display the main menu
 * @param struct point *p the point coordinate where we clicked on the screen
 * @returns nothing
 */
void Backend::showMenu(struct point *p) {
    struct coord co;

    transform_reverse(navit_get_trans(nav), p, &co);
    dbg(lvl_debug, "Point 0x%x 0x%x", co.x, co.y);
    dbg(lvl_debug, "Screen coord : %d %d", p->x, p->y);
    transform_to_geo(transform_get_projection(navit_get_trans(nav)), &co, &(this->g));
    dbg(lvl_debug, "%f %f", this->g.lat, this->g.lng);
    dbg(lvl_debug, "%p %p", nav, &c);
    this->c.pro = transform_get_projection(navit_get_trans(nav));
    this->c.x = co.x;
    this->c.y = co.y;
    dbg(lvl_debug, "c : %x %x", this->c.x, this->c.y);

    // As a test, set the Demo vehicle position to wherever we just clicked
    navit_set_position(this->nav, &c);
    navit_block(this->nav, 1);
    emit displayMenu("MainMenu.qml");
}

/**
 * @brief update the private m_maps list. Expected to be called from QML
 * @param none
 * @returns nothing
 */
void Backend::get_maps() {
    struct attr attr, on, off, description, type, data, active;
    char * label;
    bool is_active;
    struct attr_iter * iter;
    _maps.clear();

    iter = navit_attr_iter_new();
    on.type = off.type = attr_active;
    on.u.num = 1;
    off.u.num = 0;
    while (navit_get_attr(this->nav, attr_map, &attr, iter)) {
        if (map_get_attr(attr.u.map, attr_description, &description, NULL)) {
            label = g_strdup(description.u.str);
        } else {
            if (!map_get_attr(attr.u.map, attr_type, &type, NULL))
                type.u.str = "";
            if (!map_get_attr(attr.u.map, attr_data, &data, NULL))
                data.u.str = "";
            label = g_strdup_printf("%s:%s", type.u.str, data.u.str);
        }
        is_active = false;
        if (map_get_attr(attr.u.map, attr_active, &active, NULL)) {
            if (active.u.num == 1) {
                is_active = true;
            }
        }
        _maps.append(new MapObject(label, is_active));
    }
    emit mapsChanged();
}


/**
 * @brief update the private m_vehicles list. Expected to be called from QML
 * @param none
 * @returns nothing
 */
void Backend::get_vehicles() {
    struct attr attr,attr2,vattr;
    struct attr_iter *iter;
    struct attr active_vehicle;
    _vehicles.clear();

    iter=navit_attr_iter_new();
    if (navit_get_attr(this->nav, attr_vehicle, &attr, iter) && !navit_get_attr(this->nav, attr_vehicle, &attr2, iter)) {
        vehicle_get_attr(attr.u.vehicle, attr_name, &vattr, NULL);
        navit_attr_iter_destroy(iter);
        _vehicles.append(new VehicleObject(g_strdup(vattr.u.str), active_vehicle.u.vehicle, attr.u.vehicle));
        dbg(lvl_debug, "done");
        emit vehiclesChanged();
        return;
    }
    navit_attr_iter_destroy(iter);

    if (!navit_get_attr(this->nav, attr_vehicle, &active_vehicle, NULL))
        active_vehicle.u.vehicle=NULL;
    iter=navit_attr_iter_new();
    while(navit_get_attr(this->nav, attr_vehicle, &attr, iter)) {
        vehicle_get_attr(attr.u.vehicle, attr_name, &vattr, NULL);
        dbg(lvl_debug, "adding vehicle %s", vattr.u.str);
        _vehicles.append(
            new VehicleObject(
                g_strdup(vattr.u.str),
                attr.u.vehicle == active_vehicle.u.vehicle,
                attr.u.vehicle
            )
        );
    }
    navit_attr_iter_destroy(iter);
    emit vehiclesChanged();
}

/**
 * @brief set a pointer to the struct navit * for local use
 * @param none
 * @returns nothing
 */
void Backend::set_navit(struct navit *nav) {
    this->nav = nav;
}

/**
 * @brief set a pointer to the QQmlApplicationEngine * for local use
 * @param none
 * @returns nothing
 */
void Backend::set_engine(QQmlApplicationEngine * engine) {
    this->engine = engine;
}

/**
 * @brief apply search filters on one specific item
 * @param struct item * the item to filter
 * @returns 0 if the item should be discarded, 1 otherwise
 */
int Backend::filter_pois(struct item *item) {
    enum item_type *types;
    enum item_type type=item->type;
    if (type >= type_line)
        return 0;
    return 1;
}

/**
 * @brief update the private m_bookmarks list. Expected to be called from QML
 * @param none
 * @returns nothing
 */
void Backend::get_bookmarks() {
    struct attr attr,mattr;
    struct navigation * nav = NULL;
    struct item *item;
    struct coord c;
    struct pcoord pc;

    _bookmarks.clear();

    pc.pro = transform_get_projection(navit_get_trans(this->nav));

    if(navit_get_attr(this->nav, attr_bookmarks, &mattr, NULL) ) {
        bookmarks_item_rewind(mattr.u.bookmarks);
        while ((item=bookmarks_get_item(mattr.u.bookmarks))) {
            if (!item_attr_get(item, attr_label, &attr)) continue;
            dbg(lvl_debug,"full_label: %s", attr.u.str);
            if (item_coord_get(item, &c, 1)) {
                pc.x = c.x;
                pc.y = c.y;
                dbg(lvl_debug, "coords : %i x %i", pc.x, pc.y);
                _bookmarks.append(new BookmarkObject(attr.u.str, pc));
            }
        }
    }
    emit bookmarksChanged();
}

/**
 * @brief update the private m_pois list. Expected to be called from QML
 * @param none
 * @returns nothing
 */
void Backend::get_pois() {
    struct map_selection * sel, * selm;
    struct coord c, center;
    struct mapset_handle * h;
    struct map * m;
    struct map_rect * mr;
    struct item * item;
    enum projection pro = this->c.pro;
    int idist, dist;
    _pois.clear();
    dist = 10000;
    sel = map_selection_rect_new(&(this->c), dist * transform_scale(abs(this->c.y) + dist * 1.5), 18);
    center.x = this->c.x;
    center.y = this->c.y;

    dbg(lvl_debug, "center is at %x, %x", center.x, center.y);

    h = mapset_open(navit_get_mapset(this->nav));
    while ((m = mapset_next(h, 1))) {
        selm = map_selection_dup_pro(sel, pro, map_projection(m));
        mr = map_rect_new(m, selm);
        dbg(lvl_debug, "mr=%p", mr);
        if (mr) {
            while ((item = map_rect_get_item(mr))) {
                if ( filter_pois(item) &&
                        item_coord_get_pro(item, &c, 1, pro) &&
                        coord_rect_contains(&sel->u.c_rect, &c)  &&
                        (idist=transform_distance(pro, &center, &c)) < dist) {
                    struct attr attr;
                    char * label;
                    char * icon = get_icon(this->nav, item);
                    struct pcoord item_coord;
                    item_coord.pro = transform_get_projection(navit_get_trans(nav));
                    item_coord.x = c.x;
                    item_coord.y = c.y;

                    idist = transform_distance(pro, &center, &c);
                    if (item_attr_get(item, attr_label, &attr)) {
                        label = map_convert_string(item->map, attr.u.str);
                        if (icon) {
                            _pois.append(new PoiObject(label, item_to_name(item->type), idist, icon, item_coord));
                        }
                    }
                }
            }
            map_rect_destroy(mr);
        }
        map_selection_destroy(selm);
    }
    map_selection_destroy(sel);
    mapset_close(h);
    emit poisChanged();
}

/**
 * @brief get the POIs as a QList
 * @param none
 * @returns the pois QList
 */
QQmlListProperty<QObject> Backend::getPois() {
    return QQmlListProperty<QObject>(this, _pois);
}

/**
 * @brief get the Bookmarks as a QList
 * @param none
 * @returns the bookmarks QList
 */
QQmlListProperty<QObject> Backend::getBookmarks() {
    return QQmlListProperty<QObject>(this, _bookmarks);
}

/**
 * @brief get the maps as a QList
 * @param none
 * @returns the maps QList
 */
QQmlListProperty<QObject> Backend::getMaps() {
    return QQmlListProperty<QObject>(this, _maps);
}


/**
 * @brief get the vehicles as a QList
 * @param none
 * @returns the vehicles QList
 */
QQmlListProperty<QObject> Backend::getVehicles() {
    return QQmlListProperty<QObject>(this, _vehicles);
}

/**
 * @brief get the search results as a QList
 * @param none
 * @returns the search results QList
 */
QQmlListProperty<QObject> Backend::getSearchResults() {
    return QQmlListProperty<QObject>(this, _search_results);
}

/**
 * @brief get the active POI. Used when displaying the relevant menu
 * @param none
 * @returns the active POI
 */
PoiObject * Backend::activePoi() {
    dbg(lvl_debug, "name : %s", m_activePoi->name().toUtf8().data());
    dbg(lvl_debug, "type : %s", m_activePoi->type().toLatin1().data());
    return m_activePoi;
}

/**
 * @brief get the current bookmark. Used when displaying the relevant menu
 * @param none
 * @returns the current bookmark
 */
BookmarkObject * Backend::currentBookmark() {
    return m_currentBookmark;
}

/**
 * @brief get the currently selected vehicle. Used when displaying the relevant menu
 * @param none
 * @returns the active POI
 */
VehicleObject * Backend::currentVehicle() {
    struct attr attr;
    dbg(lvl_debug, "name : %s", m_currentVehicle->name().toUtf8().data());
    if (m_currentVehicle->vehicle()) {
        if (vehicle_get_attr(m_currentVehicle->vehicle(), attr_position_nmea, &attr, NULL))
            dbg(lvl_debug, "NMEA : %s", attr.u.str);
    } else {
        dbg(lvl_debug, "m_currentVehicle->v is null");
    }

    return m_currentVehicle;
}


void Backend::block_draw() {
    navit_block(this->nav, 1);
    dbg(lvl_debug, "Draw operations blocked per UI request");
}

/**
 * @brief set the canvas size to use when drawing the map
 * @param int width
 * @param int height
 * @returns nothing
 */
void Backend::resize(int width, int height) {
    // If we need to resize the canvas, it means that something (the main map,
    // or a menu item) wants to display a map. Ensure that draw operations
    // are not blocked then.
    navit_block(this->nav, -1);
    navit_handle_resize(nav, width, height);
}

/**
 * @brief set the active POI. Used when clicking on a POI list to display one single POI
 * @param int index the index of the POI in the m_pois list
 * @returns nothing
 */
void Backend::setActivePoi(int index) {
    struct pcoord c;
    m_activePoi = (PoiObject *)_pois.at(index);
    c = m_activePoi->coords();
    resize(320, 240);
    navit_set_center(this->nav, &c, 1);
    emit activePoiChanged();
}
/**
 * @brief set the current bookmark. Used when clicking on a bookmark list to display one single bookmark
 * @param int index the index of the bookmark in the m_bookmarks list
 * @returns nothing
 */
void Backend::setCurrentBookmark(int index) {
    struct pcoord c;
    m_currentBookmark = (BookmarkObject *)_bookmarks.at(index);
    c = m_currentBookmark->coords();
    resize(320, 240);
    navit_set_center(this->nav, &c, 1);
    emit currentBookmarkChanged();
}

/**
 * @brief set the current vehicle. Used when clicking on a vehicle list to display one single vehicle
 * @param int index the index of the vehicle in the m_vehicles list
 * @returns nothing
 */
void Backend::setCurrentVehicle(int index) {
    m_currentVehicle = (VehicleObject *)_vehicles.at(index);
    emit currentVehicleChanged();
}

/**
 * @brief returns the icon absolute path
 * @param none
 * @returns the icon absolute path as a QString
 */
QString Backend::get_icon_path() {
    return QString(g_strjoin(NULL,"file://",getenv("NAVIT_SHAREDIR"),"/icons/",NULL));
}

/**
 * @brief set the destination using the currently active POI's coordinates
 * @param none
 * @returns nothing
 */
void Backend::setActivePoiAsDestination() {
    struct pcoord c;
    c = m_activePoi->coords();
    dbg(lvl_debug, "Destination : %s c=%d:0x%x,0x%x",
        m_activePoi->name().toUtf8().data(),
        c.pro, c.x, c.y);
    navit_set_destination(this->nav, &c,  m_activePoi->name().toUtf8().data(), 1);
    emit hideMenu();
}

/**
 * @brief save the search result for the next search step
 * @param int index the index of the result in the m_search_results list
 * @returns nothing
 */
void Backend::searchValidateResult(int index) {
    SearchObject * r = (SearchObject *)_search_results.at(index);
    dbg(lvl_debug, "Saving %s [%i] as search result", r->name().toUtf8().data(), index);
    if (r->getCoords()) {
        dbg(lvl_debug, "Item is at %x x %x", r->getCoords()->x, r->getCoords()->y);
    }
    if (_search_context == attr_country_all) {
        _current_country = g_strdup(r->name().toUtf8().data());
        _current_town = NULL;
        _current_street = NULL;
    } else if (_search_context == attr_town_name) {
        _current_town = g_strdup(r->name().toUtf8().data());
        _current_street = NULL;
    } else if (_search_context == attr_street_name) {
        _current_street = g_strdup(r->name().toUtf8().data());
    } else {
        dbg(lvl_error, "Unknown search context for '%s'", r->name().toUtf8().data());
    }
    // navit_set_center(this->nav, r->getCoords(), 1);
    emit displayMenu("destination_address.qml");
}

/**
 * @brief get the icon that matches the country currently used for searches
 * @param none
 * @returns an absolute path for the country icon
 */
QString Backend::get_country_icon(char * country_iso_code) {
//        if ( country_iso_code == "" ) {
//                country_iso_code = _country_iso2;
//        }
    return QString(g_strjoin(NULL,"file://",getenv("NAVIT_SHAREDIR"),"/icons/",country_iso_code,".svg",NULL));
}


static struct search_param {
    struct navit *nav;
    struct mapset *ms;
    struct search_list *sl;
    struct attr attr;
    int partial;
    void *entry_country, *entry_postal, *entry_city, *entry_district;
    void *entry_street, *entry_number;
} search_param;

/**
  * @brief set the default country
  * @param none
  * returns nothing
  */
void Backend::set_default_country() {
    _current_country = "Germany";
    _country_iso2 = "DE";
}


/**
 * @brief update the current search results according to new inputs. Currently only works to search for towns
 * @param QString text the text to search for
 * @returns nothing
 */
void Backend::updateSearch(QString text) {
    struct search_list_result *res;
    struct attr search_attr;

    if (search == NULL) {
        search=&search_param;
        dbg(lvl_debug, "search = %p", search);
        search->nav=this->nav;
        search->ms=navit_get_mapset(this->nav);
        search->sl=search_list_new(search->ms);
        search->partial = 1;
        dbg(lvl_debug,"attempting to use country '%s'", _country_iso2);
        search_attr.type=attr_country_iso2;
        search_attr.u.str=_country_iso2;
        search_list_search(search->sl, &search_attr, 0);

        while((res=search_list_get_result(search->sl)));
    }

    _search_results.clear();
    //  search->attr.type=attr_country_all;
    //  search->attr.type=attr_town_postal;
    //  search->attr.type=attr_town_name;
    //  search->attr.type=attr_street_name;

//        search->attr.type=attr_town_name;
//        search->attr.u.str="Oberhaching";
//        search_list_search(search->sl, &search->attr, search->partial);
//        while((res=search_list_get_result(search->sl)));

    search->attr.u.str = text.toUtf8().data();
    dbg(lvl_error, "searching for %s partial %d", search->attr.u.str, search->partial);

    search->attr.type = _search_context;
    search_list_search(search->sl, &search->attr, search->partial);
    int count = 0;
    while((res=search_list_get_result(search->sl))) {
        if ( _search_context == attr_country_all && res->country) {
            char * label;
            label = g_strdup(res->country->name);
            _search_results.append(
                new SearchObject(label, get_country_icon(res->country->flag), res->c)
            );
        }
        if ( _search_context == attr_town_name && res->town) {
            char * label;
            label = g_strdup(res->town->common.town_name);
            _search_results.append(
                new SearchObject(label, "icons/bigcity.png", res->c)
            );
        }
        if (res->street) {
            char * label;
            label = g_strdup(res->street->name);
            _search_results.append(
                new SearchObject(label, "icons/smallcity.png", res->c)
            );
        }
        if (count ++ > 50) {
            break;
        }
    }
    emit searchResultsChanged();
}

void Backend::setSearchContext(QString text) {
    if (text == "country") {
        _search_context = attr_country_all;
    } else if (text == "town") {
        _search_context = attr_town_name;
    } else if (text == "street") {
        _search_context = attr_street_name;
    } else {
        dbg(lvl_error, "Unhandled search context '%s'", text.toUtf8().data());
    }
}

QString Backend::currentCountry() {
    dbg(lvl_debug, "Current country : %s/%s", _country_iso2, _current_country);
    return QString(_current_country);
}

QString Backend::currentCountryIso2() {
    dbg(lvl_debug, "Current country : %s/%s", _country_iso2, _current_country);
    return QString(_country_iso2);
}

QString Backend::currentTown() {
    if (_current_town == NULL) {
        _current_town = "Enter City";
    }
    dbg(lvl_debug, "Current town : %s", _current_town);
    return QString(_current_town);
}

QString Backend::currentStreet() {
    if (_current_street == NULL) {
        _current_street = "Enter Street";
    }
    dbg(lvl_debug, "Current street : %s", _current_street);
    return QString(_current_street);
}

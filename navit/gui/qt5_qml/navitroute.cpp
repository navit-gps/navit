#include "navitroute.h"

NavitRoute::NavitRoute() : m_status(Invalid)
{

}
void listIcons(NavitInstance *navitInstance) {
    struct attr attr;
    navit_get_attr(navitInstance->getNavit(), attr_layout, &attr, nullptr);

    if(attr.u.layout && attr.u.layout->layers) {
        qDebug() << "Got layout";
        GList* layers = attr.u.layout->layers;
        while (layers) {
            qDebug() << "Got layers";
            struct layer * l = (struct layer *)layers->data;
            QString name = l->name;
            if(name.contains("POI", Qt::CaseInsensitive)){
                GList * itemgras = l->itemgras;
                while(itemgras){
                    struct itemgra * itg = (struct itemgra *)itemgras->data;
                    GList * elements = itg->elements;
                    GList * types = itg->type;
                    while(elements){
                        struct element * el = (struct element *)elements->data;
                        //                    struct attr itattr = (struct attr) g_list_nth(types, g_list_index(elements, elements->data));
                        if(el->type == element::element_icon){
                            qDebug () << "src : " << el->u.icon.src << "x : " << el->u.icon.x << "y : " << el->u.icon.y;
                        }
                        elements = g_list_next(elements);
                    }
                    itemgras = g_list_next(itemgras);
                }
            }
            layers=g_list_next(layers);
        }
    }
}

void NavitRoute::setNavit(NavitInstance * navit){
    m_navitInstance = navit;

    struct callback* cb = callback_new_attr_1(callback_cast(NavitRoute::routeCallbackHandler),
                                              attr_position_coord_geo,this);
    struct callback* cb2 = callback_new_attr_1(callback_cast(NavitRoute::destinationCallbackHandler),
                                               attr_destination,this);

    struct route * route = navit_get_route(m_navitInstance->getNavit());
    m_destCount = route_get_destination_count(route);

    navit_add_callback(m_navitInstance->getNavit(),cb);
    navit_add_callback(m_navitInstance->getNavit(),cb2);

    struct attr route_attr;

    if (navit_get_attr(m_navitInstance->getNavit(),attr_route,&route_attr,nullptr)) {
        struct attr callback;
        callback.type=attr_callback;
        callback.u.callback=callback_new_attr_1(callback_cast(NavitRoute::statusCallbackHandler), attr_route_status, this);
        route_add_attr(route_attr.u.route, &callback);
    }

    routeUpdate();
}

void NavitRoute::updateNextTurn(struct map * map){
    struct map_rect * mr = nullptr;
    struct item * item = nullptr;

    if(map)
        mr = map_rect_new(map,nullptr);
    if(mr) {
        while ((item = map_rect_get_item(mr))
                && (item->type == type_nav_position || item->type == type_nav_none/* || level-- > 0*/));
        if (item) {
            QString url = QString("%1_bk.svg").arg(graphics_icon_path(item_to_name(item->type)));
            QUrl nextTurnIcon = QUrl::fromLocalFile(url);
            if(nextTurnIcon != m_nextTurnIcon){
                m_nextTurnIcon = nextTurnIcon;
                emit nextTurnChanged();
            }
        }
    }
    map_rect_destroy(mr);
}

void NavitRoute::routeUpdate(){
    struct map * map = nullptr;
    struct map_rect * mr = nullptr;
    struct navigation * nav = nullptr;
    struct attr attr,route;
    struct item * item = nullptr;
    struct item * item2 = nullptr;

    nav = navit_get_navigation(m_navitInstance->getNavit());
    if(!nav) {
        return;
    }
    map = navigation_get_map(nav);
    if(map)
        mr = map_rect_new(map,nullptr);
    if(mr) {
        m_directions.clear();
        while((item = map_rect_get_item(mr))) {
            if(item_attr_get(item,attr_navigation_long,&attr)) {
                m_directions << map_convert_string_tmp(item->map,attr.u.str);
            }
        }
    }

    map_rect_destroy(mr);
    if(map)
        mr = map_rect_new(map,nullptr);
    if(mr) {
        map_rect_get_item(mr);
        if((item2 = map_rect_get_item(mr))){
            struct attr length_attr, street_attr;
            if(item_attr_get(item2,attr_length,&length_attr)) {
                m_nextTurnDistance = QString("%1 meters").arg(length_attr.u.num);
            }

            QString streetname;
            if(item_attr_get(item2,attr_street_name,&street_attr)) {
                streetname = street_attr.u.str;
            }
            if(item_attr_get(item2,attr_street_name_systematic,&street_attr)) {
                if(streetname.isEmpty())
                    streetname = street_attr.u.str;
                else
                    streetname.append(QString(" (%1)").arg(street_attr.u.str));
            }
            m_nextTurn = streetname;

            emit nextTurnChanged();
        }

    }
    map_rect_destroy(mr);

    if (navit_get_attr(m_navitInstance->getNavit(), attr_route, &route, nullptr)) {
        struct attr destination_length, destination_time;

        if (route_get_attr(route.u.route, attr_destination_length, &destination_length, nullptr)){
            m_distance = attr_to_text_ext(&destination_length, nullptr, attr_format_with_units, attr_format_default, nullptr);
        }

        if (route_get_attr(route.u.route, attr_destination_time, &destination_time, nullptr)){
            char test[] = ": ads :";

            QStringList timeLeft = QString(attr_to_text_ext(&destination_time, test, attr_format_with_units, attr_format_default, nullptr)).split(":");
            QDateTime dt = QDateTime::currentDateTime();
            QTime time;

            switch (timeLeft.size()) {
            case 4:
    //            m_timeLeft = QString("%1 days %2 hours %3%4").arg("");
                dt = dt.addDays(timeLeft[0].toInt());
                time.setHMS(timeLeft[1].toInt(),timeLeft[2].toInt(),timeLeft[3].toInt());
                m_timeLeft = QString("%1 day %2 h %3 min").arg(timeLeft[0].toInt()).arg(timeLeft[1].toInt()).arg(timeLeft[2].toInt());
                break;
            case 3:
                time.setHMS(timeLeft[0].toInt(),timeLeft[1].toInt(),timeLeft[2].toInt());
                m_timeLeft = QString("%1 h %2 min").arg(timeLeft[0].toInt()).arg(timeLeft[1].toInt());
                break;
            case 2:
                time.setHMS(0,timeLeft[0].toInt(),timeLeft[1].toInt());
                m_timeLeft = QString("%1 min").arg(timeLeft[0].toInt());
                break;
            case 1:
                time.setHMS(0,0,timeLeft[0].toInt());
                m_timeLeft = QString("%1 seconds").arg(timeLeft[0].toInt());
                break;
            }

            dt = dt.addMSecs(time.msecsSinceStartOfDay());

            m_arrivalTime = dt.toString("hh:mm");
        }
    }

    struct attr vehicle,speed_attr;

    int speed = -1;
    if (navit_get_attr(m_navitInstance->getNavit(), attr_vehicle, &vehicle, nullptr)) {
        vehicle_get_attr(vehicle.u.vehicle,attr_position_speed, &speed_attr, nullptr);
        speed = *(speed_attr.u.numd);
    }
    m_speed = speed;

    struct attr maxspeed_attr, street_name_attr;
    struct tracking *tracking;
    int routespeed = -1;
    tracking = navit_get_tracking(m_navitInstance->getNavit());

    if (tracking) {

        if (tracking_get_attr(tracking, attr_maxspeed, &maxspeed_attr, nullptr)) {
            routespeed = maxspeed_attr.u.num;
        }
        if (routespeed == -1) {
            struct vehicleprofile *prof=navit_get_vehicleprofile(m_navitInstance->getNavit());
            struct roadprofile *rprof=nullptr;
            if (prof && item)
                rprof=vehicleprofile_get_roadprofile(prof, item->type);
            if (rprof) {
                if(rprof->maxspeed!=0)
                    routespeed=rprof->maxspeed;
            }
        }
        QString streetname;
        if (tracking_get_attr(tracking, attr_street_name, &street_name_attr, nullptr)) {
            streetname = street_name_attr.u.str;
        }
        if (tracking_get_attr(tracking, attr_street_name_systematic, &street_name_attr, nullptr)) {
            if(streetname.isEmpty())
                streetname = street_name_attr.u.str;
            else
                streetname.append(QString(" (%1)").arg(street_name_attr.u.str));
        }
        m_currentStreet = streetname;
    }
    m_speedLimit = routespeed;
    emit propertiesChanged();

    updateNextTurn(map);
    statusUpdate();
}

QString NavitRoute::getLastDestination(struct pcoord *pc) {
    if(m_navitInstance){
        struct map *formerdests;
        struct map_rect *mr_formerdests;
        struct item *item;
        struct attr attr;

        if(!navit_get_attr(m_navitInstance->getNavit(), attr_former_destination_map, &attr, nullptr))
            return "";

        formerdests=attr.u.map;
        if(!formerdests)
            return "";

        mr_formerdests=map_rect_new(formerdests, nullptr);
        if(!mr_formerdests)
            return "";

        pc->pro = map_projection(formerdests);

        QString ret;
        while ((item=map_rect_get_item(mr_formerdests))) {
            struct coord c;
            if (item->type!=type_former_destination) continue;
            if (!item_attr_get(item, attr_label, &attr)) continue;

            if (item_coord_get(item, &c, 1)) {
                pc->x = c.x;
                pc->y = c.y;
                ret=attr.u.str;
            }
        }
        map_rect_destroy(mr_formerdests);
        return ret;
    }
}
void NavitRoute::destinationUpdate(){
    struct route * route = navit_get_route(m_navitInstance->getNavit());
    int destCount = route_get_destination_count(route);

    if(destCount > m_destCount){
        QString destination = getLastDestination(&m_lastDestinationCoord);

        navit_set_center(m_navitInstance->getNavit(), &m_lastDestinationCoord, 1);
        navit_zoom_level(m_navitInstance->getNavit(), 4, nullptr);
        emit destinationAdded(destination);
    } else if (destCount < m_destCount) {
        emit destinationRemoved();
    }
    if(destCount == 0){
        emit navigationFinished();
    }
    m_destCount = destCount;
}

void NavitRoute::statusUpdate(){
    struct navigation * nav = nullptr;
    nav = navit_get_navigation(m_navitInstance->getNavit());
    if(!nav) {
        return;
    }
    struct attr attr;
    if(navigation_get_attr(nav, attr_nav_status, &attr,nullptr)){

        int status = (attr.u.num == status_recalculating) ? status_routing : attr.u.num;
        if(m_status != status){
            m_status = static_cast<Status>(status);
            qDebug() << "status : " << nav_status_to_text(status);
            emit statusChanged();
        }
    }
}
void NavitRoute::routeCallbackHandler(NavitRoute * navitRoute){
    navitRoute->routeUpdate();
}
void NavitRoute::destinationCallbackHandler(NavitRoute * navitRoute){
    navitRoute->destinationUpdate();
}

void NavitRoute::statusCallbackHandler(NavitRoute * navitRoute, int status){
    navitRoute->statusUpdate();
}
void NavitRoute::cancelNavigation(){
    if(m_navitInstance){
        navit_set_destination(m_navitInstance->getNavit(), nullptr, nullptr, 0);
    }
}

void NavitRoute::setDestination(QString label, int x, int y){
    if(m_navitInstance){
        cancelNavigation();
        struct pcoord c = NavitHelper::positionToPcoord(m_navitInstance, x, y);
        NavitHelper::setDestination(m_navitInstance, label, c.x, c.y);
    }
}

void NavitRoute::setPosition(int x, int y){
    if(m_navitInstance){
        struct pcoord c = NavitHelper::positionToPcoord(m_navitInstance, x, y);
        navit_set_position(m_navitInstance->getNavit(), &c);
        NavitHelper::setPosition(m_navitInstance, c.x, c.y);
    }
}

void NavitRoute::addStop(QString label, int x, int y,  int position){
    if(m_navitInstance){
        struct pcoord c = NavitHelper::positionToPcoord(m_navitInstance, x, y);
        NavitHelper::addStop(m_navitInstance, position, label, c.x, c.y);
    }
}

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

    statusUpdate();
}

void NavitRoute::routeUpdate(){
    struct map * map = nullptr;
    struct map_rect * mr = nullptr;
    struct navigation * nav = nullptr;
    struct attr attr,route;
    struct item * item = nullptr;

    nav = navit_get_navigation(m_navitInstance->getNavit());
    if(!nav) {
        return;
    }
    map = navigation_get_map(nav);
    if(map)
        mr = map_rect_new(map,nullptr);
    if(mr) {
        if (navit_get_attr(m_navitInstance->getNavit(), attr_route, &route, nullptr)) {
            struct attr destination_length, destination_time;
            char *distance=nullptr,*timeLeft=nullptr;
            if (route_get_attr(route.u.route, attr_destination_length, &destination_length, nullptr))
                distance=attr_to_text_ext(&destination_length, nullptr, attr_format_with_units, attr_format_default, nullptr);
            if (route_get_attr(route.u.route, attr_destination_time, &destination_time, nullptr))
                timeLeft=attr_to_text_ext(&destination_time, nullptr, attr_format_with_units, attr_format_default, nullptr);
            QStringList timeLeftArr = QString(timeLeft).split(":");
            QDateTime dt = QDateTime::currentDateTime();
            QTime time;

            switch (timeLeftArr.size()) {
            case 4:
                dt = dt.addDays(timeLeftArr[0].toInt());
                time.setHMS(timeLeftArr[1].toInt(),timeLeftArr[2].toInt(),timeLeftArr[3].toInt());
                break;
            case 3:
                time.setHMS(timeLeftArr[0].toInt(),timeLeftArr[1].toInt(),timeLeftArr[2].toInt());
                break;
            case 2:
                time.setHMS(0,timeLeftArr[0].toInt(),timeLeftArr[1].toInt());
                break;
            case 1:
                time.setHMS(0,0,timeLeftArr[0].toInt());
                break;
            }

            dt = dt.addMSecs(time.msecsSinceStartOfDay());

            m_distance = distance;
            m_timeLeft = timeLeft;
            m_arrivalTime = dt.toString("hh:mm");
        }

        m_directions.clear();
        while((item = map_rect_get_item(mr))) {
            if(item_attr_get(item,attr_navigation_long,&attr)) {
                m_directions << map_convert_string_tmp(item->map,attr.u.str);
            }
        }
        emit propertiesChanged();
    }
    map_rect_destroy(mr);

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

    statusUpdate();

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
    navigation_get_attr(nav, attr_nav_status, &attr,nullptr);
    if(m_status != attr.u.num){
        m_status = static_cast<Status>(attr.u.num);
        emit statusChanged();
    }
}
void NavitRoute::routeCallbackHandler(NavitRoute * navitRoute){
    navitRoute->routeUpdate();
}
void NavitRoute::destinationCallbackHandler(NavitRoute * navitRoute){
    navitRoute->destinationUpdate();
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

#include "navitpoimodel.h"

NavitPOIModel::NavitPOIModel(QObject *parent)
{

}

QHash<int, QByteArray> NavitPOIModel::roleNames() const{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[TypeRole] = "type";
    roles[DistanceRole] = "distance";
    roles[IconRole] = "icon";
    roles[CoordinatesRole] = "coordinates";
    roles[AddressRole] = "address";
    return roles;
}

QVariant NavitPOIModel::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() >= m_pois.count())
        return QVariant();

    const QVariantMap *poi = &m_pois.at(index.row());

    if (role == NameRole)
        return poi->value("name");
    if (role == TypeRole)
        return poi->value("type");
    if (role == DistanceRole)
        return poi->value("distance");
    if (role == IconRole)
        return poi->value("icon");
    if (role == CoordinatesRole)
        return poi->value("coords");
    if (role == AddressRole)
        return poi->value("address");

    return QVariant();
}


int NavitPOIModel::rowCount(const QModelIndex & parent) const {
    return m_pois.count();
}

Qt::ItemFlags NavitPOIModel::flags(const QModelIndex &index) const {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool NavitPOIModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    return false;
}

QModelIndex NavitPOIModel::index(int row, int column, const QModelIndex &parent) const {
    return createIndex(row, column);
}

QModelIndex NavitPOIModel::parent(const QModelIndex &child) const {
    return QModelIndex();
}

int NavitPOIModel::columnCount(const QModelIndex &parent) const {
    return 0;
}

QString NavitPOIModel::getAddressString(struct item *item, int prependPostal) {
    struct attr attr;
    QStringList address;

    item_attr_rewind(item);
    if(prependPostal && item_attr_get(item, attr_postal, &attr))
        address << map_convert_string_tmp(item->map,attr.u.str);
    if(item_attr_get(item, attr_house_number, &attr))
        address << map_convert_string_tmp(item->map,attr.u.str);
    if(item_attr_get(item, attr_street_name, &attr))
        address << map_convert_string_tmp(item->map,attr.u.str);
    if(item_attr_get(item, attr_street_name_systematic, &attr))
        address << map_convert_string_tmp(item->map,attr.u.str);
    if(item_attr_get(item, attr_district_name, &attr))
        address << map_convert_string_tmp(item->map,attr.u.str);
    if(item_attr_get(item, attr_town_name, &attr))
        address << map_convert_string_tmp(item->map,attr.u.str);
    if(item_attr_get(item, attr_county_name, &attr))
        address << map_convert_string_tmp(item->map,attr.u.str);
    if(item_attr_get(item, attr_country_name, &attr))
        address << map_convert_string_tmp(item->map,attr.u.str);
    if(item_attr_get(item, attr_address, &attr))
        address << " | " << map_convert_string_tmp(item->map,attr.u.str);

    return address.join(" ");
}
static void format_dist(int dist, char *distbuf) {
    if (dist > 10000)
        sprintf(distbuf,"%d ", dist/1000);
    else if (dist>0)
        sprintf(distbuf,"%d.%d ", dist/1000, (dist%1000)/100);
}
void NavitPOIModel::search(double lng, double lat, QString filter){
    if(m_navitInstance){
        struct coord d;
        struct coord_geo g;

        struct pcoord new_center;
        new_center.pro = transform_get_projection(navit_get_trans(m_navitInstance->getNavit()));

        g.lng = lng;
        g.lat = lat;
        transform_from_geo(new_center.pro, &g, &d);
        new_center.x = d.x;
        new_center.y = d.y;

        struct map_selection * sel, * selm;
        struct coord c, center;
        struct mapset_handle * h;
        struct map * m;
        struct map_rect * mr;
        struct item * item;
        int idist;
        int dist;
        dist = 10000;
        sel = map_selection_rect_new(&(new_center), dist * transform_scale(abs(new_center.y) + dist * 1.5), 18);

        m_pois.clear();
        m_poiTypes.clear();

        dbg(lvl_debug, "center is at %x, %x", center.x, center.y);

        h = mapset_open(navit_get_mapset(m_navitInstance->getNavit()));
        while ((m = mapset_next(h, 1))) {
            selm = map_selection_dup_pro(sel, new_center.pro, map_projection(m));
            mr = map_rect_new(m, selm);
            dbg(lvl_debug, "mr=%p", mr);
            if (mr) {
                while ((item = map_rect_get_item(mr))) {
                    if(!filter.isEmpty() && filter != item_to_name(item->type)){
                        continue;
                    }
                    if ( item_is_poi(*item) &&
                         item_coord_get_pro(item, &c, 1, new_center.pro) &&
                         coord_rect_contains(&sel->u.c_rect, &c)  &&
                         (idist=transform_distance(new_center.pro, &center, &c)) < dist) {

                        struct attr attr;
                        char * label;
                        char * icon = get_icon(m_navitInstance->getNavit(), item);
                        struct pcoord item_coord;
                        item_coord.pro = transform_get_projection(navit_get_trans(m_navitInstance->getNavit()));
                        item_coord.x = c.x;
                        item_coord.y = c.y;

                        idist = transform_distance(new_center.pro, &center, &c);
                        if (item_attr_get(item, attr_label, &attr)) {
                            label = map_convert_string(item->map, attr.u.str);

                            if (icon) {

                                if(!m_poiTypes.contains(item_to_name(item->type))){
                                    m_poiTypes << item_to_name(item->type);
                                }
                                char distStr[32]="";
                                format_dist(idist, distStr);

                                qDebug () << distStr;
                                QString address = getAddressString(item,0);

                                QVariantMap coords;
                                coords.insert("x", item_coord.x);
                                coords.insert("y", item_coord.y);
                                coords.insert("pro", item_coord.pro);
                                QVariantMap poi;
                                poi.insert("name", label);
                                poi.insert("type", item_to_name(item->type));
                                poi.insert("distance", idist);
                                poi.insert("icon", icon);
                                poi.insert("coords", coords);
                                poi.insert("address", address);

                                beginInsertRows(QModelIndex(), rowCount(), rowCount());
                                m_pois.append(poi);
                                endInsertRows();
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
    }
}

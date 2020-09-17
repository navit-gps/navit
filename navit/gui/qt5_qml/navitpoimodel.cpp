#include "navitpoimodel.h"

POISearchWorker::POISearchWorker (NavitInstance * navit) :
    m_navitInstance(navit),
    m_running(false), m_mutex(){

}
POISearchWorker::~POISearchWorker (){
    qDebug() << "Deleting POISearchWorker";
}
void POISearchWorker::setParameters(QString filter, int screenX, int screenY, int distance){
    m_filter = filter;
    m_screenX = screenX;
    m_screenY = screenY;
    m_distance = distance;
}
void POISearchWorker::run()
{
    struct point p;
    struct transformation * trans;

    struct map_selection * sel, * selm;
    struct coord c, center;
    struct pcoord pcenter;
    struct mapset_handle * h;
    struct map * m;
    struct map_rect * mr;
    struct item * item;

    enum projection pro;
    int idist = 0;

    if(m_screenX < 0) {
        m_screenX = navit_get_width(m_navitInstance->getNavit()) / 2;
        m_screenY = navit_get_height(m_navitInstance->getNavit()) / 2;
    }

    p.x = m_screenX;
    p.y = m_screenY;

    trans = navit_get_trans(m_navitInstance->getNavit());
    pro = transform_get_projection(trans);
    transform_reverse(trans, &p, &center);

    pcenter.x = center.x;
    pcenter.y = center.y;
    pcenter.pro = pro;

    int distanceSel =  m_distance * transform_scale(abs(center.y) + m_distance * 1.5);
    qDebug() << "distanceSel : " << distanceSel;
    sel = map_selection_rect_new(&(pcenter), distanceSel, 18);

    dbg(lvl_debug, "screenX : %d screenY : %d center is at %x, %x", m_screenX, m_screenY, center.x, center.y);

    h = mapset_open(navit_get_mapset(m_navitInstance->getNavit()));
    m_mutex.lock();
    m_running = true;
    while ((m = mapset_next(h, 1)) && m_running) {
        selm = map_selection_dup_pro(sel, pro, map_projection(m));
        mr = map_rect_new(m, selm);
        dbg(lvl_debug, "mr=%p", mr);
        if (mr) {
            //            QMutexLocker locker(&m_mutex);

            while ((item = map_rect_get_item(mr)) && m_running) {
                if(!m_filter.isEmpty() && m_filter != item_to_name(item->type)){
                    continue;
                }

                if ( item_is_poi(*item) &&
                     item_coord_get_pro(item, &c, 1, pro) &&
                     coord_rect_contains(&sel->u.c_rect, &c) &&
                     (idist = transform_distance(pro, &center, &c))/* < m_distance*/) {

                    item_attr_rewind(item);
                    struct attr attr;
                    char * name;
                    char * icon = get_icon(m_navitInstance->getNavit(), item);

                    if (item_attr_get(item, attr_label, &attr)) {
                        name = map_convert_string(item->map, attr.u.str);

                        QString address = NavitHelper::getAddress(m_navitInstance, c,"");
                        QString label = QString("%0, %1").arg(name).arg(address);
                        QVariantMap coords;
                        coords.insert("x", c.x);
                        coords.insert("y", c.y);
                        coords.insert("pro", pro);
                        QVariantMap poi;
                        poi.insert("name", name);
                        poi.insert("type", item_to_name(item->type));
                        poi.insert("distance", idist);
                        poi.insert("icon", icon);
                        poi.insert("coords", coords);
                        poi.insert("address", address);
                        poi.insert("label", label);
                        emit gotSearchResult(poi);
                    }
                }
            }
            map_rect_destroy(mr);
        }
        map_selection_destroy(selm);
    }

    map_selection_destroy(sel);
    mapset_close(h);
    m_mutex.unlock();
}
void POISearchWorker::stop() {
    qDebug() << "Stopping worker";
    m_running = false;
    m_mutex.lock();
    m_mutex.unlock();
    qDebug() << "Worker stopped";
}

NavitPOIModel::NavitPOIModel(QObject *parent)
{
}

NavitPOIModel::~NavitPOIModel(){
    if(m_poiWorker){
        delete m_poiWorker;
    }
}

void NavitPOIModel::setNavit(NavitInstance * navit){
    m_navitInstance = navit;
    m_poiWorker = new POISearchWorker(m_navitInstance);
    m_poiWorker->setAutoDelete(false);
    connect(m_poiWorker, &POISearchWorker::gotSearchResult, this, &NavitPOIModel::receiveSearchResult);
}

QHash<int, QByteArray> NavitPOIModel::roleNames() const{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[TypeRole] = "type";
    roles[DistanceRole] = "distance";
    roles[IconRole] = "icon";
    roles[CoordinatesRole] = "coords";
    roles[AddressRole] = "address";
    roles[LabelRole] = "label";
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
    if (role == LabelRole)
        return poi->value("label");

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

void NavitPOIModel::search(QString filter, int screenX, int screenY, int distance){
    if(m_poiWorker){
        m_poiWorker->blockSignals(true);
        m_poiWorker->stop();
        qDebug() << "clearing pois";

        beginResetModel();
        m_pois.clear();
        m_poiTypes.clear();
        endResetModel();

        m_poiWorker->setParameters(filter, screenX, screenY, distance);

        m_poiWorker->blockSignals(false);
        QThreadPool::globalInstance()->start(m_poiWorker);
    }
}
//void NavitPOIModel::receiveSearchResult(QVariantMap poi){
//    qDebug() << "\t" << poi.value("name").toString();


//    //                            if(!m_poiTypes.contains(item_to_name(item->type))){
//    //                                m_poiTypes << item_to_name(item->type);
//    //                            }

////    QVariantMap tmpPoi(poi);
//    int poiDist = poi.value("distM").toInt();
////    tmpPoi.insert("distance", NavitHelper::formatDist(poiDist));
//    if(m_pois.size() == 0){
//        beginInsertRows(QModelIndex(), 0, 0);
//        m_pois.append(poi);
//        endInsertRows();
//    }
//    for(int i = 0; i < m_pois.length(); i++){
//        int listDist = m_pois.at(i).value("distance").toInt();
//        if(poiDist >= listDist ){
//            continue;
//        }
//        beginInsertRows(QModelIndex(), i, i);
//        m_pois.append(poi);
//        endInsertRows();
//    }
//}
void NavitPOIModel::receiveSearchResult(QVariantMap poi){
    modelMutex.lock();
//    qDebug() << "\t" << poi.value("name").toString() << "\t" << poi.value("distance").toInt();


    //                            if(!m_poiTypes.contains(item_to_name(item->type))){
    //                                m_poiTypes << item_to_name(item->type);
    //                            }

    int index = 0;
    int poiDist = poi.value("distance").toInt();
    for(; index < m_pois.length(); index++){
        int itemDist = m_pois.at(index).value("distance").toInt();
        if(poiDist < itemDist){
            break;
        }
    }

    beginInsertRows(QModelIndex(), index, index);
    m_pois.insert(index, poi);
    endInsertRows();
    modelMutex.unlock();
}

void NavitPOIModel::setAsDestination(int index){
    if(m_pois.size() > index){
        NavitHelper::setDestination(m_navitInstance,
                                    m_pois[index]["label"].toString(),
                                    m_pois[index]["coords"].toMap()["x"].toInt(),
                                    m_pois[index]["coords"].toMap()["y"].toInt());
    }
}

void NavitPOIModel::setAsPosition(int index){
    if(m_pois.size() > index){
        NavitHelper::setPosition(m_navitInstance,
                                 m_pois[index]["coords"].toMap()["x"].toInt(),
                                 m_pois[index]["coords"].toMap()["y"].toInt());
    }
}

void NavitPOIModel::addAsBookmark(int index){
    if(m_pois.size() > index){
        NavitHelper::addBookmark(m_navitInstance,
                                 m_pois[index]["label"].toString(),
                                 m_pois[index]["coords"].toMap()["x"].toInt(),
                                 m_pois[index]["coords"].toMap()["y"].toInt());
    }
}

void NavitPOIModel::addStop(int index,  int position){
    if(m_pois.size() > index){
        NavitHelper::addStop(m_navitInstance,
                             position,
                                    m_pois[index]["label"].toString(),
                                    m_pois[index]["coords"].toMap()["x"].toInt(),
                                    m_pois[index]["coords"].toMap()["y"].toInt());
    }
}

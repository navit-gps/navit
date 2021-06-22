#include "navitpoimodel.h"

POISearchWorker::POISearchWorker (NavitInstance * navit, QString filter, int screenX, int screenY, int distance) :
    m_navitInstance(navit),
    m_filter(filter),
    m_screenX(screenX),
    m_screenY(screenY),
    m_distance(distance){

    struct point p;
    struct transformation * trans;

    struct coord center;
    struct pcoord pcenter;

    if(m_screenX < 0) {
        m_screenX = navit_get_width(m_navitInstance->getNavit()) / 2;
        m_screenY = navit_get_height(m_navitInstance->getNavit()) / 2;
    }

    p.x = m_screenX;
    p.y = m_screenY;

    trans = navit_get_trans(m_navitInstance->getNavit());
    m_pro = transform_get_projection(trans);
    transform_reverse(trans, &p, &center);

    pcenter.x = center.x;
    pcenter.y = center.y;
    pcenter.pro = m_pro;

    int distanceSel =  m_distance * transform_scale(abs(center.y) + m_distance * 1.5);
    qDebug() << "distanceSel : " << distanceSel;
    m_sel = map_selection_rect_new(&(pcenter), distanceSel, 18);

    dbg(lvl_debug, "screenX : %d screenY : %d center is at %x, %x", m_screenX, m_screenY, center.x, center.y);

    m_h = mapset_open(navit_get_mapset(m_navitInstance->getNavit()));
}

POISearchWorker::~POISearchWorker (){
    qDebug() << "Deleting POISearchWorker";
    finish();

    if (m_idleCallback) {
        callback_destroy(m_idleCallback);
        m_idleCallback=nullptr;
    }
}
void POISearchWorker::start(){
    this->m_idleCallback=callback_new_1(callback_cast(POISearchWorker::callbackHandler), this);
    this->m_idle=event_add_idle(100,this->m_idleCallback);
}

void POISearchWorker::callbackHandler(POISearchWorker this_){
    this_.proccess();
}
void POISearchWorker::proccess(){
    if(!m_idle || !m_idleCallback){
        qDebug() << "m_idle or m_idleCallbackis null returning";
        return;
    }
    if(!m_mr){
        qDebug() << "m_mr is null, proccessMapset";
        proccessMapset();
    } else if(!m_item){
        qDebug() << "m_item is null, proccessMapset";
        map_rect_destroy(m_mr);
        proccessMapset();
    }

    if(m_mr){
        for(int i = 0; i < 100; i++){
            if(!m_item){
                proccessMapsetItem();
                return;
            }
        }
    } else {
        qDebug() << "mr is null, destroying";
        map_selection_destroy(m_sel);
        finish();
    }
}

void POISearchWorker::proccessMapset(){
    qDebug() << "    Processing mapset";
    m_m = mapset_next(m_h, 1);
    if(!m_m){
        qDebug() << "Mapset is null";
        m_selm = nullptr;
        m_mr = nullptr;
        return;
    }
    m_selm = map_selection_dup_pro(m_sel, m_pro, map_projection(m_m));
    m_mr = map_rect_new(m_m, m_sel);

    dbg(lvl_debug, "mr=%p", m_mr)
}


void POISearchWorker::proccessMapsetItem(){
//    qDebug() << "      Processing mapset Item";
    if(!m_mr){
        return;
    }
    m_item = map_rect_get_item(m_mr);
    if(!m_item){
        return;
    }
    if(!m_filter.isEmpty() && m_filter != item_to_name(m_item->type)){
        return;
    }
    struct coord c;
    int idist = 0;

    if ( item_is_poi(*m_item) &&
         item_coord_get_pro(m_item, &c, 1, m_pro) &&
         coord_rect_contains(&m_sel->u.c_rect, &c) &&
         (idist = transform_distance(m_pro, &m_center, &c))/* < m_distance*/) {

        item_attr_rewind(m_item);
        struct attr attr;
        char * name;
        char * icon = get_icon(m_navitInstance->getNavit(), m_item);

        if (item_attr_get(m_item, attr_label, &attr)) {
            name = map_convert_string(m_item->map, attr.u.str);

            if(m_item->type==type_poly_building && item_attr_get(m_item, attr_house_number, &attr) ) {
                if(strcmp(name,map_convert_string_tmp(m_item->map,attr.u.str))==0) {
                    g_free(name);
                    return;
                }
            }

            QString address = NavitHelper::getAddress(m_navitInstance, c,"");
            QString label = QString("%0, %1").arg(name).arg(address);
            QVariantMap coords;
            coords.insert("x", c.x);
            coords.insert("y", c.y);
            coords.insert("pro", m_pro);
            QVariantMap poi;
            poi.insert("name", name);
            poi.insert("type", item_to_name(m_item->type));
            poi.insert("distance", idist);
            poi.insert("icon", icon);
            poi.insert("coords", coords);
            poi.insert("address", address);
            poi.insert("label", label);
            emit gotSearchResult(poi);
        }
    }
}

void POISearchWorker::finish(){
    qDebug() << "Finishing worker";
    if(m_sel){
        map_selection_destroy(m_sel);
        m_sel=nullptr;
    }
    if(m_h){
        mapset_close(m_h);
        m_h=nullptr;
    }
    if (m_idle) {
        event_remove_idle(m_idle);
        m_idle=nullptr;
    }
    if (m_idleCallback){
        callback_destroy(m_idleCallback);
        m_idleCallback = nullptr;
    }
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

void NavitPOIModel::stopWorker(bool clearModel){
    if(m_poiWorker){
        m_poiWorker->finish();
    }

    if(clearModel){
        beginResetModel();
        m_pois.clear();
        m_poiTypes.clear();
        endResetModel();
    }
}
void NavitPOIModel::search(QString filter, int screenX, int screenY, int distance){
    stopWorker(true);

    if(m_poiWorker){
        delete (m_poiWorker);
        m_poiWorker = nullptr;
    }

    m_poiWorker = new POISearchWorker(m_navitInstance, filter, screenX, screenY, distance);
    connect(m_poiWorker, &POISearchWorker::gotSearchResult, this, &NavitPOIModel::receiveSearchResult);
    m_poiWorker->start();
}

void NavitPOIModel::receiveSearchResult(QVariantMap poi){
    modelMutex.lock();
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
        stopWorker();
        NavitHelper::setDestination(m_navitInstance,
                                    m_pois[index]["label"].toString(),
                                    m_pois[index]["coords"].toMap()["x"].toInt(),
                                    m_pois[index]["coords"].toMap()["y"].toInt());
        stopWorker(true);
    }
}

void NavitPOIModel::setAsPosition(int index){
    if(m_pois.size() > index){
        stopWorker();
        NavitHelper::setPosition(m_navitInstance,
                                 m_pois[index]["coords"].toMap()["x"].toInt(),
                                 m_pois[index]["coords"].toMap()["y"].toInt());
        stopWorker(true);
    }
}

void NavitPOIModel::addAsBookmark(int index){
    if(m_pois.size() > index){
        stopWorker();
        NavitHelper::addBookmark(m_navitInstance,
                                 m_pois[index]["label"].toString(),
                                 m_pois[index]["coords"].toMap()["x"].toInt(),
                                 m_pois[index]["coords"].toMap()["y"].toInt());
        stopWorker(true);
    }
}

void NavitPOIModel::addStop(int index,  int position){
    if(m_pois.size() > index){
        stopWorker();
        NavitHelper::addStop(m_navitInstance,
                             position,
                                    m_pois[index]["label"].toString(),
                                    m_pois[index]["coords"].toMap()["x"].toInt(),
                                    m_pois[index]["coords"].toMap()["y"].toInt());
        stopWorker(true);
    }
}

void NavitPOIModel::reset(){
    stopWorker(true);
}

#include "navitsearchmodel.h"

#include "event.h"

NavitSearchModel::NavitSearchModel(QObject *parent)
{
}


QHash<int, QByteArray> NavitSearchModel::roleNames() const{
    QHash<int, QByteArray> roles;
    roles[LabelRole] = "label";
    roles[NameRole] = "name";
    roles[IconRole] = "icon";
    roles[AddressRole] = "address";
    roles[DistanceRole] = "distance";

    return roles;
}

QVariant NavitSearchModel::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() >= m_searchResults.count())
        return QVariant();

    const QVariantMap *poi = &m_searchResults.at(index.row());

    if (role == LabelRole)
        return poi->value("label");
    else if (role == NameRole)
        return poi->value("label");
    else if (role == IconRole)
        return poi->value("icon");
    else if (role == AddressRole)
        return poi->value("address");
    else if (role == DistanceRole)
        return poi->value("distance");
    return QVariant();
}


int NavitSearchModel::rowCount(const QModelIndex & parent) const {
    return m_searchResults.count();
}

Qt::ItemFlags NavitSearchModel::flags(const QModelIndex &index) const {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool NavitSearchModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    return false;
}

QModelIndex NavitSearchModel::index(int row, int column, const QModelIndex &parent) const {
    return createIndex(row, column);
}

QModelIndex NavitSearchModel::parent(const QModelIndex &child) const {
    return QModelIndex();
}

int NavitSearchModel::columnCount(const QModelIndex &parent) const {
    return 0;
}

void NavitSearchModel::setNavit(NavitInstance * navit){
    m_navitInstance = navit;

    struct mapset * ms = navit_get_mapset(m_navitInstance->getNavit());
    m_searchResultList=search_list_new(ms);
    search_list_set_default_country();

    m_search_type = attr_town_or_district_name;
}

void NavitSearchModel::handleSearchResult(){
    struct search_list_result *res;

    res=search_list_get_result(m_searchResultList);
    if (!res) {
        idle_end();
        return;
    }


    QVariantMap result;
    QString label;
    QString icon;
    QStringList address;

    if (res->country) {
        label = g_strdup(res->country->name);
        icon = res->country->flag;
        address.append(label);
    }

    if (res->town) {
        label = g_strdup(res->town->common.town_name);
        icon = "icons/bigcity.png";
        address.append(label);
    }

    if (res->street) {
        label = g_strdup(res->street->name);
        icon = "icons/smallcity.png";
        address.append(label);
    }


    qDebug() << label;

    result.insert("label", label);
    result.insert("icon", icon);

    result.insert("address", address.join(", "));
    result.insert("distance", 1);

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_searchResults.append(result);
    endInsertRows();

    qDebug() << result["label"];

}
void NavitSearchModel::idle_cb(NavitSearchModel * searchModel){
    searchModel->handleSearchResult();
}

void NavitSearchModel::idle_start(){
    m_idle_cb = callback_new_1(callback_cast(idle_cb), this);
    m_event_idle = event_add_idle(50,m_idle_cb);
    callback_call_0(m_idle_cb);
}

void NavitSearchModel::idle_end(){
    if (m_event_idle != nullptr) {
        event_remove_idle(m_event_idle);
        m_event_idle=nullptr;
    }
    if (m_idle_cb != nullptr) {
        callback_destroy(m_idle_cb);
        m_idle_cb=nullptr;
    }
}

void NavitSearchModel::search2(QString queryText){
    struct search_list_result *res;

    struct attr attr;
    QString label;
    QString icon;


    if(!m_search_type){
        m_search_type = attr_country_all;
    }

    attr.u.str = queryText.toUtf8().data();
    attr.type = m_search_type;

    search_list_search(m_searchResultList, &attr, 1);
    int count = 0;

    beginResetModel();
    m_searchResults.clear();
    endResetModel();

    while((res=search_list_get_result(m_searchResultList))) {

        QVariantMap result;
        QStringList address;

        if (res->country) {
            label = g_strdup(res->country->name);
            icon = res->country->flag;
            address.append(label);
        }

        if (res->town) {
            label = g_strdup(res->town->common.town_name);
            icon = "icons/bigcity.png";
            address.append(label);
        }

        if (res->street) {
            label = g_strdup(res->street->name);
            icon = "icons/smallcity.png";
            address.append(label);
        }



        result.insert("label", label);
        result.insert("icon", icon);
        result.insert("address", address.join(", "));

        beginInsertRows(QModelIndex(), rowCount(), rowCount());
        m_searchResults.append(result);
        endInsertRows();

        if (count ++ > 50) {
            break;
        }
        qDebug() << icon << label;
    }
}

void NavitSearchModel::search(QString queryText){
    if(queryText == ""){
        return;
    }
    struct attr attr;

    idle_end();

    beginResetModel();
    m_searchResults.clear();
    endResetModel();

    attr.u.str = queryText.toLocal8Bit().data();
    attr.type = m_search_type;

    qDebug () << "searching : " << queryText.toLocal8Bit().data();
    search_list_search(m_searchResultList, &attr, 1);
    idle_start();
}

void NavitSearchModel::search_list_set_default_country() {
    struct attr search_attr, country_name, country_iso2, *country_attr;
    struct item *item;
    struct country_search *cs;
    struct tracking *tracking;
    struct search_list_result *res;

    country_attr=country_default();
    tracking=navit_get_tracking(m_navitInstance->getNavit());
    if (tracking && tracking_get_attr(tracking, attr_country_id, &search_attr, nullptr))
        country_attr=&search_attr;
    if (country_attr) {
        cs=country_search_new(country_attr, 0);
        item=country_search_get_item(cs);
        if (item && item_attr_get(item, attr_country_name, &country_name)) {
            search_attr.type=attr_country_all;
            qDebug() << "country " << country_name.u.str;
            search_attr.u.str=country_name.u.str;
            search_list_search(m_searchResultList, &search_attr, 0);
            while((res=search_list_get_result(m_searchResultList)));
            if(m_country_iso2) {
                g_free(m_country_iso2);
                this->m_country_iso2=nullptr;
            }
            if (item_attr_get(item, attr_country_iso2, &country_iso2))
                this->m_country_iso2=g_strdup(country_iso2.u.str);
        }
        country_search_destroy(cs);
    } else {
        qWarning() << "warning: no default country found";
        if (m_country_iso2) {
            qDebug() << "attempting to use country" << m_country_iso2;
            search_attr.type=attr_country_iso2;
            search_attr.u.str=m_country_iso2;
            search_list_search(m_searchResultList, &search_attr, 0);
            while((res=search_list_get_result(m_searchResultList)));
        }
    }
    search_list_select(m_searchResultList, attr_country_all, 0, 0);
}


void NavitSearchModel::select(int index){
    qDebug() << "Select : " << index;

    beginResetModel();
    m_searchResults.clear();
    endResetModel();

    search_list_select(m_searchResultList, m_search_type, 0, 0);
    search_list_select(m_searchResultList, m_search_type, index, 1);

    switch(m_search_type){
    case attr_country_all:
        m_search_type = attr_town_or_district_name;
        break;
    case attr_town_or_district_name:
        m_search_type = attr_street_name;
        break;
    case attr_street_name:
        m_search_type = attr_house_number;
        break;
    case attr_house_number:
        break;
    }
}

void NavitSearchModel::setAsDestination(int index){
    //    if(m_recents.size() > index){
    //        NavitHelper::setDestination(m_navitInstance,
    //                                    m_recents[index]["label"].toString(),
    //                                    m_recents[index]["coords"].toMap()["x"].toInt(),
    //                                    m_recents[index]["coords"].toMap()["y"].toInt());
    //    }
}

void NavitSearchModel::addStop(int index,  int position){
    //    if(m_recents.size() > index){
    //        NavitHelper::addStop(m_navitInstance,
    //                             position,
    //                                    m_recents[index]["label"].toString(),
    //                                    m_recents[index]["coords"].toMap()["x"].toInt(),
    //                                    m_recents[index]["coords"].toMap()["y"].toInt());
    //    }
}

void NavitSearchModel::setAsPosition(int index){
    //    if(m_recents.size() > index){
    //        NavitHelper::setPosition(m_navitInstance,
    //                                 m_recents[index]["coords"].toMap()["x"].toInt(),
    //                                 m_recents[index]["coords"].toMap()["y"].toInt());
    //    }
}

void NavitSearchModel::addAsBookmark(int index){
    //    if(m_recents.size() > index){
    //        NavitHelper::addBookmark(m_navitInstance,
    //                                 m_recents[index]["label"].toString(),
    //                                 m_recents[index]["coords"].toMap()["x"].toInt(),
    //                                 m_recents[index]["coords"].toMap()["y"].toInt());
    //    }
}

void NavitSearchModel::remove(int index) {
    //    if(index < m_recents.size()){
    //        beginRemoveRows(QModelIndex(), index, index);
    //        m_recents.removeAt(index);
    //        endInsertRows();
    //    }
}

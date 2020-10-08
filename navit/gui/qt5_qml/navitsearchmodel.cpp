#include "navitsearchmodel.h"

#include "event.h"

SearchWorker::SearchWorker (NavitInstance * navit, search_list *searchResultList, QString searchQuery, enum attr_type search_type) :
    m_navitInstance(navit),
    m_searchResultList(searchResultList),
    m_searchQuery(searchQuery),
    m_search_type(search_type){

}

void SearchWorker::run()
{
    if(m_searchQuery == "" && m_search_type != attr_house_number)
        return;

    struct search_list_result *res;

    struct attr attr;
    QString label;
    QString icon;

    QByteArray qry2 = m_searchQuery.toLocal8Bit();
    attr.u.str = static_cast<char *>(malloc(qry2.size() + 1));
    memcpy ( attr.u.str, qry2.data(), qry2.size() + 1 );
    attr.type = static_cast<enum attr_type>(m_search_type);

    search_list_search(m_searchResultList, &attr, 1);

    int i=0;
    while((res=search_list_get_result(m_searchResultList))) {
        SearchResult result;
        QStringList addressString;
        Address address;

        if (res->country) {
            label = QString(res->country->name);
            icon = res->country->flag;
            addressString.append(label);
            address.country = label;
        }

        if (res->town) {
            QString district = "";
            label = res->town->common.town_name;
            if(res->town->common.district_name){
                district =  QString(" (%1)").arg(res->town->common.district_name);
                label.append(district);
            }

            icon = "icons/bigcity.png";

            if (res->town->common.postal_mask)
                addressString.append(res->town->common.postal_mask);
            else if (res->town->common.postal)
                addressString.append(res->town->common.postal);

            struct attr state_attr;
            if(attr_generic_get_attr(res->town->common.attrs,nullptr,attr_state_name,&state_attr,nullptr)){
                addressString.append(state_attr.u.str);
            }

            if(res->town->common.county_name){
                addressString.append(res->town->common.county_name);

            }
            struct attr municipality_attr;
            if(attr_generic_get_attr(res->town->common.attrs,nullptr,attr_municipality_name,&municipality_attr,nullptr)){
                addressString.append(municipality_attr.u.str);
            }

            addressString.append(label);

            address.town = label;
        }

        if (res->street) {
            label = QString(res->street->name);
            icon = "icons/smallcity.png";
            addressString.append(label);

            if (res->street->common.postal_mask)
                addressString.append(res->street->common.postal_mask);
            else if (res->street->common.postal)
                addressString.append(res->street->common.postal);

            address.street = label;
        }

        if (res->house_number) {
            QString houseNumber = QString(res->house_number->house_number);
            icon = "icons/smallcity.png";
            address.house = label;

            QStringList fullAddress;
            for(int i=addressString.size();i-->0;)
            {
                fullAddress << addressString[i];
            }
            label = QString("%0 %1").arg(houseNumber,fullAddress.join(", "));
            addressString.clear();
        }

        if(m_search_type != attr_country_all){
            memcpy(&result.coords, res->c, sizeof result.coords);
        }
        QStringList reverseAddrStr;
        while(!addressString.isEmpty()){
            reverseAddrStr << addressString.takeLast();
        }

        result.index = i;
        result.label = label;
        result.icon = icon;
        result.address = address;
        result.addressString = reverseAddrStr.join(", ");
        result.distance = "0";
        i++;
        emit gotSearchResult(result);

        if ( QThread::currentThread()->isInterruptionRequested() ) {
            break;
        }
    }
}

NavitSearchModel::NavitSearchModel(QObject *parent)
{
    connect(this, &NavitSearchModel::searchQueryChanged, this, &NavitSearchModel::search);
    qRegisterMetaType<SearchResult>("SearchResult");
}
NavitSearchModel::~NavitSearchModel()
{
    qDebug() << "Deleting NavitSearchModel";
    if(m_searchWorker){
        qDebug() << "Deleting m_searchWorker";
        delete m_searchWorker;
    }
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

    const SearchResult *poi = &m_searchResults.at(index.row());

    if (role == LabelRole)
        return poi->label;
    else if (role == NameRole)
        return poi->label;
    else if (role == IconRole)
        return poi->icon;
    else if (role == AddressRole)
        return poi->addressString;
    else if (role == DistanceRole)
        return poi->distance;
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

    changeSearchType(SearchTown);
}

void NavitSearchModel::receiveSearchResult(SearchResult result){
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_searchResults.append(result);
    endInsertRows();
}
void NavitSearchModel::changeSearchType(enum SearchType searchType, bool restoreQuery){
    beginResetModel();
    m_searchResults.clear();
    endResetModel();

    m_search_type = searchType;
    emit searchTypeChanged();

    if(restoreQuery){
        switch(m_search_type){
        case SearchCountry:
            m_searchQuery = m_searchQueryCountry;
            m_searchQueryTown = "";
            m_searchQueryStreet = "";
            m_address.address.country = "";
            m_address.address.town = "";
            m_address.address.street = "";
            m_address.address.house = "";
            break;
        case SearchTown:
            m_searchQuery = m_searchQueryTown;
            m_searchQueryStreet = "";
            m_address.address.town = "";
            m_address.address.street = "";
            m_address.address.house = "";
            break;
        case SearchStreet:
            m_searchQuery = m_searchQueryStreet;
            m_address.address.street = "";
            m_address.address.house = "";
            break;
        case SearchHouse:
            m_address.address.house = "";
            break;
        }

        emit searchQueryChanged();
        emit addressChanged();
    }
}

enum NavitSearchModel::SearchType NavitSearchModel::getSearchType(){
    return m_search_type;
}

void NavitSearchModel::stopSearchWorker(bool clearModel){
    if(m_searchWorker != nullptr){
        m_searchWorker->requestInterruption();
        m_searchWorker->wait();
        m_searchWorker = nullptr;
    }
    if(clearModel){
        beginResetModel();
        m_searchResults.clear();
        endResetModel();
    }
}

void NavitSearchModel::search(){
    stopSearchWorker(true);
    if(m_searchQuery == "" && m_search_type != SearchHouse){
        return;
    }

    m_searchWorker = new SearchWorker(m_navitInstance, m_searchResultList,m_searchQuery, static_cast<enum attr_type>(m_search_type));
    connect(m_searchWorker, &SearchWorker::gotSearchResult, this, &NavitSearchModel::receiveSearchResult);
    m_searchWorker->start();
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
//    search_list_select(m_searchResultList, attr_country_all, 0, 0);

    m_address.address.country = search_attr.u.str;
    emit addressChanged();

    m_search_type = SearchTown;
    searchTypeChanged();
}


void NavitSearchModel::select(int index){
    enum attr_type type;
    type = static_cast<enum attr_type>(m_search_type);

    search_list_select(m_searchResultList, type, 0, 0);
    search_list_select(m_searchResultList, type, index + 1, 1);

    SearchResult result =  m_searchResults.at(index);

    m_address.address = result.address;
    m_address.coords = result.coords;
    m_address.addressString = result.addressString;
    m_address.label = result.label;

    emit addressChanged();

    switch(m_search_type){
    case SearchCountry:
        changeSearchType(SearchTown, false);
        m_searchQueryCountry = m_searchQuery;
        break;
    case SearchTown:
        changeSearchType(SearchStreet, false);
        m_searchQueryTown = m_searchQuery;
        break;
    case SearchStreet:
        changeSearchType(SearchHouse, false);
        m_searchQueryStreet = m_searchQuery;
        break;
    case SearchHouse:
        setAsDestination(index);
        reset();
        break;
    }
    m_searchQuery = "";
    emit searchQueryChanged();
}

void NavitSearchModel::setAsDestination(int index){
    if(m_searchResults.size() > index){
        stopSearchWorker();
        qDebug() << "Setting destination : " << m_searchResults[index].addressString;
        NavitHelper::setDestination(m_navitInstance,
                                    m_searchResults[index].addressString,
                                    m_searchResults[index].coords.x,
                                    m_searchResults[index].coords.y);
        stopSearchWorker(true);
    }
}

void NavitSearchModel::addStop(int index,  int position){
    if(m_searchResults.size() > index){
        stopSearchWorker();
        NavitHelper::addStop(m_navitInstance,
                             position,
                             m_searchResults[index].addressString,
                             m_searchResults[index].coords.x,
                             m_searchResults[index].coords.y);
        stopSearchWorker(true);
    }
}

void NavitSearchModel::setAsPosition(int index){
    if(m_searchResults.size() > index){
        stopSearchWorker();
        NavitHelper::setPosition(m_navitInstance,
                                 m_searchResults[index].coords.x,
                                 m_searchResults[index].coords.y);
        stopSearchWorker(true);
    }
}

void NavitSearchModel::addAsBookmark(int index){
    if(m_searchResults.size() > index){
        stopSearchWorker();
        NavitHelper::addBookmark(m_navitInstance,
                                 m_searchResults[index].addressString,
                                 m_searchResults[index].coords.x,
                                 m_searchResults[index].coords.y);
        stopSearchWorker(true);
    }
}
void NavitSearchModel::setAddressAsDestination(){
    stopSearchWorker();
    NavitHelper::setDestination(m_navitInstance,
                                m_address.addressString,
                                m_address.coords.x,
                                m_address.coords.y);
    stopSearchWorker(true);
}
void NavitSearchModel::setAddressAsPosition(){
    stopSearchWorker();
    NavitHelper::setPosition(m_navitInstance,
                             m_address.coords.x,
                             m_address.coords.y);
    stopSearchWorker(true);
}
void NavitSearchModel::addAddressAsBookmark(){
    stopSearchWorker();
    NavitHelper::addBookmark(m_navitInstance,
                             m_address.addressString,
                             m_address.coords.x,
                             m_address.coords.y);
    stopSearchWorker(true);
}
void NavitSearchModel::addAddressStop(int position){
    stopSearchWorker();
    NavitHelper::addStop(m_navitInstance,
                         position,
                         m_address.addressString,
                         m_address.coords.x,
                         m_address.coords.y);
    stopSearchWorker(true);
}
void NavitSearchModel::remove(int index) {
    //    if(index < m_recents.size()){
    //        beginRemoveRows(QModelIndex(), index, index);
    //        m_recents.removeAt(index);
    //        endInsertRows();
    //    }
}
void NavitSearchModel::viewAddressOnMap(){
    viewOnMap(m_address);
}
void NavitSearchModel::viewResultOnMap(int index){
    SearchResult result =  m_searchResults.at(index);
    viewOnMap(result);
}

void NavitSearchModel::viewOnMap(SearchResult pointMap) {
    QList<SearchResult> resultToMap;
    resultToMap << pointMap;
    searchResultsToMap(resultToMap);

    struct pcoord pc;
    pc.x = pointMap.coords.x;
    pc.y = pointMap.coords.y;
    pc.pro = pointMap.coords.pro;

    navit_set_center(m_navitInstance->getNavit(), &pc, 1);
}

void NavitSearchModel::searchResultsToMap(QList<SearchResult> results){
    GList* list = nullptr;
    GList* p = nullptr;
    int results_map_population = 0;
    struct attr a;

    for (int i = 0; i < results.size(); ++i) {
        SearchResult result = results.at(i);

        struct lcoord coords;
        coords.c.x=result.coords.x;
        coords.c.y=result.coords.y;
        coords.label=strdup(result.addressString.toLocal8Bit());
        list = g_list_prepend(list, &coords);
        qDebug() << QString("Adding %1 to the map x : %2 y : %3").arg(coords.label).arg(coords.c.x).arg(coords.c.y);
    }

    results_map_population = navit_populate_search_results_map(m_navitInstance->getNavit(), list, nullptr);

    if (list) {
        for(p=list; p; p=g_list_next(p)) {
            if (((struct lcoord *)(p->data))->label)
                g_free(((struct lcoord *)(p->data))->label);
        }
    }
    g_list_free(list);
    if(!results_map_population)
        return;

    a.type=attr_orientation;
    a.u.num=0;
    navit_set_attr(m_navitInstance->getNavit(),&a);	/* Set orientation to North */
}

void NavitSearchModel::reset(){
    stopSearchWorker(true);

    m_search_type = SearchTown;
    searchTypeChanged();

    m_searchQuery = "";
    searchQueryChanged();

    m_address = SearchResult();
    addressChanged();

    search_list_set_default_country();
}

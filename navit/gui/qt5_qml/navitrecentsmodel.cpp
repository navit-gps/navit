#include "navitrecentsmodel.h"

NavitRecentsModel::NavitRecentsModel(QObject *parent)
{

}

QHash<int, QByteArray> NavitRecentsModel::roleNames() const{
    QHash<int, QByteArray> roles;
    roles[LabelRole] = "label";
    return roles;
}

QVariant NavitRecentsModel::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() >= m_recents.count())
        return QVariant();

    const QVariantMap *poi = &m_recents.at(index.row());

    if (role == LabelRole)
        return poi->value("label");
    return QVariant();
}


int NavitRecentsModel::rowCount(const QModelIndex & parent) const {
    return m_recents.count();
}

Qt::ItemFlags NavitRecentsModel::flags(const QModelIndex &index) const {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool NavitRecentsModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    return false;
}

QModelIndex NavitRecentsModel::index(int row, int column, const QModelIndex &parent) const {
    return createIndex(row, column);
}

QModelIndex NavitRecentsModel::parent(const QModelIndex &child) const {
    return QModelIndex();
}

int NavitRecentsModel::columnCount(const QModelIndex &parent) const {
    return 0;
}

void NavitRecentsModel::setNavit(NavitInstance * navit){
    m_navitInstance = navit;
    update();
}
void NavitRecentsModel::update() {
    if(m_navitInstance){
        struct map *formerdests;
        struct map_rect *mr_formerdests;
        struct item *item;
        struct attr attr;
        char *label_full;
        enum projection projection;

        if(!navit_get_attr(m_navitInstance->getNavit(), attr_former_destination_map, &attr, nullptr))
            return;

        formerdests=attr.u.map;
        if(!formerdests)
            return;

        mr_formerdests=map_rect_new(formerdests, nullptr);
        if(!mr_formerdests)
            return;

        projection = map_projection(formerdests);

        while ((item=map_rect_get_item(mr_formerdests))) {
            struct coord c;
            if (item->type!=type_former_destination) continue;
            if (!item_attr_get(item, attr_label, &attr)) continue;

            label_full=attr.u.str;

            if (item_coord_get(item, &c, 1)) {
                QVariantMap coords;
                coords.insert("x", c.x);
                coords.insert("y", c.y);
                coords.insert("pro", projection);

                QVariantMap recentItem;
                recentItem.insert("coords",coords);
                recentItem.insert("label",label_full);

                beginInsertRows(QModelIndex(), 0, 0);
                m_recents.prepend(recentItem);
                endInsertRows();
            }
        }
        map_rect_destroy(mr_formerdests);
    }
}

void NavitRecentsModel::setAsDestination(int index){
    if(m_recents.size() > index){
        NavitHelper::setDestination(m_navitInstance,
                                    m_recents[index]["label"].toString(),
                                    m_recents[index]["coords"].toMap()["x"].toInt(),
                                    m_recents[index]["coords"].toMap()["y"].toInt());
    }
}

void NavitRecentsModel::addStop(int index,  int position){
    if(m_recents.size() > index){
        NavitHelper::addStop(m_navitInstance,
                             position,
                                    m_recents[index]["label"].toString(),
                                    m_recents[index]["coords"].toMap()["x"].toInt(),
                                    m_recents[index]["coords"].toMap()["y"].toInt());
    }
}

void NavitRecentsModel::setAsPosition(int index){
    if(m_recents.size() > index){
        NavitHelper::setPosition(m_navitInstance,
                                 m_recents[index]["coords"].toMap()["x"].toInt(),
                                 m_recents[index]["coords"].toMap()["y"].toInt());
    }
}

void NavitRecentsModel::addAsBookmark(int index){
    if(m_recents.size() > index){
        NavitHelper::addBookmark(m_navitInstance,
                                 m_recents[index]["label"].toString(),
                                 m_recents[index]["coords"].toMap()["x"].toInt(),
                                 m_recents[index]["coords"].toMap()["y"].toInt());
    }
}

void NavitRecentsModel::remove(int index) {
    if(index < m_recents.size()){
        beginRemoveRows(QModelIndex(), index, index);
        m_recents.removeAt(index);
        endInsertRows();
    }
}

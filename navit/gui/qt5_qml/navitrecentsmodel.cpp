#include "navitrecentsmodel.h"

NavitRecentsModel::NavitRecentsModel(QObject *parent)
{

}
QHash<int, QByteArray> NavitRecentsModel::roleNames() const{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[CoordXRole] = "coordX";
    roles[CoordYRole] = "coordY";
    roles[CoordProjectionRole] = "coordProjection";
    return roles;
}

QVariant NavitRecentsModel::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() >= m_recents.count())
        return QVariant();

    const QVariantMap *poi = &m_recents.at(index.row());

    if (role == NameRole)
        return poi->value("name");
    if (role == CoordXRole)
        return poi->value("coordX");
    if (role == CoordYRole)
        return poi->value("coordY");
    if (role == CoordProjectionRole)
        return poi->value("coordProjection");

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
                QVariantMap recentItem;
                recentItem.insert("coordX",c.x);
                recentItem.insert("coordY",c.y);
                recentItem.insert("coordProjection",projection);
                recentItem.insert("name",label_full);

                beginInsertRows(QModelIndex(), rowCount(), rowCount());
                m_recents.append(recentItem);
                endInsertRows();
            }
        }
        map_rect_destroy(mr_formerdests);
    }
}

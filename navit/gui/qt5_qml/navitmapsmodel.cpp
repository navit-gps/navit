#include "navitmapsmodel.h"

NavitMapsModel::NavitMapsModel(QObject *parent)
{

}

QHash<int, QByteArray> NavitMapsModel::roleNames() const{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[ActionRole] = "action";
    roles[ImageUrlRole] = "imageUrl";
    return roles;
}

QVariant NavitMapsModel::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() >= m_maps.count())
        return QVariant();

    const QVariantMap *poi = &m_maps.at(index.row());

    if (role == NameRole)
        return poi->value("name");
    else if (role == ActionRole)
        return poi->value("action");
    else if (role == ImageUrlRole)
        return poi->value("imageUrl");
    return QVariant();
}


int NavitMapsModel::rowCount(const QModelIndex & parent) const {
    return m_maps.count();
}

Qt::ItemFlags NavitMapsModel::flags(const QModelIndex &index) const {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool NavitMapsModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    return false;
}

QModelIndex NavitMapsModel::index(int row, int column, const QModelIndex &parent) const {
    return createIndex(row, column);
}

QModelIndex NavitMapsModel::parent(const QModelIndex &child) const {
    return QModelIndex();
}

int NavitMapsModel::columnCount(const QModelIndex &parent) const {
    return 0;
}

void NavitMapsModel::setNavit(NavitInstance * navit){
    m_navitInstance = navit;
    update();
}

QString NavitMapsModel::getMapLabel(struct map * map){
    struct attr description, type, data;
    QStringList nameList;
    QString name;
    if (map_get_attr(map, attr_description, &description, nullptr)) {
        name = description.u.str;
    } else {
        if(map_get_attr(map, attr_type, &type, nullptr)){
            nameList << type.u.str;
        }
        if(map_get_attr(map, attr_data, &data, nullptr)){
            nameList << data.u.str;
        }
        name = nameList.join(":");
    }
    return name;
}
void NavitMapsModel::update() {
    if(m_navitInstance){
        struct attr attr, active;
        struct attr_iter *iter;

        beginResetModel();
        m_maps.clear();
        endResetModel();

        iter=navit_attr_iter_new(nullptr);

        while(navit_get_attr(m_navitInstance->getNavit(), attr_map, &attr, iter)) {

            QVariantMap maps;
            //TODO : Internal GUI has option for download? gui_internal_cmd_map_download

            maps.insert("name",getMapLabel(attr.u.map));
            maps.insert("action", "toggleMap");

            map_get_attr(attr.u.map, attr_active, &active, nullptr);
            if (active.u.num){
                maps.insert("imageUrl", "qrc:/themes/Levy/assets/ionicons/md-checkmark-circle-outline.svg");
            } else {
                maps.insert("imageUrl", "");
            }

            beginInsertRows(QModelIndex(), rowCount(), rowCount());
            m_maps.append(maps);
            endInsertRows();

        }
        navit_attr_iter_destroy(iter);
    }
}

void NavitMapsModel::toggleMap(QString name){
    if(m_navitInstance){
        struct attr attr, active, activeSet;
        struct attr_iter *iter;

        iter=navit_attr_iter_new(nullptr);

        while(navit_get_attr(m_navitInstance->getNavit(), attr_map, &attr, iter)) {
            if(getMapLabel(attr.u.map) == name){
                map_get_attr(attr.u.map, attr_active, &active, nullptr);
                activeSet.type = attr_active;
                activeSet.u.num = 1 - active.u.num;
                map_set_attr(attr.u.map, &activeSet);
                navit_draw(m_navitInstance->getNavit());
                break;
            }
        }
        navit_attr_iter_destroy(iter);
        update();
    }
}

#include "navitvehiclesmodel.h"

NavitVehiclesModel::NavitVehiclesModel(QObject *parent)
{

}

QHash<int, QByteArray> NavitVehiclesModel::roleNames() const{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[ActionRole] = "action";
    roles[ImageUrlRole] = "imageUrl";
    return roles;
}

QVariant NavitVehiclesModel::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() >= m_vehicles.count())
        return QVariant();

    const QVariantMap *poi = &m_vehicles.at(index.row());

    if (role == NameRole)
        return poi->value("name");
    else if (role == ActionRole)
        return poi->value("action");
    else if (role == ImageUrlRole)
        return poi->value("imageUrl");
    return QVariant();
}


int NavitVehiclesModel::rowCount(const QModelIndex & parent) const {
    return m_vehicles.count();
}

Qt::ItemFlags NavitVehiclesModel::flags(const QModelIndex &index) const {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool NavitVehiclesModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    return false;
}

QModelIndex NavitVehiclesModel::index(int row, int column, const QModelIndex &parent) const {
    return createIndex(row, column);
}

QModelIndex NavitVehiclesModel::parent(const QModelIndex &child) const {
    return QModelIndex();
}

int NavitVehiclesModel::columnCount(const QModelIndex &parent) const {
    return 0;
}

void NavitVehiclesModel::setNavit(NavitInstance * navit){
    m_navitInstance = navit;
    update();
}
void NavitVehiclesModel::update() {
    if(m_navitInstance){
        struct attr attr;
        struct attr_iter *iter;

        beginResetModel();
        m_vehicles.clear();
        endResetModel();

        iter=navit_attr_iter_new(nullptr);
        struct vehicleprofile *vehicleprofile = navit_get_vehicleprofile(m_navitInstance->getNavit());
        QString activeVehicle = "";
        if(vehicleprofile){
            activeVehicle = QString::fromLocal8Bit(vehicleprofile->name);
        }
        while(navit_get_attr(m_navitInstance->getNavit(), attr_vehicleprofile, &attr, iter)) {
            QVariantMap vehicles;
            QString vehicle = QString::fromLocal8Bit(attr.u.vehicleprofile->name);
            vehicles.insert("name",vehicle);
            vehicles.insert("action", "setVehicle");
            if(activeVehicle == vehicle){
                vehicles.insert("imageUrl", "qrc:/themes/Levy/assets/ionicons/md-checkmark-circle-outline.svg");
            } else {
                vehicles.insert("imageUrl", "");
            }

            beginInsertRows(QModelIndex(), rowCount(), rowCount());
            m_vehicles.append(vehicles);
            endInsertRows();
        }
        navit_attr_iter_destroy(iter);
    }
}

void NavitVehiclesModel::setVehicle(QString name){
    if(m_navitInstance){
        navit_set_vehicleprofile_name(m_navitInstance->getNavit(), name.toUtf8().data());
        update();
    }
}

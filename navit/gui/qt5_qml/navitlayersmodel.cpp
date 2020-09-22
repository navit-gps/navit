#include "navitlayersmodel.h"

NavitLayersModel::NavitLayersModel(QObject *parent)
{

}

QHash<int, QByteArray> NavitLayersModel::roleNames() const{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[ActionRole] = "action";
    roles[ImageUrlRole] = "imageUrl";
    return roles;
}

QVariant NavitLayersModel::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() >= m_layers.count())
        return QVariant();

    const QVariantMap *poi = &m_layers.at(index.row());

    if (role == NameRole)
        return poi->value("name");
    else if (role == ActionRole)
        return poi->value("action");
    else if (role == ImageUrlRole)
        return poi->value("imageUrl");
    return QVariant();
}


int NavitLayersModel::rowCount(const QModelIndex & parent) const {
    return m_layers.count();
}

Qt::ItemFlags NavitLayersModel::flags(const QModelIndex &index) const {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool NavitLayersModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    return false;
}

QModelIndex NavitLayersModel::index(int row, int column, const QModelIndex &parent) const {
    return createIndex(row, column);
}

QModelIndex NavitLayersModel::parent(const QModelIndex &child) const {
    return QModelIndex();
}

int NavitLayersModel::columnCount(const QModelIndex &parent) const {
    return 0;
}

void NavitLayersModel::setNavitLayers(NavitLayoutsModel * navitLayouts){
    m_navitLayoutsInstance = navitLayouts;
    update();
    connect(m_navitLayoutsInstance, &NavitLayoutsModel::layoutChanged, this, &NavitLayersModel::update);
}
void NavitLayersModel::update() {
    if(m_navitLayoutsInstance && m_navitLayoutsInstance->m_navitInstance){
        struct attr attr;

        beginResetModel();
        m_layers.clear();
        endResetModel();

        navit_get_attr(m_navitLayoutsInstance->m_navitInstance->getNavit(), attr_layout, &attr, nullptr);
        GList * layers = attr.u.layout->layers;

        while(layers){
            QVariantMap layerMap;
            GList *next = layers->next;
            struct layer* layer = (struct layer*)layers->data;
            layerMap.insert("name", layer->name);
            layerMap.insert("action", "toggleLayer");
            if(layer->active){
                layerMap.insert("imageUrl", "qrc:/themes/Levy/assets/ionicons/md-checkmark-circle-outline.svg");
            } else {
                layerMap.insert("imageUrl", "");
            }
            beginInsertRows(QModelIndex(), rowCount(), rowCount());
            m_layers.append(layerMap);
            endInsertRows();
            layers = next;
        }
    }
}


void NavitLayersModel::toggleLayer(QString name){
    if(m_navitLayoutsInstance && m_navitLayoutsInstance->m_navitInstance){
        struct attr attr;
        char* layerName = name.toUtf8().data();

        navit_get_attr(m_navitLayoutsInstance->m_navitInstance->getNavit(), attr_layout, &attr, nullptr);
        GList * layers = attr.u.layout->layers;
        while (layers) {
            struct layer*l = (struct layer*)layers->data;
            if(l && !strcmp(l->name,layerName) ) {
                l->active ^= 1;
                navit_draw(m_navitLayoutsInstance->m_navitInstance->getNavit());
            }
            layers=g_list_next(layers);
        }

        update();
    }
}

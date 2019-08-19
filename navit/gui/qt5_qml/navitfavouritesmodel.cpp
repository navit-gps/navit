#include "navitfavouritesmodel.h"

NavitFavouritesModel::NavitFavouritesModel(QObject *parent)
{

}
QHash<int, QByteArray> NavitFavouritesModel::roleNames() const{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[CoordXRole] = "coordX";
    roles[CoordYRole] = "coordY";
    roles[CoordProjectionRole] = "coordProjection";
    return roles;
}

QVariant NavitFavouritesModel::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() >= m_favourites.count())
        return QVariant();

    const QVariantMap *poi = &m_favourites.at(index.row());

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


int NavitFavouritesModel::rowCount(const QModelIndex & parent) const {
    return m_favourites.count();
}

Qt::ItemFlags NavitFavouritesModel::flags(const QModelIndex &index) const {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool NavitFavouritesModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    return false;
}

QModelIndex NavitFavouritesModel::index(int row, int column, const QModelIndex &parent) const {
    return createIndex(row, column);
}

QModelIndex NavitFavouritesModel::parent(const QModelIndex &child) const {
    return QModelIndex();
}

int NavitFavouritesModel::columnCount(const QModelIndex &parent) const {
    return 0;
}

void NavitFavouritesModel::setNavit(NavitInstance * navit){
    m_navitInstance = navit;
    update();
}


void NavitFavouritesModel::update() {
    if(m_navitInstance){
        qDebug() << "Getting bookmarks";
        struct attr attr,mattr;
        struct navigation * nav = nullptr;
        struct item *item;
        struct coord c;
        struct pcoord pc;

        m_favourites.clear();

        pc.pro = transform_get_projection(navit_get_trans(m_navitInstance->getNavit()));

        if(navit_get_attr(m_navitInstance->getNavit(), attr_bookmarks, &mattr, nullptr) ) {
            bookmarks_item_rewind(mattr.u.bookmarks);
            while ((item=bookmarks_get_item(mattr.u.bookmarks))) {
                if (!item_attr_get(item, attr_label, &attr)) continue;
                qDebug() << c.x << " : " << c.y;

                if (item_coord_get(item, &c, 1)) {
                    pc.x = c.x;
                    pc.y = c.y;
                    qDebug() << attr.u.str << c.x << " : " << c.y;
                    QVariantMap recentItem;
                    recentItem.insert("coordX",pc.x);
                    recentItem.insert("coordY",pc.y);
                    recentItem.insert("coordProjection",pc.pro);
                    recentItem.insert("name",attr.u.str);

                    beginInsertRows(QModelIndex(), rowCount(), rowCount());
                    m_favourites.append(recentItem);
                    endInsertRows();
                }
            }
            qDebug() << m_favourites;
        }
    }
}
void NavitFavouritesModel::showFavourites(){
    struct attr mattr;

    if(navit_get_attr(m_navitInstance->getNavit(), attr_bookmarks, &mattr, nullptr) ) {
        struct attr attr;
        struct item *item;
        struct coord c;
        struct pcoord *pc;
        enum projection pro=bookmarks_get_projection(mattr.u.bookmarks);
        int i, bm_count;

        navit_set_destination(m_navitInstance->getNavit(), nullptr, nullptr, 0);

        bm_count=bookmarks_get_bookmark_count(mattr.u.bookmarks);
        pc=(pcoord *)g_alloca(bm_count*sizeof(struct pcoord));
        bookmarks_item_rewind(mattr.u.bookmarks);
        i=0;
        while ((item=bookmarks_get_item(mattr.u.bookmarks))) {
            if (!item_attr_get(item, attr_label, &attr))
                continue;
            if (item->type == type_bookmark) {
                if (item_coord_get(item, &c, 1)) {
                    pc[i].x=c.x;
                    pc[i].y=c.y;
                    pc[i].pro=pro;
                    navit_add_destination_description(m_navitInstance->getNavit(),&pc[i],attr.u.str);
                    i++;
                }
            }
        }
        bm_count=i;

        if (bm_count>0) {
            navit_set_destinations(m_navitInstance->getNavit(), pc, bm_count, "test_", 1);
//            if (this->flags & 512) {
                struct attr follow;
                follow.type=attr_follow;
                follow.u.num=180;
//                navit_set_attr(this->nav, &this->osd_configuration);
                navit_set_attr(m_navitInstance->getNavit(), &follow);
                navit_zoom_to_route(m_navitInstance->getNavit(), 0);
//            }
        }
    }
}

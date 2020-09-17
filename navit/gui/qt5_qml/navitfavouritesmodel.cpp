#include "navitfavouritesmodel.h"

NavitFavouritesModel::NavitFavouritesModel(QObject *parent)
{

}
QHash<int, QByteArray> NavitFavouritesModel::roleNames() const{
    QHash<int, QByteArray> roles;
    roles[LabelRole] = "label";
    roles[CoordinatesRole] = "coords";
    return roles;
}

QVariant NavitFavouritesModel::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() >= m_favourites.count())
        return QVariant();

    const QVariantMap *poi = &m_favourites.at(index.row());

    if (role == LabelRole)
        return poi->value("label");
    if (role == CoordinatesRole)
        return poi->value("coords");

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
        enum projection projection;

        m_favourites.clear();

        projection = transform_get_projection(navit_get_trans(m_navitInstance->getNavit()));

        if(navit_get_attr(m_navitInstance->getNavit(), attr_bookmarks, &mattr, nullptr) ) {
            bookmarks_item_rewind(mattr.u.bookmarks);
            while ((item=bookmarks_get_item(mattr.u.bookmarks))) {
                if (!item_attr_get(item, attr_label, &attr)) continue;
                //                qDebug() << c.x << " : " << c.y;

                if (item_coord_get(item, &c, 1)) {
                    QVariantMap coords;
                    coords.insert("x", c.x);
                    coords.insert("y", c.y);
                    coords.insert("pro", projection);

                    QVariantMap recentItem;
                    recentItem.insert("coords",coords);
                    recentItem.insert("label",attr.u.str);

                    beginInsertRows(QModelIndex(), rowCount(), rowCount());
                    m_favourites.append(recentItem);
                    endInsertRows();
                }
            }
            //            qDebug() << m_favourites;
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
    }
}
void NavitFavouritesModel::setAsDestination(int index){
    if(m_favourites.size() > index){
        NavitHelper::setDestination(m_navitInstance,
                                    m_favourites[index]["label"].toString(),
                                    m_favourites[index]["coords"].toMap()["x"].toInt(),
                                    m_favourites[index]["coords"].toMap()["y"].toInt());
    }
}

void NavitFavouritesModel::setAsPosition(int index){
    if(m_favourites.size() > index){
        NavitHelper::setPosition(m_navitInstance,
                                 m_favourites[index]["coords"].toMap()["x"].toInt(),
                                 m_favourites[index]["coords"].toMap()["y"].toInt());
    }
}

void NavitFavouritesModel::addAsBookmark(int index){
    if(m_favourites.size() > index){
        NavitHelper::addBookmark(m_navitInstance,
                                 m_favourites[index]["label"].toString(),
                                 m_favourites[index]["coords"].toMap()["x"].toInt(),
                                 m_favourites[index]["coords"].toMap()["y"].toInt());
    }
}

void NavitFavouritesModel::addStop(int index,  int position){
    if(m_favourites.size() > index){
        NavitHelper::addStop(m_navitInstance,
                             position,
                                    m_favourites[index]["label"].toString(),
                                    m_favourites[index]["coords"].toMap()["x"].toInt(),
                                    m_favourites[index]["coords"].toMap()["y"].toInt());
    }
}

#ifndef NAVITSEARCHMODEL_H
#define NAVITSEARCHMODEL_H

#include <QAbstractItemModel>
#include <QDebug>
#include "navitinstance.h"
#include "navithelper.h"

#include <glib.h>
extern "C" {
#include "config.h"
#include "item.h" /* needs to be first, as attr.h depends on it */
#include "navit.h"

#include "coord.h"
#include "attr.h"
#include "xmlconfig.h" // for NAVIT_OBJECT
#include "layout.h"
#include "map.h"
#include "transform.h"

#include "mapset.h"
#include "callback.h"
#include "search.h"
#include "country.h"
#include "track.h"

}

class NavitSearchModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(NavitInstance * navit MEMBER m_navitInstance WRITE setNavit)
public:
    enum RecentsModelRoles {
        LabelRole = Qt::UserRole + 1,
        NameRole,
        IconRole,
        AddressRole,
        DistanceRole
    };

    enum SearchField {
        SearchCountry,
        SearchTown,
        SearchStreet,
        SearchHouse
    };

    Q_ENUMS(SearchField)

    explicit NavitSearchModel(QObject *parent = 0);
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex & index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    void setNavit(NavitInstance * navit);

    Q_INVOKABLE void setAsDestination(int index);
    Q_INVOKABLE void setAsPosition(int index);
    Q_INVOKABLE void addAsBookmark(int index);
    Q_INVOKABLE void addStop(int index,  int position);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void select(int index);
    Q_INVOKABLE void search(QString queryText);
    Q_INVOKABLE void search2(QString queryText);
    void search_list_set_default_country();
    static void idle_cb(NavitSearchModel * searchModel);
    void handleSearchResult();

private:
    NavitInstance *m_navitInstance = nullptr;
    QList<QVariantMap> m_searchResults;
    QString m_country = "United Kingdom";

    struct callback * m_idle_cb;
    struct event_idle * m_event_idle;

    struct search_list *m_searchResultList;

    char *m_country_iso2;
    enum attr_type m_search_type;
    void update();

    void idle_start();
    void idle_end();
    void setDefaultCountry();
};

#endif // NAVITSEARCHMODEL_H

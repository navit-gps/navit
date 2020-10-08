#ifndef NAVITSEARCHMODEL_H
#define NAVITSEARCHMODEL_H

#include <QAbstractItemModel>
#include <QDebug>
#include <QThread>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>

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

#include "graphics.h"
}

Q_DECLARE_METATYPE(struct item *)

struct Address{
    QString country;
    QString town;
    QString street;
    QString house;
};
struct SearchResult{
    int index;
    QString distance;
    QString addressString;
    QString icon;
    QString label;
    struct pcoord coords;
    struct Address address;
};

Q_DECLARE_METATYPE(SearchResult)

class SearchWorker : public QThread
{
    Q_OBJECT
public:
    SearchWorker (NavitInstance * navit, struct search_list *searchResultList, QString searchQuery, enum attr_type search_type);
    void run() override;
signals:
    void gotSearchResult(SearchResult result);
private:
    NavitInstance * m_navitInstance;
    struct search_list *m_searchResultList;
    QString m_searchQuery;
    enum attr_type m_search_type;
};

class NavitSearchModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(NavitInstance * navit MEMBER m_navitInstance WRITE setNavit)
    Q_PROPERTY(SearchType currentSearchType READ getSearchType WRITE changeSearchType NOTIFY searchTypeChanged)
    Q_PROPERTY(QString searchQuery MEMBER m_searchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(QString country READ getCountry NOTIFY addressChanged)
    Q_PROPERTY(QString town READ getTown NOTIFY addressChanged)
    Q_PROPERTY(QString street READ getStreet NOTIFY addressChanged)
    Q_PROPERTY(QString house READ getHouse NOTIFY addressChanged)

public:
    enum SearchModelRoles {
        LabelRole = Qt::UserRole + 1,
        NameRole,
        IconRole,
        AddressRole,
        DistanceRole
    };

    enum SearchType {
        SearchCountry = attr_country_all,
        SearchTown = attr_town_or_district_name,//attr_town_or_district_name or attr_town_postal
        SearchStreet = attr_street_name,
        SearchHouse = attr_house_number
    };

    Q_ENUMS(SearchType)
    explicit NavitSearchModel(QObject *parent = 0);
    ~NavitSearchModel ();
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex & index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    void setNavit(NavitInstance * navit);
    void search_list_set_default_country();
public slots:
    void setAsDestination(int index);
    void setAsPosition(int index);
    void addAsBookmark(int index);
    void addStop(int index,  int position);
    void setAddressAsDestination();
    void setAddressAsPosition();
    void addAddressAsBookmark();
    void addAddressStop(int position);
    void viewAddressOnMap();
    void viewResultOnMap(int index);
    void remove(int index);
    void select(int index);
    void search();
    void reset();
signals:
    void searchTypeChanged();
    void searchQueryChanged();
    void addressChanged();
private slots:
    void receiveSearchResult(SearchResult result);
private:
    NavitInstance *m_navitInstance = nullptr;

    QList<SearchResult> m_searchResults;

    QString m_country = "United Kingdom";

    struct search_list *m_searchResultList;

    char *m_country_iso2;

    enum SearchType m_search_type;

    QString m_searchQuery;
    QString m_searchQueryCountry;
    QString m_searchQueryTown;
    QString m_searchQueryStreet;

    SearchResult m_address;

    SearchWorker *m_searchWorker = nullptr;

    void update();
    void stopSearchWorker(bool clearModel = false);
    void setDefaultCountry();
    void changeSearchType(enum SearchType searchType, bool restoreQuery = true);
    void searchResultsToMap(QList<SearchResult> results);
    enum SearchType getSearchType();

    void viewOnMap(SearchResult pointMap);

    QString getCountry(){
        return m_address.address.country;
    }
    QString getTown(){
        return m_address.address.town;
    }
    QString getStreet(){
        return m_address.address.street;
    }
    QString getHouse(){
        return m_address.address.house;
    }
};

#endif // NAVITSEARCHMODEL_H

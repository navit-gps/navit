#ifndef SEARCHOBJECT_H
#define SEARCHOBJECT_H

#include <QObject>

class SearchObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString icon READ icon WRITE setIcon NOTIFY iconChanged)

public:
    SearchObject(QObject *parent=0);
    SearchObject(const QString &name, const QString &icon, struct pcoord *c, QObject *parent=0);

    QString name() const;
    void setName(const QString &name);

    QString icon() const;
    void setIcon(const QString &icon);
    struct pcoord * getCoords() const;

signals:
    void nameChanged();
    void iconChanged();

private:
    QString m_name;
    QString m_icon;
    struct pcoord *m_c;
};

#endif // SEARCHOBJECT_H

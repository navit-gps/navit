#ifndef NAVITINSTANCE_H
#define NAVITINSTANCE_H

#include <QObject>

class NavitInstance : public QObject
{
    Q_OBJECT
public:
    explicit NavitInstance(struct navit* nav, struct graphics_priv* gp, QObject *parent = nullptr): m_graphics_priv(gp), m_navit(nav) {}
    navit *getNavit() {
        return m_navit;
    }
    void emit_update() {
        emit update();
    }
    struct graphics_priv* m_graphics_priv;
signals:
    void update();
private:
    navit * m_navit;
};

#endif // NAVITINSTANCE_H

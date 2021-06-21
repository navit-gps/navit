#ifndef QT5GRAPHICSWORKER_H
#define QT5GRAPHICSWORKER_H

#include <QObject>
#include <QThread>
#include <glib.h>
#include <QMutex>
#include <QHash>

extern "C" {
#include "config.h"

#include "callback.h"
#include "color.h"
#include "debug.h"
#include "event.h"

#include "point.h" /* needs to be before graphics.h */

#include "graphics.h"
#include "item.h"
#include "keys.h"
#include "navit.h"

#include "event.h"
}

#if defined(WINDOWS) || defined(WIN32) || defined(HAVE_API_WIN32_CE)
#include <windows.h>
#endif

#include "graphics_qt5.h"
#include <QSocketNotifier>

class Qt5GraphicsWorker : public QObject {
    Q_OBJECT
public:
    Qt5GraphicsWorker(QObject *parent = nullptr);
    struct eventWatch {
        QSocketNotifier* sn;
        struct callback* cb;
        int fd;
    };

    struct eventTimeout {
        int id;
        struct callback *cb;
        int type;
    };

    struct eventIdle {
        int id;
        struct callback *cb;
    };

    QHash<int, int> m_timer_type;
    QHash<int, struct callback *> m_timer_callback;
    QHash<int, struct eventWatch*> m_watches;
    QHash<int, struct eventTimeout*> m_timeouts;
    QHash<int, struct eventIdle*> m_idles;

    void resizeEvent(NavitInstance *navitInstance, int width, int height);
    void mapMove(NavitInstance *navitInstance, struct point *origin, struct point *destination);
    void zoomIn(NavitInstance *navitInstance, int zoomLevel, struct point *p);
    void zoomOut(NavitInstance *navitInstance, int zoomLevel, struct point *p);
    void zoomToRoute(NavitInstance *navitInstance);
    void setNumAttr(NavitInstance *navitInstance, struct attr *attr);
    void centerOnPosition(NavitInstance *navitInstance);

signals:
    void addIdle(struct eventIdle* ei);
    void addTimeout(struct eventTimeout* et, int timeout);
    void removeIdle(struct eventIdle* ei);
    void removeTimeout(struct eventTimeout* et);

public slots:
    void watchEvent(int id);

    void addIdleHandler(struct eventIdle* ei);
    void addTimeoutHandler(struct eventTimeout* et, int timeout);
    void removeIdleHandler(struct eventIdle* ei);
    void removeTimeoutHandler(struct eventTimeout* et);

protected:
    void timerEvent(QTimerEvent* event);
private:
};

extern Qt5GraphicsWorker* qt5_timer;

#endif // QT5GRAPHICSWORKER_H

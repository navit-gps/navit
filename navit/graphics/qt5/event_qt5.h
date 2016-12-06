#include <glib.h>
#include <QObject>

class qt5_navit_timer : public QObject
{
        Q_OBJECT
public:
        qt5_navit_timer(QObject * parent = 0);
        GHashTable *timer_type;
        GHashTable *timer_callback;
        GHashTable *watches;
protected:
        void timerEvent(QTimerEvent * event);
};


void
qt5_event_init(void);

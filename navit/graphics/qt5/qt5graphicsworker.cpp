#include "qt5graphicsworker.h"

Qt5GraphicsWorker::Qt5GraphicsWorker(QObject *parent) : QObject(parent) {
    dbg(lvl_debug, "qt5_navit_timer object created")

    connect(this, &Qt5GraphicsWorker::addIdle, this, &Qt5GraphicsWorker::addIdleHandler);
    connect(this, &Qt5GraphicsWorker::addTimeout, this, &Qt5GraphicsWorker::addTimeoutHandler);
    connect(this, &Qt5GraphicsWorker::removeIdle, this, &Qt5GraphicsWorker::removeIdleHandler);
    connect(this, &Qt5GraphicsWorker::removeTimeout, this, &Qt5GraphicsWorker::removeTimeoutHandler);
}

void Qt5GraphicsWorker::watchEvent(int id) {
    struct eventWatch* ret = m_watches[id];
    if (ret) {
        qDebug() << "Watch event : " << id;
        callback_call_0(ret->cb);
    }
}

void Qt5GraphicsWorker::addIdleHandler(struct eventIdle* ei){
    ei->id = startTimer(0);
    m_idles.insert(ei->id, ei);
    qDebug() << "Added idle : " << ei->id << " thread : " << QThread::currentThread();
}
void Qt5GraphicsWorker::addTimeoutHandler(struct eventTimeout* et, int timeout){
    et->id = startTimer(timeout);
    m_timeouts.insert(et->id, et);
    qDebug() << "Added timeout : " << et->id << " thread : " << QThread::currentThread();
}
void Qt5GraphicsWorker::removeIdleHandler(struct eventIdle* ei){
    killTimer(ei->id);
    m_idles.remove(ei->id);
    free(ei);
}
void Qt5GraphicsWorker::removeTimeoutHandler(struct eventTimeout* et){
    killTimer(et->id);
    m_timeouts.remove(et->id);
    free(et);
}

void Qt5GraphicsWorker::timerEvent(QTimerEvent* event) {
    int id = event->timerId();

    struct callback* cb = nullptr;
    struct eventTimeout* et = nullptr;

    if(m_idles.contains(id)){
        cb = m_idles[id]->cb;
//        qDebug() << "Idle event : " << id << " thread : " << QThread::currentThread();
    } else if(m_timeouts.contains(id)) {
        et = m_timeouts[id];
//        qDebug() << "Timeout event : " << id << " thread : " << QThread::currentThread();
        cb = et->cb;
    }

    if (cb)
        callback_call_0(cb);

    if(et && et->type == 0){
        removeTimeoutHandler(et);
//        qDebug() << "Oneshot timeout removed : " << id;
    }
}


void Qt5GraphicsWorker::resizeEvent(NavitInstance *navitInstance, int width, int height) {
    if(navitInstance){
        resize_callback(navitInstance->m_graphics_priv, width, height);
    }
}
void Qt5GraphicsWorker::mapMove(NavitInstance *navitInstance, struct point *origin, struct point *destination){
    if(navitInstance){
        qDebug() << "Map move thread : " << QThread::currentThread();
        navit_drag_map(navitInstance->getNavit(), origin, destination);
    }
}
void Qt5GraphicsWorker::zoomIn(NavitInstance *navitInstance, int zoomLevel, struct point *p){
    if(navitInstance){
        navit_zoom_in(navitInstance->getNavit(), zoomLevel, p);
    }
}
void Qt5GraphicsWorker::zoomOut(NavitInstance *navitInstance, int zoomLevel, struct point *p){
    if(navitInstance){
        navit_zoom_out(navitInstance->getNavit(), zoomLevel, p);
    }
}
void Qt5GraphicsWorker::zoomToRoute(NavitInstance *navitInstance) {
    if(navitInstance){
        navit_zoom_to_route(navitInstance->getNavit(), 1);
    }

}
void Qt5GraphicsWorker::setNumAttr(NavitInstance *navitInstance, struct attr *attr) {
    if(navitInstance && attr){
        navit_set_attr(navitInstance->getNavit(), attr);
    }
}

void Qt5GraphicsWorker::centerOnPosition(NavitInstance *navitInstance) {
    if(navitInstance){
        navit_set_center_cursor_draw(navitInstance->getNavit());
    }
}

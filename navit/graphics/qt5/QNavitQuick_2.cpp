/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2019 Navit Team
 * Copyright (C) 2019 Viktor Verebelyi <me@viktorgino.me>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <glib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
extern "C" {
#include "config.h"
#include "item.h" /* needs to be first, as attr.h depends on it */
#include "navit.h"

#include "callback.h"
#include "color.h"
#include "debug.h"
#include "event.h"
#include "transform.h"

#include "point.h" /* needs to be before graphics.h */

#include "graphics.h"
#include "keys.h"
#include "plugin.h"
#include "window.h"

#include "bookmarks.h"
}
#if defined(WINDOWS) || defined(WIN32) || defined(HAVE_API_WIN32_CE)
#include <windows.h>
#endif
#include "QNavitQuick_2.h"
#include "graphics_qt5.h"
#include <QPainter>

#include <QOpenGLFramebufferObject>
QNavitQuick_2::QNavitQuick_2(QQuickItem* parent)
    : QQuickPaintedItem(parent),
      graphics_priv(nullptr),
      m_moveX(0),
      m_moveY(0) {
    setAcceptedMouseButtons(Qt::AllButtons);

    connect(this, &QNavitQuick_2::onResizeEvent, qt5_timer, &Qt5GraphicsWorker::resizeEvent);
    connect(this, &QNavitQuick_2::onMapMove, qt5_timer, &Qt5GraphicsWorker::mapMove);
    connect(this, &QNavitQuick_2::onZoomIn, qt5_timer, &Qt5GraphicsWorker::zoomIn);
    connect(this, &QNavitQuick_2::onZoomOut, qt5_timer, &Qt5GraphicsWorker::zoomOut);
    connect(this, &QNavitQuick_2::onZoomToRoute, qt5_timer, &Qt5GraphicsWorker::zoomToRoute);
    connect(this, &QNavitQuick_2::onSetNumAttr, qt5_timer, &Qt5GraphicsWorker::setNumAttr);
    connect(this, &QNavitQuick_2::onCenterOnPosition, qt5_timer, &Qt5GraphicsWorker::centerOnPosition);
}

void QNavitQuick_2::paintOverlays(QPainter* painter, struct graphics_priv* gp, QPaintEvent* event) {
    GHashTableIter iter;
    struct graphics_priv *key, *value;
    g_hash_table_iter_init(&iter, gp->overlays);
    while (g_hash_table_iter_next(&iter, (void**)&key, (void**)&value)) {
        if (!value->disable) {
            QRect rr(value->x, value->y, value->pixmap->width(), value->pixmap->height());
            if (event->rect().intersects(rr)) {
                dbg(lvl_debug, "draw overlay (%d, %d, %d, %d)", value->x + value->scroll_x, value->y + value->scroll_y,
                    value->pixmap->width(), value->pixmap->height())

                painter->drawPixmap(value->x, value->y, *value->pixmap);
                /* draw overlays of overlay if any by recursive calling */
                paintOverlays(painter, value, event);
            }
        }
    }
}

void QNavitQuick_2::paint(QPainter* painter) {
    QPaintEvent event = QPaintEvent(QRect(boundingRect().x(), boundingRect().y(), boundingRect().width(),
                                          boundingRect().height()));

    dbg(lvl_debug, "enter (%f, %f, %f, %f)", boundingRect().x(), boundingRect().y(), boundingRect().width(),
        boundingRect().height())

    /* color background if any */
    if (graphics_priv->background_graphics_gc_priv != nullptr) {
        painter->setPen(*graphics_priv->background_graphics_gc_priv->pen);
        painter->fillRect(boundingRect(), *graphics_priv->background_graphics_gc_priv->brush);
    }

    painter->drawPixmap(m_moveX, m_moveY, *graphics_priv->pixmap,
                        boundingRect().x(), boundingRect().y(),
                        boundingRect().width(), boundingRect().height());

    /* disable on root pane disables ALL overlays (for drag of background) */
    if(!(graphics_priv->disable)) {
        paintOverlays(painter, graphics_priv, &event);
    }
    qDebug() << "Painting thread : " << QThread::currentThread();
    updateZoomLevel();
}

void QNavitQuick_2::geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry) {
    dbg(lvl_debug, "enter")
    QQuickPaintedItem::geometryChanged(newGeometry, oldGeometry);
    QPainter* painter = nullptr;
    if (graphics_priv == nullptr) {
        dbg(lvl_debug, "Context not set, aborting")
        return;
    }
    if (graphics_priv->pixmap != nullptr) {
        if((width() != graphics_priv->pixmap->width()) || (height() != graphics_priv->pixmap->height())) {
            delete graphics_priv->pixmap;
            graphics_priv->pixmap = nullptr;
        }
    }
    if (graphics_priv->pixmap == nullptr) {
        graphics_priv->pixmap = new QPixmap(width(), height());
    }
    painter = new QPainter(graphics_priv->pixmap);
    if (painter != nullptr) {
        QBrush brush;
        painter->fillRect(0, 0, width(), height(), brush);
        delete painter;
    }
    dbg(lvl_debug, "size %fx%f", width(), height())
    dbg(lvl_debug, "pixmap %p %dx%d", graphics_priv->pixmap, graphics_priv->pixmap->width(),
        graphics_priv->pixmap->height())
    /* if the root window got resized, tell navit about it */
    if (graphics_priv->root){
        emit onResizeEvent(m_navitInstance, width(), height());
    }
}

void QNavitQuick_2::mousePressEvent(QMouseEvent* event) {
    QPoint loc;
    loc.setX(event->x());
    loc.setY(event->y());
    if(event->button() == Qt::LeftButton){
        m_originX = event->x();
        m_originY = event->y();
        emit leftButtonClicked(loc);
    } else if (event->button() == Qt::RightButton) {
        emit rightButtonClicked(loc);
    }
}

void QNavitQuick_2::mouseReleaseEvent(QMouseEvent* event) {
    if(event->button() == Qt::LeftButton){
        mapMove(m_originX, m_originY, event->x(), event->y());
        m_moveX = 0;
        m_moveY = 0;
    }
}

void QNavitQuick_2::mouseMoveEvent(QMouseEvent* event) {
    if(event->buttons() == Qt::LeftButton){
        setFollowVehicle(false);
        if(event->modifiers() & Qt::ShiftModifier){
            int pitch = qFloor((m_originY - event->y()) / 10);
            int orientation =  m_orientation + ( qFloor((event->x() - m_originX)) / 10 );

            if(m_pitch + pitch < 0 ) {
                setPitch(0);
            } else if(m_pitch + pitch > 60 ) {
                setPitch(60);
            } else {
                setPitch(m_pitch + pitch);
            }

            setOrientation(orientation % 360);

        } else {
            m_moveX = event->x() - m_originX;
            m_moveY = event->y() - m_originY;
            update();
        }
        emit positionChanged(event);
    }
}

void QNavitQuick_2::wheelEvent(QWheelEvent* event) {
    if (event->angleDelta().y() > 0){
        zoomInToPoint(2, event->position().x(), event->position().y());
    } else {
        zoomOutFromPoint(2, event->position().x(), event->position().y());
    }
}

void QNavitQuick_2::mapMove(int originX, int originY, int destinationX, int destinationY) {
    struct point *origin = new struct point;
    origin->x = originX;
    origin->y = originY;
    struct point *destination = new struct point;
    destination->x = destinationX;
    destination->y = destinationY;

    emit onMapMove(m_navitInstance, origin, destination);
}

void QNavitQuick_2::zoomIn(int zoomLevel, point *p){
    emit onZoomIn(m_navitInstance, zoomLevel, p);
}
void QNavitQuick_2::zoomOut(int zoomLevel, point *p){
    emit onZoomOut(m_navitInstance, zoomLevel, p);
}

void QNavitQuick_2::zoomInToPoint(int zoomLevel, int x, int y){
    if(m_navitInstance){
        struct point *p = new struct point;
        p->x = x;
        p->y = y;
        zoomIn(zoomLevel, p);
    }
}

void QNavitQuick_2::zoomOutFromPoint(int zoomLevel, int x, int y){
    if(m_navitInstance){
        struct point *p = new struct point;
        p->x = x;
        p->y = y;
        zoomOut(zoomLevel, p);
    }
}
void QNavitQuick_2::zoomToRoute(){
    emit onZoomToRoute(m_navitInstance);
}

void QNavitQuick_2::setNavitNumProperty(enum attr_type type, int value){
    if(m_navitInstance){
        struct attr attr;

        attr.type = type;
        attr.u.num = value;

        emit onSetNumAttr(m_navitInstance, &attr);
    }
}

int QNavitQuick_2::getNavitNumProperty(enum attr_type type){
    struct attr attr;
    if(m_navitInstance){
        navit_get_attr(m_navitInstance->getNavit(), type, &attr, nullptr);
    }
    return attr.u.num;
}

void QNavitQuick_2::setPitch(int pitch){
    setNavitNumProperty(attr_pitch, pitch);
}

void QNavitQuick_2::setFollowVehicle(bool followVehicle){
    qDebug() << "setFollowVehicle " << followVehicle;
    setNavitNumProperty(attr_follow, 0);
    setNavitNumProperty(attr_follow_cursor, followVehicle);
}

void QNavitQuick_2::setTracking(bool tracking) {
    setNavitNumProperty(attr_tracking, tracking);
}

void QNavitQuick_2::setAutozoom(bool autoZoom){
    setNavitNumProperty(attr_autozoom, (int)autoZoom);
    setNavitNumProperty(attr_autozoom_active, (int)autoZoom);
}

void QNavitQuick_2::setOrientation(int orientation){
    setNavitNumProperty(attr_orientation, orientation);
}

void QNavitQuick_2::addBookmark(QString label, int x, int y){
    NavitHelper::addBookmark(m_navitInstance, label, x, y);
}
NavitInstance* QNavitQuick_2::navitInstance(){
    return m_navitInstance;
}

void QNavitQuick_2::setNavitInstance(NavitInstance *navit){
    m_navitInstance=navit;
    if(m_navitInstance) {
        graphics_priv = navit->m_graphics_priv;

        QObject::connect(navit, SIGNAL(update()), this, SLOT(update()));

        navit_add_callback(m_navitInstance->getNavit(),callback_new_attr_1(callback_cast(QNavitQuick_2::attributeCallbackHandler),
                                                                           attr_orientation,this));
        navit_add_callback(m_navitInstance->getNavit(),callback_new_attr_1(callback_cast(QNavitQuick_2::attributeCallbackHandler),
                                                                           attr_follow_cursor,this));
        navit_add_callback(m_navitInstance->getNavit(),callback_new_attr_1(callback_cast(QNavitQuick_2::attributeCallbackHandler),
                                                                           attr_tracking,this));
        navit_add_callback(m_navitInstance->getNavit(),callback_new_attr_1(callback_cast(QNavitQuick_2::attributeCallbackHandler),
                                                                           attr_autozoom_active,this));
        navit_add_callback(m_navitInstance->getNavit(),callback_new_attr_1(callback_cast(QNavitQuick_2::attributeCallbackHandler),
                                                                           attr_pitch,this));
        m_orientation = getNavitNumProperty(attr_orientation);
        m_followVehicle = getNavitNumProperty(attr_follow_cursor);
        m_tracking = getNavitNumProperty(attr_tracking);
        m_autoZoom = getNavitNumProperty(attr_autozoom_active);
        m_pitch = getNavitNumProperty(attr_pitch);
        emit propertiesChanged();
    }
}

QString QNavitQuick_2::getAddress(int x, int y){
    if(m_navitInstance){
        coord c = NavitHelper::positionToCoord(m_navitInstance->getNavit(), x, y);
        return NavitHelper::getAddress(m_navitInstance, c);
    }
    return "";
}

void QNavitQuick_2::centerOnPosition(){
    emit onCenterOnPosition(m_navitInstance);
}

void QNavitQuick_2::updateZoomLevel(){
    if(m_navitInstance) {
        struct transformation * trans = navit_get_trans(m_navitInstance->getNavit());
        long scale = transform_get_scale(trans);

        int i = 0;
        while(scale >> i){
            i++;
        }

        m_zoomLevel = 19 - i;
        emit zoomLevelChanged();
    }
}

void QNavitQuick_2::attributeCallbackHandler(QNavitQuick_2 *navitGraph, navit *this_, attr *attr){
    navitGraph->attributeCallback(attr);
}
void QNavitQuick_2::attributeCallback(attr *attr){
    switch(attr->type){
    case attr_orientation :
        m_orientation = attr->u.num;
        break;
    case attr_follow_cursor :
        m_followVehicle = attr->u.num;
        break;
    case attr_tracking :
        m_tracking = attr->u.num;
        break;
    case attr_autozoom_active :
        m_autoZoom = attr->u.num;
        break;
    case attr_pitch :
        m_pitch = attr->u.num;
        break;
    default:
        return;
    }
    emit propertiesChanged();
}

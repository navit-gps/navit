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
    graphics_priv = nullptr;
}

static void paintOverlays(QPainter* painter, struct graphics_priv* gp, QPaintEvent* event) {
    GHashTableIter iter;
    struct graphics_priv *key, *value;
    g_hash_table_iter_init(&iter, gp->overlays);
    while (g_hash_table_iter_next(&iter, (void**)&key, (void**)&value)) {
        if (!value->disable) {
            QRect rr(value->x, value->y, value->pixmap->width(), value->pixmap->height());
            if (event->rect().intersects(rr)) {
                dbg(lvl_debug, "draw overlay (%d, %d, %d, %d)", value->x + value->scroll_x, value->y + value->scroll_y,
                    value->pixmap->width(), value->pixmap->height());

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
        boundingRect().height());

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
}

void QNavitQuick_2::geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry) {
    dbg(lvl_debug, "enter");
    QQuickPaintedItem::geometryChanged(newGeometry, oldGeometry);
    QPainter* painter = NULL;
    if (graphics_priv == NULL) {
        dbg(lvl_debug, "Context not set, aborting");
        return;
    }
    if (graphics_priv->pixmap != NULL) {
        if((width() != graphics_priv->pixmap->width()) || (height() != graphics_priv->pixmap->height())) {
            delete graphics_priv->pixmap;
            graphics_priv->pixmap = NULL;
        }
    }
    if (graphics_priv->pixmap == NULL) {
        graphics_priv->pixmap = new QPixmap(width(), height());
    }
    painter = new QPainter(graphics_priv->pixmap);
    if (painter != NULL) {
        QBrush brush;
        painter->fillRect(0, 0, width(), height(), brush);
        delete painter;
    }
    dbg(lvl_debug, "size %fx%f", width(), height());
    dbg(lvl_debug, "pixmap %p %dx%d", graphics_priv->pixmap, graphics_priv->pixmap->width(),
        graphics_priv->pixmap->height());
    /* if the root window got resized, tell navit about it */
    if (graphics_priv->root)
        resize_callback(graphics_priv, width(), height());
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
        if(m_navitInstance){
            struct point origin;
            origin.x = m_originX;
            origin.y = m_originY;
            struct point destination;
            destination.x = event->x();
            destination.y = event->y();

            navit_drag_map(m_navitInstance->getNavit(), &origin, &destination);
        }
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
    if (event->delta() > 0){
        zoomInToPoint(2, event->x(), event->y());
    } else {
        zoomOutFromPoint(2, event->x(), event->y());
    }
}

void QNavitQuick_2::mapMove(int originX, int originY, int destinationX, int destinationY) {
    if(m_navitInstance){
        struct point origin;
        origin.x = originX;
        origin.y = originY;
        struct point destination;
        destination.x = destinationX;
        destination.y = destinationY;

        navit_drag_map(m_navitInstance->getNavit(), &origin, &destination);
    }
}

void QNavitQuick_2::zoomIn(int zoomLevel){
    if(m_navitInstance){
        navit_zoom_in(m_navitInstance->getNavit(), zoomLevel, nullptr);
        updateZoomLevel();
    }
}
void QNavitQuick_2::zoomOut(int zoomLevel){    
    if(m_navitInstance){
        navit_zoom_out(m_navitInstance->getNavit(), zoomLevel, nullptr);
        updateZoomLevel();
    }
}

void QNavitQuick_2::zoomInToPoint(int zoomLevel, int x, int y){
    if(m_navitInstance){
        struct point p;
        p.x = x;
        p.y = y;
        navit_zoom_in(m_navitInstance->getNavit(), zoomLevel, &p);
        updateZoomLevel();
    }
}

void QNavitQuick_2::zoomOutFromPoint(int zoomLevel, int x, int y){
    if(m_navitInstance){
        struct point p;
        p.x = x;
        p.y = y;
        navit_zoom_out(m_navitInstance->getNavit(), zoomLevel, &p);
        updateZoomLevel();
    }
}

void QNavitQuick_2::zoomToRoute(){
    if(m_navitInstance){
        navit_zoom_to_route(m_navitInstance->getNavit(), 1);
        updateZoomLevel();
    }
}

void QNavitQuick_2::setNavitNumProperty(enum attr_type type, int value){
    if(m_navitInstance){
        struct attr attr;

        attr.type = type;
        attr.u.num = value;
        navit_set_attr(m_navitInstance->getNavit(), &attr);
    }
}

int QNavitQuick_2::getNavitNumProperty(enum attr_type type){
    struct attr attr;
    if(m_navitInstance){
        navit_get_attr(m_navitInstance->getNavit(), type, &attr, nullptr);
    }
    return attr.u.num;
}


void QNavitQuick_2::setNorthing(bool northing){
    if(northing != m_northing){
        setNavitNumProperty(attr_orientation, northing);
        m_northing = northing;
        emit propertiesChanged();
    }
}
void QNavitQuick_2::setPitch(int pitch){
    if(pitch != m_pitch){
        setNavitNumProperty(attr_pitch, pitch);
        m_pitch = pitch;
        emit propertiesChanged();
    }
}

void QNavitQuick_2::setFollowVehicle(bool followVehicle){
    if(followVehicle != m_followVehicle){
        setNavitNumProperty(attr_follow, 1000);
        setNavitNumProperty(attr_follow_cursor, followVehicle);
        m_followVehicle = followVehicle;
        emit propertiesChanged();
    }
}

void QNavitQuick_2::setAutozoom(bool autoZoom){
    if(autoZoom != m_autoZoom){
        setNavitNumProperty(attr_autozoom, 1);
        setNavitNumProperty(attr_autozoom_active, (int)autoZoom);
        m_autoZoom = autoZoom;
        emit propertiesChanged();
    }
}

void QNavitQuick_2::setOrientation(int orientation){
    if(orientation != m_orientation){
        setNorthing(false);
        setNavitNumProperty(attr_orientation, orientation);
        m_orientation = orientation;
        emit propertiesChanged();
    }
}

void QNavitQuick_2::addBookmark(QString label, int x, int y){
    if(m_navitInstance){
        struct attr attr;
        struct pcoord c = NavitHelper::positionToPcoord(m_navitInstance, x ,y);
        navit_get_attr(m_navitInstance->getNavit(), attr_bookmarks, &attr, nullptr);

        bookmarks_add_bookmark(attr.u.bookmarks, &c, label.toUtf8().data());
    }
}
NavitInstance* QNavitQuick_2::navitInstance(){
    return m_navitInstance;
}

void QNavitQuick_2::setNavitInstance(NavitInstance *navit){
    m_navitInstance=navit;
    graphics_priv = navit->m_graphics_priv;

    QObject::connect(navit, SIGNAL(update()), this, SLOT(update()));

    m_northing = getNavitNumProperty(attr_orientation);
    m_pitch = getNavitNumProperty(attr_pitch);
    m_autoZoom = getNavitNumProperty(attr_autozoom_active);
    m_followVehicle = getNavitNumProperty(attr_follow_cursor);
    m_orientation = getNavitNumProperty(attr_orientation);
    emit propertiesChanged();
}

QString QNavitQuick_2::getAddress(int x, int y){
    coord c = NavitHelper::positionToCoord(m_navitInstance, x, y);
    return NavitHelper::getAddress(m_navitInstance, c);
}

QVariantMap QNavitQuick_2::positionToCoordinates(int x, int y){
    QVariantMap ret;
    if(m_navitInstance){
        struct coord c = NavitHelper::positionToCoord(m_navitInstance, x ,y);
        ret.insert("x", c.x);
        ret.insert("y", c.y);
    }
    return ret;
}

void QNavitQuick_2::centerOnPosition(){
    if(m_navitInstance){
        navit_set_center_cursor_draw(m_navitInstance->getNavit());
    }
}

void QNavitQuick_2::updateZoomLevel(){
    struct transformation * trans = navit_get_trans(m_navitInstance->getNavit());
    long scale = transform_get_scale(trans);

    int i = 0;
    while(scale >> i){
        i++;
    }

    m_zoomLevel = 19 - i;
    emit zoomLevelChanged();
}

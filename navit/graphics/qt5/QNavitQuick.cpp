/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
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

#include "callback.h"
#include "color.h"
#include "debug.h"
#include "event.h"

#include "point.h" /* needs to be before graphics.h */

#include "graphics.h"
#include "keys.h"
#include "plugin.h"
#include "window.h"
}
#if defined(WINDOWS) || defined(WIN32) || defined(HAVE_API_WIN32_CE)
#include <windows.h>
#endif
#include "QNavitQuick.h"
#include "graphics_qt5.h"
#include <QPainter>

QNavitQuick::QNavitQuick(QQuickItem* parent)
    : QQuickPaintedItem(parent) {
    setAcceptedMouseButtons(Qt::AllButtons);
    graphics_priv = NULL;
}

void QNavitQuick::setGraphicContext(GraphicsPriv* gp) {
    dbg(lvl_debug, "enter");
    graphics_priv = gp->gp;
    QObject::connect(gp, SIGNAL(update()), this, SLOT(update()));
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

void QNavitQuick::paint(QPainter* painter) {
    QPaintEvent event = QPaintEvent(QRect(boundingRect().x(), boundingRect().y(), boundingRect().width(),
                                          boundingRect().height()));

    dbg(lvl_debug, "enter (%f, %f, %f, %f)", boundingRect().x(), boundingRect().y(), boundingRect().width(),
        boundingRect().height());

    /* color background if any */
    if (graphics_priv->background_graphics_gc_priv != NULL) {
        painter->setPen(*graphics_priv->background_graphics_gc_priv->pen);
        painter->fillRect(boundingRect(), *graphics_priv->background_graphics_gc_priv->brush);
    }
    /* draw base */
    painter->drawPixmap(graphics_priv->scroll_x, graphics_priv->scroll_y, *graphics_priv->pixmap,
                        boundingRect().x(), boundingRect().y(),
                        boundingRect().width(), boundingRect().height());
    /* disable on root pane disables ALL overlays (for drag of background) */
    if(!(graphics_priv->disable)) {
        paintOverlays(painter, graphics_priv, &event);
    }
}

void QNavitQuick::keyPressEvent(QKeyEvent* event) {
    dbg(lvl_debug, "enter");
    char key[2];
    int keycode;
    char* text = NULL;

    keycode = event->key();
    key[0] = '\0';
    key[1] = '\0';
    switch (keycode) {
    case Qt::Key_Up:
        key[0] = NAVIT_KEY_UP;
        break;
    case Qt::Key_Down:
        key[0] = NAVIT_KEY_DOWN;
        break;
    case Qt::Key_Left:
        key[0] = NAVIT_KEY_LEFT;
        break;
    case Qt::Key_Right:
        key[0] = NAVIT_KEY_RIGHT;
        break;
    case Qt::Key_Backspace:
        key[0] = NAVIT_KEY_BACKSPACE;
        break;
    case Qt::Key_Tab:
        key[0] = NAVIT_KEY_TAB;
        break;
    case Qt::Key_Delete:
        key[0] = NAVIT_KEY_DELETE;
        break;
    case Qt::Key_Escape:
        key[0] = NAVIT_KEY_BACK;
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        key[0] = NAVIT_KEY_RETURN;
        break;
    case Qt::Key_ZoomIn:
        key[0] = NAVIT_KEY_ZOOM_IN;
        break;
    case Qt::Key_ZoomOut:
        key[0] = NAVIT_KEY_ZOOM_OUT;
        break;
    case Qt::Key_PageUp:
        key[0] = NAVIT_KEY_PAGE_UP;
        break;
    case Qt::Key_PageDown:
        key[0] = NAVIT_KEY_PAGE_DOWN;
        break;
    default:
        QString str = event->text();
        if ((str != NULL) && (str.size() != 0)) {
            text = str.toUtf8().data();
        }
    }
    if (text != NULL)
        callback_list_call_attr_1(graphics_priv->callbacks, attr_keypress, (void*)text);
    else if (key[0])
        callback_list_call_attr_1(graphics_priv->callbacks, attr_keypress, (void*)key);
    else
        dbg(lvl_debug, "keyval 0x%x", keycode);
}

void QNavitQuick::keyReleaseEvent(QKeyEvent* event) {
    dbg(lvl_debug, "enter");
}

void QNavitQuick::geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry) {
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

void QNavitQuick::mouseEvent(int pressed, QMouseEvent* event) {
    struct point p;
    dbg(lvl_debug, "enter");
    p.x = event->x();
    p.y = event->y();
    switch (event->button()) {
    case Qt::LeftButton:
        callback_list_call_attr_3(graphics_priv->callbacks, attr_button, GINT_TO_POINTER(pressed), GINT_TO_POINTER(1),
                                  GINT_TO_POINTER(&p));
        break;
    case Qt::MidButton:
        callback_list_call_attr_3(graphics_priv->callbacks, attr_button, GINT_TO_POINTER(pressed), GINT_TO_POINTER(2),
                                  GINT_TO_POINTER(&p));
        break;
    case Qt::RightButton:
        callback_list_call_attr_3(graphics_priv->callbacks, attr_button, GINT_TO_POINTER(pressed), GINT_TO_POINTER(3),
                                  GINT_TO_POINTER(&p));
        break;
    default:
        break;
    }
}

void QNavitQuick::mousePressEvent(QMouseEvent* event) {
    dbg(lvl_debug, "enter");
    mouseEvent(1, event);
}

void QNavitQuick::mouseReleaseEvent(QMouseEvent* event) {
    dbg(lvl_debug, "enter");
    mouseEvent(0, event);
}

void QNavitQuick::mouseMoveEvent(QMouseEvent* event) {
    dbg(lvl_debug, "enter");
    struct point p;
    p.x = event->x();
    p.y = event->y();
    callback_list_call_attr_1(graphics_priv->callbacks, attr_motion, (void*)&p);
}

void QNavitQuick::wheelEvent(QWheelEvent* event) {
    struct point p;
    int button;
    dbg(lvl_debug, "enter");
    p.x = event->x(); // xy-coordinates of the mouse pointer
    p.y = event->y();

    if (event->delta() > 0) // wheel movement away from the person
        button = 4;
    else if (event->delta() < 0) // wheel movement towards the person
        button = 5;
    else
        button = -1;

    if (button != -1) {
        callback_list_call_attr_3(graphics_priv->callbacks, attr_button, GINT_TO_POINTER(1), GINT_TO_POINTER(button),
                                  GINT_TO_POINTER(&p));
        callback_list_call_attr_3(graphics_priv->callbacks, attr_button, GINT_TO_POINTER(0), GINT_TO_POINTER(button),
                                  GINT_TO_POINTER(&p));
    }

    event->accept();
}

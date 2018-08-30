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
// style with: clang-format -style=WebKit -i *

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
#include "QNavitWidget.h"
#include "QNavitWidget.moc"
#include "graphics_qt5.h"

QNavitWidget::QNavitWidget(struct graphics_priv* my_graphics_priv,
                           QWidget* parent,
                           Qt::WindowFlags flags)
    : QWidget(parent, flags) {
    graphics_priv = my_graphics_priv;
}

bool QNavitWidget::event(QEvent* event) {
    if (event->type() == QEvent::Gesture)
        dbg(lvl_debug, "Gesture event caught");
    //return gestureEvent(static_cast<QGestureEvent*>(event));
    return QWidget::event(event);
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
                painter->drawPixmap(value->x + value->scroll_x, value->y + value->scroll_y, *value->pixmap);
                /* draw overlays of overlay if any by recursive calling */
                paintOverlays(painter, value, event);
            }
        }
    }
}

void QNavitWidget::paintEvent(QPaintEvent* event) {
    dbg(lvl_debug, "enter (%d, %d, %d, %d)", event->rect().x(), event->rect().y(), event->rect().width(),
        event->rect().height());
    QPainter painter(this);
    /* color background if any */
    if (graphics_priv->background_graphics_gc_priv != NULL) {
        painter.setPen(*graphics_priv->background_graphics_gc_priv->pen);
        painter.fillRect(event->rect(), *graphics_priv->background_graphics_gc_priv->brush);
    }
    painter.drawPixmap(event->rect().x(), event->rect().y(), *graphics_priv->pixmap,
                       event->rect().x() - graphics_priv->scroll_x, event->rect().y() - graphics_priv->scroll_y,
                       event->rect().width(), event->rect().height());
    paintOverlays(&painter, graphics_priv, event);
}

void QNavitWidget::resizeEvent(QResizeEvent* event) {
    QPainter* painter = NULL;
    if (graphics_priv->pixmap != NULL) {
        delete graphics_priv->pixmap;
        graphics_priv->pixmap = NULL;
    }

    graphics_priv->pixmap = new QPixmap(size());
    painter = new QPainter(graphics_priv->pixmap);
    if (painter != NULL) {
        QBrush brush;
        painter->fillRect(0, 0, width(), height(), brush);
        delete painter;
    }
    dbg(lvl_debug, "size %dx%d", width(), height());
    dbg(lvl_debug, "pixmap %p %dx%d", graphics_priv->pixmap, graphics_priv->pixmap->width(),
        graphics_priv->pixmap->height());
    /* if the root window got resized, tell navit about it */
    if (graphics_priv->root)
        resize_callback(graphics_priv, width(), height());
}

void QNavitWidget::mouseEvent(int pressed, QMouseEvent* event) {
    struct point p;
    //        dbg(lvl_debug,"enter");
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

void QNavitWidget::keyPressEvent(QKeyEvent* event) {
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

void QNavitWidget::mousePressEvent(QMouseEvent* event) {
    //        dbg(lvl_debug,"enter");
    mouseEvent(1, event);
}

void QNavitWidget::mouseReleaseEvent(QMouseEvent* event) {
    //        dbg(lvl_debug,"enter");
    mouseEvent(0, event);
}

void QNavitWidget::mouseMoveEvent(QMouseEvent* event) {
    struct point p;
    //        dbg(lvl_debug,"enter");
    p.x = event->x();
    p.y = event->y();
    callback_list_call_attr_1(graphics_priv->callbacks, attr_motion, (void*)&p);
}

void QNavitWidget::wheelEvent(QWheelEvent* event) {
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

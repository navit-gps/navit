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
#include "graphics_qt_qpainter.h"
#include "RenderArea.h"

#include "RenderArea.moc"

#ifdef QT_QPAINTER_USE_EMBEDDING
EmbeddedWidget::EmbeddedWidget(struct graphics_priv *priv, QWidget* child, QWidget *parent)
    : QX11EmbedWidget(parent) {
    this->gra=priv;
    this->setWindowTitle(priv->window_title);
    QStackedLayout* _outerLayout = new QStackedLayout(this);
    this->setLayout(_outerLayout);
    _outerLayout->addWidget(child);
    _outerLayout->setCurrentWidget(child);
}

void EmbeddedWidget::closeEvent(QCloseEvent* event) {
    gra->widget->processClose();
}
#endif /* QT_QPAINTER_USE_EMBEDDING */

//##############################################################################################################
//# Description: Constructor
//# Comment: Using a QPixmap for rendering the graphics
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
RenderArea::RenderArea(struct graphics_priv *priv, QT_QPAINTER_RENDERAREA_PARENT *parent, int w, int h, int overlay)
    : QT_QPAINTER_RENDERAREA_PARENT(parent) {
    pixmap = new QPixmap(w, h);
#ifndef QT_QPAINTER_NO_WIDGET
    if (!overlay) {
#if QT_VERSION >= 0x040700
        grabGesture(Qt::PinchGesture);
        grabGesture(Qt::SwipeGesture);
        grabGesture(Qt::PanGesture);
#endif
        setWindowTitle(priv->window_title);
    }
#endif
    is_overlay=overlay;
    gra=priv;
#ifdef QT_QPAINTER_USE_EVENT_QT
    timer_type=g_hash_table_new(NULL, NULL);
    timer_callback=g_hash_table_new(NULL, NULL);
    watches=g_hash_table_new(NULL, NULL);
#ifndef QT_QPAINTER_NO_WIDGET
    setAttribute(Qt::WA_OpaquePaintEvent, true);
#endif
#endif
}

//##############################################################################################################
//# Description: QWidget:closeEvent
//# Comment: Deletes navit object and stops event loop on graphics shutdown
//##############################################################################################################
void RenderArea::processClose() {
    callback_list_call_attr_0(this->cbl, attr_window_closed);
}
void RenderArea::closeEvent(QCloseEvent* event) {
    this->processClose();
}

bool RenderArea::event(QEvent *event) {
#if QT_VERSION >= 0x040700
    if (event->type() == QEvent::Gesture) {
        dbg(lvl_debug,"gesture");
        return true;
    }
#endif
    return QWidget::event(event);
}
//##############################################################################################################
//# Description: QWidget:sizeHint
//# Comment: This property holds the recommended size for the widget
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
QSize RenderArea::sizeHint() const {
    return QSize(gra->w, gra->h);
}

//##############################################################################################################
//# Description: QWidget:paintEvent
//# Comment: A paint event is a request to repaint all or part of the widget.
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
void RenderArea::paintEvent(QPaintEvent * event) {
    qt_qpainter_draw(gra, &event->rect(), 1);
}

void RenderArea::do_resize(QSize size) {
    if (pixmap->paintingActive()) {
        pixmap->paintEngine()->painter()->end();
    }
    delete pixmap;
    pixmap=new QPixmap(size);
    pixmap->fill();
    QPainter painter(pixmap);
    QBrush brush;
    painter.fillRect(0, 0, size.width(), size.height(), brush);
    dbg(lvl_debug,"size %dx%d", size.width(), size.height());
    dbg(lvl_debug,"pixmap %p %dx%d", pixmap, pixmap->width(), pixmap->height());
    callback_list_call_attr_2(this->cbl, attr_resize, GINT_TO_POINTER(size.width()), GINT_TO_POINTER(size.height()));
}

//##############################################################################################################
//# Description: QWidget::resizeEvent()
//# Comment: When resizeEvent() is called, the widget already has its new geometry.
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
void RenderArea::resizeEvent(QResizeEvent * event) {
    if (!this->is_overlay) {
        RenderArea::do_resize(event->size());
    }
}

//##############################################################################################################
//# Description: Method to handle mouse clicks
//# Comment: Delegate of QWidget::mousePressEvent and QWidget::mouseReleaseEvent (see below)
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
void RenderArea::mouseEvent(int pressed, QMouseEvent *event) {
    struct point p;
    p.x=event->x();
    p.y=event->y();
    switch (event->button()) {
    case Qt::LeftButton:
        callback_list_call_attr_3(this->cbl, attr_button, GINT_TO_POINTER(pressed), GINT_TO_POINTER(1), GINT_TO_POINTER(&p));
        break;
    case Qt::MidButton:
        callback_list_call_attr_3(this->cbl, attr_button, GINT_TO_POINTER(pressed), GINT_TO_POINTER(2), GINT_TO_POINTER(&p));
        break;
    case Qt::RightButton:
        callback_list_call_attr_3(this->cbl, attr_button, GINT_TO_POINTER(pressed), GINT_TO_POINTER(3), GINT_TO_POINTER(&p));
        break;
    default:
        break;
    }
}

void RenderArea::mousePressEvent(QMouseEvent *event) {
    mouseEvent(1, event);
}

void RenderArea::mouseReleaseEvent(QMouseEvent *event) {
    mouseEvent(0, event);
}

//##############################################################################################################
//# Description: QWidget::mouseMoveEvent
//# Comment: If mouse tracking is switched on, mouse move events occur even if no mouse button is pressed.
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
void RenderArea::mouseMoveEvent(QMouseEvent *event) {
    struct point p;
    p.x=event->x();
    p.y=event->y();
    callback_list_call_attr_1(this->cbl, attr_motion, (void *)&p);
}


//##############################################################################################################
//# Description: Qt Event :: Zoom in/out with the mouse's scrollwheel
//# Comment:
//# Authors: Stefan Klumpp (04/2008)
//##############################################################################################################
void RenderArea::wheelEvent(QWheelEvent *event) {
    struct point p;
    int button;

    p.x=event->x();	// xy-coordinates of the mouse pointer
    p.y=event->y();

    if (event->delta() > 0)	// wheel movement away from the person
        button=4;
    else if (event->delta() < 0) // wheel movement towards the person
        button=5;
    else
        button=-1;

    if (button != -1) {
        callback_list_call_attr_3(this->cbl, attr_button, GINT_TO_POINTER(1), GINT_TO_POINTER(button), GINT_TO_POINTER(&p));
        callback_list_call_attr_3(this->cbl, attr_button, GINT_TO_POINTER(0), GINT_TO_POINTER(button), GINT_TO_POINTER(&p));
    }

    event->accept();
}

#define CASE(x) case x:

void RenderArea::keyPressEvent(QKeyEvent *event) {
    QString str=event->text();
    const char *text=str.toUtf8().constData();
    dbg(lvl_debug,"enter text='%s' 0x%x (%zu) key=%d", text, text[0], strlen(text), event->key());
    if (!text || !text[0] || text[0] == 0x7f) {
        dbg(lvl_debug,"special key");
        switch (event->key()) {
        case 4099: {
            char text_backspace[] = {NAVIT_KEY_BACKSPACE,'\0'};
            text=text_backspace;
        }
        break;
        case 4101:
#ifdef QT_QPAINTER_CUSTOM_RETURN
            QT_QPAINTER_CUSTOM_RETURN
#endif
            {
                char text_return[] = {NAVIT_KEY_RETURN,'\0'};
                text=text_return;
            }
            break;
        case 4114:
#ifdef QT_QPAINTER_CUSTOM_LEFT
            QT_QPAINTER_CUSTOM_LEFT
#endif
            {
                char text_left[] = {NAVIT_KEY_LEFT,'\0'};
                text=text_left;
            }
            break;
        case 4115:
#ifdef QT_QPAINTER_CUSTOM_UP
            QT_QPAINTER_CUSTOM_UP
#endif
            {
                char text_up[] = {NAVIT_KEY_UP,'\0'};
                text=text_up;
            }
            break;
        case 4116:
#ifdef QT_QPAINTER_CUSTOM_RIGHT
            QT_QPAINTER_CUSTOM_RIGHT
#endif
            {
                char text_right[] = {NAVIT_KEY_RIGHT,'\0'};
                text=text_right;
            }
            break;
        case 4117:
#ifdef QT_QPAINTER_CUSTOM_DOWN
            QT_QPAINTER_CUSTOM_DOWN
#endif
            {
                char text_down[] = {NAVIT_KEY_DOWN,'\0'};
                text=text_down;
            }
            break;
        }
    }
    callback_list_call_attr_1(this->cbl, attr_keypress, (void *)text);
    event->accept();
}

void RenderArea::watchEvent(int fd) {
#ifdef QT_QPAINTER_USE_EVENT_QT
    struct event_watch *ev=(struct event_watch *)g_hash_table_lookup(watches, (void *)fd);
    dbg(lvl_debug,"fd=%d ev=%p cb=%p", fd, ev, ev->cb);
    callback_call_0(ev->cb);
#endif
}

#ifdef QT_QPAINTER_USE_EVENT_QT
void RenderArea::timerEvent(QTimerEvent *event) {
    int id=event->timerId();
    struct callback *cb=(struct callback *)g_hash_table_lookup(timer_callback, (void *)id);
    if (cb)
        callback_call_0(cb);
    if (!g_hash_table_lookup(timer_type, (void *)id))
        event_qt_remove_timeout((struct event_timeout *)id);
}
#endif /* QT_QPAINTER_USE_EVENT_QT */


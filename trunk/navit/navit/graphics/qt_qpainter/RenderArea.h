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
#ifndef __RENDERAREA_H
#define __RENDERAREA_H

#ifdef QT_QPAINTER_USE_EMBEDDING
class EmbeddedWidget : public QX11EmbedWidget {
        struct graphics_priv *gra;
public:
        EmbeddedWidget(struct graphics_priv *priv, QWidget* child, QWidget *parent = NULL);
protected:
        void closeEvent(QCloseEvent *event);
};
#endif

class RenderArea : public QT_QPAINTER_RENDERAREA_PARENT
{
     Q_OBJECT
 public:
     RenderArea(struct graphics_priv *priv, QT_QPAINTER_RENDERAREA_PARENT *parent = 0, int w=800, int h=800, int overlay=0);
     void do_resize(QSize size);
     QPixmap *pixmap;
     struct callback_list *cbl;
     struct graphics_priv *gra;

#ifdef QT_QPAINTER_USE_EVENT_QT
     GHashTable *timer_type;
     GHashTable *timer_callback;
     GHashTable *watches;
#endif

     void processClose();
protected:
     int is_overlay;
     QSize sizeHint() const;
     void paintEvent(QPaintEvent *event);
     void resizeEvent(QResizeEvent *event);
     void mouseEvent(int pressed, QMouseEvent *event);
     void mousePressEvent(QMouseEvent *event);
     void mouseReleaseEvent(QMouseEvent *event);
     void mouseMoveEvent(QMouseEvent *event);
     void wheelEvent(QWheelEvent *event);
     void keyPressEvent(QKeyEvent *event);
     void closeEvent(QCloseEvent *event);
     bool event(QEvent *event);
#ifdef QT_QPAINTER_USE_EVENT_QT
     void timerEvent(QTimerEvent *event);
#endif
 protected slots:
     void watchEvent(int fd);
 };

#endif /* __RENDERAREA_H */

/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2017 Navit Team
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

#ifndef __QNavitWidget_h
#define __QNavitWidget_h
class QNavitWidget;
#include "graphics_qt5.h"
#include <QEvent>
#include <QMouseEvent>
#include <QPixmap>
#include <QWheelEvent>
#include <QWidget>

class QNavitWidget : public QWidget {
    Q_OBJECT
public: QNavitWidget(struct graphics_priv* my_graphics_priv,
                         QWidget* parent,
                         Qt::WindowFlags flags);

protected:
    virtual bool event(QEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void paintEvent(QPaintEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void mouseEvent(int pressed, QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void wheelEvent(QWheelEvent* event);

private:
    struct graphics_priv* graphics_priv;
};
#endif

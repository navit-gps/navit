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

#include <QObject>
#include <glib.h>

class qt5_navit_timer : public QObject {
    Q_OBJECT
public: qt5_navit_timer(QObject* parent = 0);
    GHashTable* timer_type;
    GHashTable* timer_callback;
    GHashTable* watches;

public slots:
    void watchEvent(int id);

protected:
    void timerEvent(QTimerEvent* event);
};

void qt5_event_init(void);

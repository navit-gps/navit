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

#include <time.h>
#include <glib.h>
#include "coord.h"
#include "item.h"
#include "route.h"
#include "speech.h"
#include "phrase.h"

void phrase_route_calc(void *speech) {
#if 0
    if (! speech)
        return;
    speech_say(speech,"Die Route wird berechnet\n");
#endif
}

void phrase_route_calculated(void *speech, void *route) {
#if 0
    struct tm *eta;
#endif
    if (! speech)
        return;

#if 0 /* FIXME */
    eta=route_get_eta(route);

    speech_sayf(speech,"Die Route wurde berechnet, geschätzte Ankunftszeit %d Uhr %d  Entfernung %4.0f Kilometer",
                eta->tm_hour,eta->tm_min,route_get_len(route)/1000);
#endif

}

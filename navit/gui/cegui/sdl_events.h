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

#include "SDL/SDL.h"
#include "CEGUI.h"



#define SRCH_COUNTRY 1
#define SRCH_TOWN 2
#define SRCH_STREET 3
#define SRCH_NUMBER 4



bool handleItemSelect(int r);
bool ItemSelect(const CEGUI::EventArgs& event);
bool handleMouseEnters(const CEGUI::EventArgs& event);
void handle_destination_change();

bool DestinationEntryChange(const CEGUI::EventArgs& event);
bool DestinationWindowSwitch(const CEGUI::EventArgs& event);
bool BookmarkSelectionSwitch(const CEGUI::EventArgs& event);
bool BookmarkSelect(const CEGUI::EventArgs& event);
bool FormerDestSelectionSwitch(const CEGUI::EventArgs& event);
bool FormerDestSelect(const CEGUI::EventArgs& event);
bool AddressSearchSwitch(const CEGUI::EventArgs& event);
bool Switch_to_nGhost(const CEGUI::EventArgs& event);
bool RoadBookSwitch(const CEGUI::EventArgs& event);
bool ButtonGo(const CEGUI::EventArgs& event);
bool ZoomIn(const CEGUI::EventArgs& event);
bool ZoomOut(const CEGUI::EventArgs& event);
bool ButtonQuit(const CEGUI::EventArgs& event);

void inject_time_pulse(double& last_time_pulse);

bool Handle_Virtual_Key_Down(const CEGUI::EventArgs& event);

CEGUI::uint SDLKeyToCEGUIKey(SDLKey key);

void inject_input(bool& must_quit);



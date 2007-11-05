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
bool DialogWindowSwitch(const CEGUI::EventArgs& event);
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



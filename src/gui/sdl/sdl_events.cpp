#include "CEGUI.h"
#include "sdl_events.h"
#include "gui_sdl.h"

#include <CEGUI/RendererModules/OpenGLGUIRenderer/openglrenderer.h>

//  FIXME temporary fix for enum
#include "projection.h"
#include "item.h"
#include "navit.h"
#include "debug.h"
#include "track.h"
#include "search.h"
#include "coord.h"
#include "country.h"
#include "string.h"

// Library for window switching (-> nGhost)
#include "wmcontrol.h"


struct sdl_destination{
	int country;
	int town;
	int town_street_assoc;
	int current_search;
} SDL_dest;


static struct search_param {
	struct navit *nav;
	struct mapset *ms;
	struct search_list *sl;
	struct attr attr;
} search_param;

// extern "C" struct navit *global_navit;
// 

void route_to(int x,int y){
	struct coord pos;
	pos.x=x;
	pos.y=y; 
	using namespace CEGUI;
	extern struct navit *sdl_gui_navit;

	try {
		WindowManager::getSingleton().getWindow("DestinationWindow")->hide();
		WindowManager::getSingleton().getWindow("Navit/Routing/Tips")->show();

// 		WindowManager::getSingleton().getWindow("Navit/ProgressWindow")->show();
	// 	route_set_destination(co->route, &pos);
		// I could have been using search->nav instead of sdl_gui_navit. is it better this way?
	
// 		WindowManager::getSingleton().getWindow("Navit/ProgressWindow")->hide();
// 		WindowManager::getSingleton().getWindow("OSD/RoadbookButton")->show();
// 		WindowManager::getSingleton().getWindow("OSD/ETA")->show();
	}
	catch (CEGUI::Exception& e)
	{
		fprintf(stderr,"CEGUI Exception occured: \n%s\n", e.getMessage().c_str());
		printf("Missing control!...\n");
	}
		navit_set_destination(sdl_gui_navit, &pos, "FIXME");

}


bool handleItemSelect(int r)
{
	using namespace CEGUI;
	extern CEGUI::Window* myRoot;

	MultiColumnList* mcl = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("DestinationWindow/Listbox"));
	
	ListboxItem * item = mcl->getItemAtGridReference(MCLGridRef(r,0));
	ListboxItem * itemid = mcl->getItemAtGridReference(MCLGridRef(r,1));
	ListboxItem * item_assoc = mcl->getItemAtGridReference(MCLGridRef(r,2));


	Window* country_edit = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/CountryEditbox"));
	Window* twn_edit = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/TownEditbox"));
	Window* street_edit = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/StreetEditbox"));

	if(SDL_dest.current_search==SRCH_COUNTRY){
		country_edit->setText(item->getText());
		// FIXME Need to record the country here so all searches are made by default in this country
		twn_edit->activate();
		SDL_dest.current_search=SRCH_TOWN;
		myRoot->getChild("Navit/Keyboard")->getChild("Navit/Keyboard/Input")->setText("");

	} else 	if(SDL_dest.current_search==SRCH_TOWN){
		twn_edit->setText(item->getText());

		ListboxItem * itemx = mcl->getItemAtGridReference(MCLGridRef(r,3));
		ListboxItem * itemy = mcl->getItemAtGridReference(MCLGridRef(r,4));
	
		Window* Dest_x = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/Dest_x"));
		Dest_x->setText(itemx->getText().c_str());

		Window* Dest_y = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/Dest_y"));
		Dest_y->setText(itemy->getText().c_str());

		mcl->resetList();

		SDL_dest.current_search=SRCH_STREET;
		street_edit->activate();
		myRoot->getChild("Navit/Keyboard")->getChild("Navit/Keyboard/Input")->setText("");

	} else if(SDL_dest.current_search==SRCH_STREET){
		street_edit->setText(item->getText());

		myRoot->getChild("Navit/Keyboard")->hide();

		ListboxItem * itemx = mcl->getItemAtGridReference(MCLGridRef(r,3));
		ListboxItem * itemy = mcl->getItemAtGridReference(MCLGridRef(r,4));
	
		Window* Dest_x = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/Dest_x"));
		Dest_x->setText(itemx->getText().c_str());

		Window* Dest_y = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/Dest_y"));
		Dest_y->setText(itemy->getText().c_str());

		mcl->resetList();

		SDL_dest.current_search=SRCH_STREET;

		myRoot->getChild("Navit/Keyboard")->getChild("Navit/Keyboard/Input")->setText("");


		/*
		ListboxItem * itemid = mcl->getItemAtGridReference(MCLGridRef(r,1));
		int segment_id=atoi(itemid->getText().c_str());
		printf("street seg id : %li\n",segment_id);

		extern struct container *co;
		struct block_info res_blk_inf;
		struct street_str *res_str;
		street_get_by_id(co->map_data, 33, segment_id,&res_blk_inf,&res_str );

		struct street_coord * streetcoord;
		streetcoord=street_coord_get(&res_blk_inf,res_str);

		printf("Street coordinates : %i,%i\n",streetcoord->c->x,streetcoord->c->y);

	 	char xbuff [256];
		sprintf(xbuff,"%li",streetcoord->c->x);
	 	char ybuff [256];
		sprintf(ybuff,"%li",streetcoord->c->y);

		Window* Dest_x = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/Dest_x"));
		Dest_x->setText(xbuff);

		Window* Dest_y = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/Dest_y"));
		Dest_y->setText(ybuff);

		struct street_name name;
// 		printf("street_name_get_by_id returns : %i\n",street_name_get_by_id(&name, res_blk_inf.mdata, res_str->nameid));
		street_name_get_by_id(&name, res_blk_inf.mdata, res_str->nameid);
// 		printf("name1:%s / name2%s\n",name.name1,name.name2);

		struct street_name_number_info num;
		struct street_name_info inf;

		SDL_dest.current_search=SRCH_NUMBER;
		mcl->resetList();

		while (street_name_get_info(&inf, &name)) {
			while(street_name_get_number_info(&num,&inf)){
// 				printf(" House Number : %i -> %i\n",num.first,num.last);
				for(int i=num.first;i<=num.last;i+=2){
					add_number_to_list(i,num.c->x,num.c->y);
				}
			}
		}
		*/
// 		route_to(streetcoord->c->x,streetcoord->c->y);
	} else if (SDL_dest.current_search==SRCH_NUMBER){

		struct coord pos;
		ListboxItem * itemx = mcl->getItemAtGridReference(MCLGridRef(r,3));
		ListboxItem * itemy = mcl->getItemAtGridReference(MCLGridRef(r,4));
	
		pos.x=atoi(itemx->getText().c_str());
		pos.y=atoi(itemy->getText().c_str());

		route_to(pos.x,pos.y);
	}

	return true;
}



bool ItemSelect(const CEGUI::EventArgs& event)
{
	using namespace CEGUI;

	MultiColumnList* mcl = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("DestinationWindow/Listbox"));
	ListboxItem * item = mcl->getFirstSelectedItem();
	handleItemSelect(mcl->getItemRowIndex(item));
}

bool handleMouseEnters(const CEGUI::EventArgs& event)
{
	// FIXME this whole function could maybe be removed
	const CEGUI::WindowEventArgs& we =  static_cast<const CEGUI::WindowEventArgs&>(event);

	// FIXME theses variables should be shared
	extern CEGUI::OpenGLRenderer* renderer;
	extern CEGUI::Window* myRoot;

	using namespace CEGUI;
	myRoot->getChild("Navit/Keyboard")->getChild("Navit/Keyboard/Input")->setText("");
	MultiColumnList* mcl = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("DestinationWindow/Listbox"));

	String senderID = we.window->getName();

	if (senderID == "DestinationWindow/CountryEditbox"){
		// First, clean off the Street and Town Editbox
		extern Window* myRoot;
		Window* town_edit = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/TownEditbox"));
		town_edit->setText("");
		Window* street_edit = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/StreetEditbox"));
		street_edit->setText("");
		SDL_dest.current_search=SRCH_COUNTRY;

	} else if (senderID == "DestinationWindow/TownEditbox"){
		// First, clean off the Street Editbox
		extern Window* myRoot;
		Window* street_edit = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/StreetEditbox"));
		street_edit->setText("");
		SDL_dest.current_search=SRCH_TOWN;

	} else if (senderID == "DestinationWindow/StreetEditbox"){
		// First, make sure the user selected an entry in the town choice. If he hadn't, select the first for him.
  		if(SDL_dest.current_search==SRCH_TOWN){
			if (mcl->getRowCount()>0)
			{
				handleItemSelect(0);
			}			
		}
		SDL_dest.current_search=SRCH_STREET;

	}
}


void handle_destination_change(){

	using namespace CEGUI;
	extern CEGUI::Window* myRoot;

	struct search_param *search=&search_param;
	struct search_list_result *res;

	MultiColumnList* mcl = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("DestinationWindow/Listbox"));


	if (SDL_dest.current_search==SRCH_COUNTRY)
	{	
		Editbox* country_edit = static_cast<Editbox*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/CountryEditbox"));
		String content=country_edit->getText();

		mcl->resetList();
		dbg(0,"Starting a country search : %s\n",content.c_str());

		search->attr.type=attr_country_all;

		// FIXME the following codeblock could be shared between country, town and street search
		search->attr.u.str=(char *)content.c_str();

		search_list_search(search->sl, &search->attr, 1);
		while((res=search_list_get_result(search->sl))) {
			ListboxTextItem* itemListbox = new ListboxTextItem(res->country->name);
			itemListbox->setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush");

			mcl->addRow(itemListbox,0);
		}

	} else if (SDL_dest.current_search==SRCH_TOWN)
	{	
		
		Editbox* town_edit = static_cast<Editbox*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/TownEditbox"));
		String content=town_edit->getText();


		mcl->resetList();

		if(strlen(content.c_str())<4){

		}  else {
			// dbg(1,"town searching for %s\n",content.c_str());
			search->attr.type=attr_town_name;
			search->attr.u.str=(char *)content.c_str();

			search_list_search(search->sl, &search->attr, 1);
			while((res=search_list_get_result(search->sl))) {
				ListboxTextItem* itemListbox = new ListboxTextItem(res->town->name);
				itemListbox->setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush");

				mcl->addRow(itemListbox,0);

				char x [256];
				sprintf(x,"%li",res->c->x);
				ListboxTextItem* xitem = new ListboxTextItem(x);
			
				char y [256];
				sprintf(y,"%li",res->c->y);
			
				ListboxTextItem* yitem = new ListboxTextItem(y);
// 				item->setSelectionBrushImage(&ImagesetManager::getSingleton().getImageset("TaharezLook")->getImage("MultiListSelectionBrush"));
			
				try
				{
					mcl->setItem(xitem, 3, mcl->getRowCount()-1);
					mcl->setItem(yitem, 4, mcl->getRowCount()-1);
				}
				// something went wrong, so cleanup the ListboxTextItem.
				catch (InvalidRequestException)
				{
// 					delete item;
				}
	
			}

		}


	} else if (SDL_dest.current_search==SRCH_STREET)
	{	
		Editbox* street_edit = static_cast<Editbox*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/StreetEditbox"));
		
		String content=street_edit->getText();
		if(strlen(content.c_str())<1){

		}  else {
			// dbg(1,"street searching for %s\n",content.c_str());
			search->attr.type=attr_street_name;
			search->attr.u.str=(char *)content.c_str();

			mcl->resetList();

			search_list_search(search->sl, &search->attr, 1);
			while((res=search_list_get_result(search->sl))) {
				ListboxTextItem* itemListbox = new ListboxTextItem(res->street->name);
				itemListbox->setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush");

				mcl->addRow(itemListbox,0);

				char x [256];
				sprintf(x,"%li",res->c->x);
				ListboxTextItem* xitem = new ListboxTextItem(x);
			
				char y [256];
				sprintf(y,"%li",res->c->y);
			
				ListboxTextItem* yitem = new ListboxTextItem(y);
// 				item->setSelectionBrushImage(&ImagesetManager::getSingleton().getImageset("TaharezLook")->getImage("MultiListSelectionBrush"));
			
				try
				{
					mcl->setItem(xitem, 3, mcl->getRowCount()-1);
					mcl->setItem(yitem, 4, mcl->getRowCount()-1);
				}
				// something went wrong, so cleanup the ListboxTextItem.
				catch (InvalidRequestException)
				{
// 					delete item;
				}

	
			}
	
// 			street_name_search(search->map_data, 33, SDL_dest.town_street_assoc, content.c_str(), 1, destination_street_add, search);
		}
	}

}


bool DestinationEntryChange(const CEGUI::EventArgs& event)
{
	handleMouseEnters(event);
	handle_destination_change();

	return true;
}

bool DialogWindowSwitch(const CEGUI::EventArgs& event)
{
	using namespace CEGUI;


	if(sdl_gui_navit){	
	} else {
		printf("*** Invalid navit instance in sdl_events\n");
	}
	struct search_param *search=&search_param;

 	// dbg(1,"search->nav=sdl_gui_navit;\n");
 	search->nav=sdl_gui_navit;
 	// dbg(1,"search->ms=navit_get_mapset(sdl_gui_navit);\n");
 	search->ms=navit_get_mapset(sdl_gui_navit);
 	// dbg(1,"search->sl=search_list_new(search->ms);\n");
 	search->sl=search_list_new(search->ms);


	extern CEGUI::Window* myRoot;
	const CEGUI::WindowEventArgs& we =  static_cast<const CEGUI::WindowEventArgs&>(event);
	if(we.window->getParent()->getChild("DestinationWindow")->isVisible()){
		we.window->getParent()->getChild("DestinationWindow")->hide();
	} else {
		Window* town_edit = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/TownEditbox"));
		town_edit->setText("");
		Window* street_edit = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/StreetEditbox"));
		street_edit->setText("");
		town_edit->activate();

		SDL_dest.current_search=SRCH_COUNTRY;
		MultiColumnList* mcl = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("DestinationWindow/Listbox"));
		mcl->resetList();
		
		
		// Code to get the current country and set it by default for the search
		struct attr search_attr, country_name, *country_attr;
		struct tracking *tracking;
		struct country_search *cs;
		struct item *item;


		Editbox* country_edit = static_cast<Editbox*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/CountryEditbox"));

		country_attr=country_default();
		tracking=navit_get_tracking(sdl_gui_navit);
		if (tracking && tracking_get_current_attr(tracking, attr_country_id, &search_attr)) 
			country_attr=&search_attr;
		cs=country_search_new(country_attr, 0);
		item=country_search_get_item(cs);
		if (item && item_attr_get(item, attr_country_name, &country_name)){
			country_edit->setText(country_name.u.str);
			handle_destination_change();
			}
		country_search_destroy(cs);

		/*
		// This code should 'guess' your country based upon your locale settings.
		// useful if nothing else worked
		char * lc_lang;
		lc_lang = getenv ("LANG");
		if (lc_lang!=NULL){
			char lang_code [3];
			strncpy(lang_code, lc_lang+3, 2);
			lang_code[2]='\0';
			dbg(0,"LC_LANG = %s -> %s\n",lc_lang,lang_code);

			struct search_param *search=&search_param;
			struct search_list_result *res;
			search->attr.type=attr_country_iso2;
			search->attr.u.str=lang_code;
			dbg(0,"launching iso2 search\n");	
			search_list_search(search->sl, &search->attr, 1);
			dbg(0,"got result. ready to parse\n");
 			while((res=search_list_get_result(search->sl))) {
 				dbg(0," got country : \n",res->country->name);
 			}
			dbg(0,"done parsing\n");
		}

		*/

		we.window->getParent()->getChild("DestinationWindow")->show();
	}


	return true;
}

bool Switch_to_nGhost(const CEGUI::EventArgs& event)
{
	printf("Switching to nGhost\n");
	if (window_switch("Nanonymous")==EXIT_FAILURE)
	{
		popen("nghost","r");
	}

}


bool RoadBookSwitch(const CEGUI::EventArgs& event)
{
	using namespace CEGUI;
	extern CEGUI::Window* myRoot;

// 	const CEGUI::WindowEventArgs& we =  static_cast<const CEGUI::WindowEventArgs&>(event);
	if(myRoot->getChild("Navit/RoadBook")->isVisible()){
		myRoot->getChild("Navit/RoadBook")->hide();
		WindowManager::getSingleton().getWindow("OSD/RoadbookButton")->show();
	} else {
		myRoot->getChild("Navit/RoadBook")->show();
		WindowManager::getSingleton().getWindow("OSD/RoadbookButton")->hide();
	}
	return true;
}

bool ButtonGo(const CEGUI::EventArgs& event)
{
	using namespace CEGUI;
	extern CEGUI::Window* myRoot;
	
	MultiColumnList* mcl = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("DestinationWindow/Listbox"));
	// First, make sure the user selected an entry in the town choice. If he hadn't, select the first for him.
	if(SDL_dest.current_search==SRCH_TOWN){
		if (mcl->getRowCount()>0)
		{
			handleItemSelect(0);
		}
	}


	Window* Dest_x = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/Dest_x"));
	Window* Dest_y = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/Dest_y"));
	
	extern struct navit *sdl_gui_navit;
 	route_to(atoi(Dest_x->getText().c_str()),atoi(Dest_y->getText().c_str()));

	return true;
}


bool ZoomIn(const CEGUI::EventArgs& event)
{
	extern struct navit *sdl_gui_navit;
	navit_zoom_in(sdl_gui_navit, 2);
	/*
	extern struct container *co;
	struct transformation *t=co->trans;
	if(t->scale>1){
		t->scale/=2;
	}
	*/

}

bool ZoomOut(const CEGUI::EventArgs& event)
{
	extern struct navit *sdl_gui_navit;
	navit_zoom_out(sdl_gui_navit, 2);
	/*
	extern struct container *co;
	struct transformation *t=co->trans;
	t->scale*=2;
	*/
}

bool ButtonQuit(const CEGUI::EventArgs& event)
{
	exit(0);
}

bool Handle_Virtual_Key_Down(const CEGUI::EventArgs& event){
	
	using namespace CEGUI;

	extern CEGUI::Window* myRoot;

	const CEGUI::WindowEventArgs& we =  static_cast<const CEGUI::WindowEventArgs&>(event);
	String senderID = we.window->getName();

	Window* editbox = myRoot->getChild("Navit/Keyboard")->getChild("Navit/Keyboard/Input");
	String content=editbox->getText();


	if(senderID=="OK"){
		// dbg(1,"Validating : %s\n",content.c_str());
		myRoot->getChild("Navit/Keyboard")->hide();
		return 0;
	} else if(senderID=="BACK"){
		content=content.substr(0, content.length()-1);
		editbox->setText(content);
	} else {
		content+=senderID;
		editbox->setText(content);
	}

	Window* country_edit = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/CountryEditbox"));
	Window* town_edit = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/TownEditbox"));
	Window* street_edit = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/StreetEditbox"));

	switch (SDL_dest.current_search) {
		case SRCH_COUNTRY:
			country_edit->setText(content);
			break;
		case SRCH_TOWN :
			town_edit->setText(content);
			break;
		case SRCH_STREET :
			street_edit->setText(content);
			break;
	}
	handle_destination_change();
}



// Nothing really interesting below.

void handle_mouse_down(Uint8 button)
{
	switch ( button )
	{
		// handle real mouse buttons
		case SDL_BUTTON_LEFT:
			CEGUI::System::getSingleton().injectMouseButtonDown(CEGUI::LeftButton);
			break;
		case SDL_BUTTON_MIDDLE:
			CEGUI::System::getSingleton().injectMouseButtonDown(CEGUI::MiddleButton);
			break;
		case SDL_BUTTON_RIGHT:
			CEGUI::System::getSingleton().injectMouseButtonDown(CEGUI::RightButton);
			break;
		
		// handle the mouse wheel
		case SDL_BUTTON_WHEELDOWN:
			CEGUI::System::getSingleton().injectMouseWheelChange( -1 );
			break;
		case SDL_BUTTON_WHEELUP:
			CEGUI::System::getSingleton().injectMouseWheelChange( +1 );
			break;
	}
}


void handle_mouse_up(Uint8 button)
{
	switch ( button )
	{
	case SDL_BUTTON_LEFT:
		CEGUI::System::getSingleton().injectMouseButtonUp(CEGUI::LeftButton);
		break;
	case SDL_BUTTON_MIDDLE:
		CEGUI::System::getSingleton().injectMouseButtonUp(CEGUI::MiddleButton);
		break;
	case SDL_BUTTON_RIGHT:
		CEGUI::System::getSingleton().injectMouseButtonUp(CEGUI::RightButton);
		break;
	}
}

void inject_time_pulse(double& last_time_pulse)
{
	// get current "run-time" in seconds
	double t = 0.001*SDL_GetTicks();

	// inject the time that passed since the last call 
	CEGUI::System::getSingleton().injectTimePulse( float(t-last_time_pulse) );

	// store the new time as the last time
	last_time_pulse = t;
}

 CEGUI::uint SDLKeyToCEGUIKey(SDLKey key)
 {
     using namespace CEGUI;
     switch (key)
     {
     case SDLK_BACKSPACE:    return Key::Backspace;
     case SDLK_TAB:          return Key::Tab;
     case SDLK_RETURN:       return Key::Return;
     case SDLK_PAUSE:        return Key::Pause;
     case SDLK_ESCAPE:       return Key::Escape;
     case SDLK_SPACE:        return Key::Space;
     case SDLK_COMMA:        return Key::Comma;
     case SDLK_MINUS:        return Key::Minus;
     case SDLK_PERIOD:       return Key::Period;
     case SDLK_SLASH:        return Key::Slash;
     case SDLK_0:            return Key::Zero;
     case SDLK_1:            return Key::One;
     case SDLK_2:            return Key::Two;
     case SDLK_3:            return Key::Three;
     case SDLK_4:            return Key::Four;
     case SDLK_5:            return Key::Five;
     case SDLK_6:            return Key::Six;
     case SDLK_7:            return Key::Seven;
     case SDLK_8:            return Key::Eight;
     case SDLK_9:            return Key::Nine;
     case SDLK_COLON:        return Key::Colon;
     case SDLK_SEMICOLON:    return Key::Semicolon;
     case SDLK_EQUALS:       return Key::Equals;
     case SDLK_LEFTBRACKET:  return Key::LeftBracket;
     case SDLK_BACKSLASH:    return Key::Backslash;
     case SDLK_RIGHTBRACKET: return Key::RightBracket;
     case SDLK_a:            return Key::A;
     case SDLK_b:            return Key::B;
     case SDLK_c:            return Key::C;
     case SDLK_d:            return Key::D;
     case SDLK_e:            return Key::E;
     case SDLK_f:            return Key::F;
     case SDLK_g:            return Key::G;
     case SDLK_h:            return Key::H;
     case SDLK_i:            return Key::I;
     case SDLK_j:            return Key::J;
     case SDLK_k:            return Key::K;
     case SDLK_l:            return Key::L;
     case SDLK_m:            return Key::M;
     case SDLK_n:            return Key::N;
     case SDLK_o:            return Key::O;
     case SDLK_p:            return Key::P;
     case SDLK_q:            return Key::Q;
     case SDLK_r:            return Key::R;
     case SDLK_s:            return Key::S;
     case SDLK_t:            return Key::T;
     case SDLK_u:            return Key::U;
     case SDLK_v:            return Key::V;
     case SDLK_w:            return Key::W;
     case SDLK_x:            return Key::X;
     case SDLK_y:            return Key::Y;
     case SDLK_z:            return Key::Z;
     case SDLK_DELETE:       return Key::Delete;
     case SDLK_KP0:          return Key::Numpad0;
     case SDLK_KP1:          return Key::Numpad1;
     case SDLK_KP2:          return Key::Numpad2;
     case SDLK_KP3:          return Key::Numpad3;
     case SDLK_KP4:          return Key::Numpad4;
     case SDLK_KP5:          return Key::Numpad5;
     case SDLK_KP6:          return Key::Numpad6;
     case SDLK_KP7:          return Key::Numpad7;
     case SDLK_KP8:          return Key::Numpad8;
     case SDLK_KP9:          return Key::Numpad9;
     case SDLK_KP_PERIOD:    return Key::Decimal;
     case SDLK_KP_DIVIDE:    return Key::Divide;
     case SDLK_KP_MULTIPLY:  return Key::Multiply;
     case SDLK_KP_MINUS:     return Key::Subtract;
     case SDLK_KP_PLUS:      return Key::Add;
     case SDLK_KP_ENTER:     return Key::NumpadEnter;
     case SDLK_KP_EQUALS:    return Key::NumpadEquals;
     case SDLK_UP:           return Key::ArrowUp;
     case SDLK_DOWN:         return Key::ArrowDown;
     case SDLK_RIGHT:        return Key::ArrowRight;
     case SDLK_LEFT:         return Key::ArrowLeft;
     case SDLK_INSERT:       return Key::Insert;
     case SDLK_HOME:         return Key::Home;
     case SDLK_END:          return Key::End;
     case SDLK_PAGEUP:       return Key::PageUp;
     case SDLK_PAGEDOWN:     return Key::PageDown;
     case SDLK_F1:           return Key::F1;
     case SDLK_F2:           return Key::F2;
     case SDLK_F3:           return Key::F3;
     case SDLK_F4:           return Key::F4;
     case SDLK_F5:           return Key::F5;
     case SDLK_F6:           return Key::F6;
     case SDLK_F7:           return Key::F7;
     case SDLK_F8:           return Key::F8;
     case SDLK_F9:           return Key::F9;
     case SDLK_F10:          return Key::F10;
     case SDLK_F11:          return Key::F11;
     case SDLK_F12:          return Key::F12;
     case SDLK_F13:          return Key::F13;
     case SDLK_F14:          return Key::F14;
     case SDLK_F15:          return Key::F15;
     case SDLK_NUMLOCK:      return Key::NumLock;
     case SDLK_SCROLLOCK:    return Key::ScrollLock;
     case SDLK_RSHIFT:       return Key::RightShift;
     case SDLK_LSHIFT:       return Key::LeftShift;
     case SDLK_RCTRL:        return Key::RightControl;
     case SDLK_LCTRL:        return Key::LeftControl;
     case SDLK_RALT:         return Key::RightAlt;
     case SDLK_LALT:         return Key::LeftAlt;
     case SDLK_LSUPER:       return Key::LeftWindows;
     case SDLK_RSUPER:       return Key::RightWindows;
     case SDLK_SYSREQ:       return Key::SysRq;
     case SDLK_MENU:         return Key::AppMenu;
     case SDLK_POWER:        return Key::Power;
     default:                return 0;
     }
     return 0;
 }


void inject_input(bool& must_quit)
{
  SDL_Event e;
  
  // go through all available events
  while (SDL_PollEvent(&e))
  {
    // we use a switch to determine the event type
    switch (e.type)
    {
      // mouse motion handler
      case SDL_MOUSEMOTION:
        // we inject the mouse position directly.
        CEGUI::System::getSingleton().injectMousePosition(
          static_cast<float>(e.motion.x),
          static_cast<float>(e.motion.y)
        );
        break;
    
    // mouse down handler
      case SDL_MOUSEBUTTONDOWN:
        // let a special function handle the mouse button down event
        handle_mouse_down(e.button.button);
        break;
    
      // mouse up handler
      case SDL_MOUSEBUTTONUP:
        // let a special function handle the mouse button up event
        handle_mouse_up(e.button.button);
        break;
    
    
      // key down
      case SDL_KEYDOWN:
        // to tell CEGUI that a key was pressed, we inject the scancode, translated from SDL
        CEGUI::System::getSingleton().injectKeyDown(SDLKeyToCEGUIKey(e.key.keysym.sym));
        
        // as for the character it's a litte more complicated. we'll use for translated unicode value.
        // this is described in more detail below.
        if ((e.key.keysym.unicode & 0xFF80) == 0)
        {
          CEGUI::System::getSingleton().injectChar(e.key.keysym.unicode & 0x7F);
        }
        break;
    
      // key up
      case SDL_KEYUP:
        // like before we inject the scancode translated from SDL.
        CEGUI::System::getSingleton().injectKeyUp(SDLKeyToCEGUIKey(e.key.keysym.sym));
        break;
    
    
      // WM quit event occured
      case SDL_QUIT:
        must_quit = true;
        break;
    
    }
  
  }

}

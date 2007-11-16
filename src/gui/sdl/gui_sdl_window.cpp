#include "glib.h"
#include <stdio.h>

//  FIXME temporary fix for enum
#include "projection.h"

#include "item.h"
#include "navit.h"
#include "vehicle.h"	
#include "profile.h"
#include "transform.h"
#include "gui.h"
#include "coord.h"
#include "plugin.h"
#include "callback.h"
#include "point.h"
#include "graphics.h"
#include "gui_sdl.h"
#include "navigation.h"
#include "debug.h"
#include "attr.h"

#include "CEGUI.h"

// FIXME This is for 3d fonts. Needs QuesoGLC. Could probably (and should) be moved to graphics instead
// since fonts here are handled by CEGUI
#include "GL/glc.h"

#include "sdl_events.h"
#include "wmcontrol.h"

#define VM_2D 0
#define VM_3D 1

bool VIEW_MODE=VM_3D;

GLdouble eyeX=400;
GLdouble eyeY=900;
GLdouble eyeZ=-800;
GLdouble centerX=400;
GLdouble centerY=300;
GLdouble centerZ=0;
GLdouble upX=0;
GLdouble upY=-1;
GLdouble upZ=0;

struct navit *sdl_gui_navit;

#include <CEGUI/RendererModules/OpenGLGUIRenderer/openglrenderer.h>
#include "CEGUIDefaultResourceProvider.h"
CEGUI::OpenGLRenderer* renderer;

#undef profile
#define profile(x,y)

CEGUI::Window* myRoot;

#define MODULE "gui_sdl"
GLuint * DLid;

#define _(STRING)    gettext(STRING)

static int
gui_sdl_set_graphics(struct gui_priv *this_, struct graphics *gra)
{
	dbg(1,"setting up the graphics\n");

	DLid=(GLuint *)graphics_get_data(gra, "opengl_displaylist");
	if (!DLid) 
		return 1;
	return 0;
}


void drawCursor() {
	dbg(1,"Pushing a cursor from GUI\n");
                int x=400;
                int y=400;
                float cursor_size=15.0f;
                glColor4f(0.0f,0.0f,1.0f,0.75f);
                glEnable(GL_BLEND);
                glBegin(GL_TRIANGLES);
                        glVertex3f( x, y-cursor_size, 0.0f);
                        glVertex3f(x-cursor_size,y+cursor_size, 0.0f);
                        glVertex3f( x+cursor_size,y+cursor_size, 0.0f);
                glEnd();
                glDisable(GL_BLEND);
 }


static void
sdl_update_roadbook(struct navigation *nav)
{

	using namespace CEGUI;
	extern Window* myRoot;
	
	struct navigation_list *list;	
	list=navigation_list_new(nav);

	// First, update the 'Navigation Tip' on the main window

	try {
		struct attr attr;
		item_attr_get(navigation_list_get_item(list), attr_navigation_speech, &attr);
		WindowManager::getSingleton().getWindow("Navit/Routing/Tips")->setText(attr.u.str);
	}
	catch (CEGUI::Exception& e)
	{
		fprintf(stderr,"CEGUI Exception occured: \n%s\n", e.getMessage().c_str());
		printf("Missing control!...\n");
	}

	// Then, update the whole roadbook	
	try {

		/* Currently we use the 'Navit' text to display the roadbook, until Mineque design a button for that 		
		if(! WindowManager::getSingleton().getWindow("OSD/RoadbookButton")->isVisible()){
			WindowManager::getSingleton().getWindow("OSD/RoadbookButton")->show();
		}
		*/

		MultiColumnList* mcl = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("Roadbook"));
		mcl->resetList();

		item *item;
		struct attr attr;

		list=navigation_list_new(nav);	
		while ((item=navigation_list_get_item(list))) {
	 		mcl->addRow();
			item_attr_get(item, attr_navigation_short, &attr);
			ListboxTextItem* itemListbox = new ListboxTextItem(attr.u.str);
			itemListbox->setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush");
			mcl->setItem(itemListbox, 0, mcl->getRowCount()-1);
		}
		navigation_list_destroy(list);
	}
	catch (CEGUI::Exception& e)
	{
		dbg(0,"CEGUI Exception occured: \n%s\n", e.getMessage().c_str());
		dbg(0,"Missing control!\n");
	}

}

static int gui_run_main_loop(struct gui_priv *this_)
{

	using namespace CEGUI;
	dbg(1,"Entering main loop\n");

	bool must_quit = false;
	
	// get "run-time" in seconds
	double last_time_pulse = static_cast<double>(SDL_GetTicks());

	int frames=0;
	char fps [12];

	struct transformation *t;


	t=navit_get_trans(this_->nav);
	transform_set_size(t, 800, 600);	
	navit_draw(this_->nav);

	GLuint cursorDL;
	cursorDL=glGenLists(1);
	glNewList(cursorDL,GL_COMPILE);
		drawCursor();
	glEndList();

	bool enable_timer=0;

	struct navigation *navig;
	navig=navit_get_navigation(sdl_gui_navit);

	navigation_register_callback(navig,
		attr_navigation_long,
		callback_new_0((void (*)())sdl_update_roadbook)
	);


	while (!must_quit)
	{
		if(enable_timer)
 			profile(0,NULL);
		glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);

 		glMatrixMode(GL_MODELVIEW);
 		glLoadIdentity();

		if(VIEW_MODE==VM_3D){
 			gluLookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);
		}

 		// FIXME This is to draw a ground. This is ugly and need to be fixed.
		// Without it, we see the color of sky under the roads.
		glColor4f(0.0f,0.7f,0.35f,1.0f);
		glBegin(GL_POLYGON);
			glVertex3f( -800,-600*3, 0.0f);
			glVertex3f( -800,600*2, 0.0f);
			glVertex3f( 1600,600*2, 0.0f);	
			glVertex3f( 1600,-600*3, 0.0f);	
		glEnd();


 		if(enable_timer)
 			profile(0,"graphics_redraw");
 		if (!g_main_context_iteration (NULL, FALSE))
 			sleep(1);
 		if(enable_timer)
 			profile(0,"main context");

	//	graphics_get_data(this_->gra,DLid);
		
#if 0
		glCallList(*DLid);
#endif
		navit_draw_displaylist(sdl_gui_navit);

		// glCallList(cursorDL);
		inject_input(must_quit);
 		if(enable_timer)
 			profile(0,"inputs");

		// Render the cursor.
		int x=400;
		int y=480;
		float cursor_size=15.0f;
		glColor4f(0.0f,1.0f,0.0f,0.75f);
		glEnable(GL_BLEND);
		glBegin(GL_TRIANGLES);
			glVertex3f( x, y-cursor_size, 0.0f);
			glVertex3f(x-cursor_size,y+cursor_size, 0.0f);
			glVertex3f( x+cursor_size,y+cursor_size, 0.0f);	
		glEnd();
		glDisable(GL_BLEND);
 		if(enable_timer)
 			profile(0,"cursor");

		frames++;
		if(SDL_GetTicks()-last_time_pulse>1000){
			sprintf(fps,"%i fps",frames); // /(SDL_GetTicks()/1000));
			frames=0;
			last_time_pulse = SDL_GetTicks();
		}
 		WindowManager::getSingleton().getWindow("OSD/Satellites")->setText(fps);

 		if(enable_timer)
 			profile(0,"fps");

 		CEGUI::System::getSingleton().renderGUI();
 		if(enable_timer)
 			profile(0,"GUI");

		SDL_GL_SwapBuffers();
	}


}


static struct menu_priv *
gui_sdl_toolbar_new(struct gui_priv *this_, struct menu_methods *meth)
{
	return NULL; //gui_gtk_ui_new(this_, meth, "/ui/ToolBar", nav, 0);
}

static struct statusbar_priv *
gui_sdl_statusbar_new(struct gui_priv *gui, struct statusbar_methods *meth)
{
	return NULL; //gui_gtk_ui_new(this_, meth, "/ui/ToolBar", nav, 0);
}

static struct menu_priv *
gui_sdl_menubar_new(struct gui_priv *this_, struct menu_methods *meth)
{
	return NULL; //gui_gtk_ui_new(this_, meth, "/ui/MenuBar", nav, 0);
}

static struct menu_priv *
gui_sdl_popup_new(struct gui_priv *this_, struct menu_methods *meth)
{
	return NULL; //gui_gtk_ui_new(this_, meth, "/ui/PopUp", nav, 1);
}

struct gui_methods gui_sdl_methods = {
	gui_sdl_menubar_new,
	gui_sdl_toolbar_new,
	gui_sdl_statusbar_new,
	gui_sdl_popup_new,
	gui_sdl_set_graphics,
	gui_run_main_loop,
};


int init_GL() {

	dbg(1,"init_GL()\n");

	// Blue sky
 	glClearColor(0.3,0.7,1.0,0);

	if(VIEW_MODE==VM_2D){
		dbg(1,"Switching to 2D view\n");
// 		myRoot->getWindow("OSD/ViewMode")->setText("2D");
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
	
		glOrtho( 0, XRES, YRES, 0, -1, 1 );
	
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	} else {
		dbg(1,"Switching to 3D view\n");
// 		myRoot->getWindow("OSD/ViewMode")->setText("3D");

		// Dimensions de la fenetre de rendu 
		glViewport(0, 0, XRES, YRES);
		// Initialisation de la matrice de projection 
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45, 1.0, 0.1, 2800.0);
		// Rendu avec lissage de Gouraud 
// 		glShadeModel(GL_SMOOTH);
	// 	glEnable(GL_DEPTH_TEST);


 		glMatrixMode(GL_MODELVIEW);
   		glLoadIdentity();
// 		gluLookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);

	}

	//Display list code
	//	GLuint glGenLists(GLsizei numberOfIDsRequired);
	// linesDL = glGenLists(1);

	if( glGetError() != GL_NO_ERROR ) {
		return 0;
	}
	return 1;
}

bool ToggleView(const CEGUI::EventArgs& event)
{
	VIEW_MODE=!VIEW_MODE;

	if(VIEW_MODE==VM_2D){
 		CEGUI::WindowManager::getSingleton().getWindow("OSD/ViewMode")->setText("2D");
	} else {
 		CEGUI::WindowManager::getSingleton().getWindow("OSD/ViewMode")->setText("3D");
	}
	init_GL();
}

bool MoveCamera(const CEGUI::EventArgs& event){
	
	CEGUI::Scrollbar * sb = static_cast<const CEGUI::Scrollbar *>(CEGUI::WindowManager::getSingleton().getWindow("OSD/Scrollbar1"));
 	dbg(0,"moving : %f\n",sb->getScrollPosition());
	eyeZ=-sb->getScrollPosition();
	if (eyeZ>-100){
		eyeZ=-100;
	}
}

bool ShowKeyboard(const CEGUI::EventArgs& event){
	CEGUI::WindowManager::getSingleton().getWindow("Navit/Keyboard/Input")->setText("");
	CEGUI::WindowManager::getSingleton().getWindow("Navit/Keyboard")->show();
}

void Add_KeyBoard_key(CEGUI::String key,int x,int y,int w){
	
	using namespace CEGUI;
//	char button_name [5];
//	sprintf(button_name,"%s",key);
	FrameWindow* wnd = (FrameWindow*)WindowManager::getSingleton().createWindow("TaharezLook/Button", key);
	CEGUI::WindowManager::getSingleton().getWindow("Navit/Keyboard")->addChildWindow(wnd);
	wnd->setPosition(UVector2(cegui_absdim(x), cegui_absdim( y)));
	wnd->setSize(UVector2(cegui_absdim(w), cegui_absdim( 40)));
	wnd->setText(key);
	wnd->subscribeEvent(PushButton::EventClicked, Event::Subscriber(Handle_Virtual_Key_Down));
	
}	


void BuildKeyboard(){
	int w=55;
	int offset_x=10;
	int count_x=0;
	
	int y=25;
	Add_KeyBoard_key("A",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("Z",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("E",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("R",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("T",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("Y",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("U",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("I",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("O",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("P",offset_x+(count_x++)*w,y,w);
	count_x++;
	Add_KeyBoard_key("7",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("8",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("9",offset_x+(count_x++)*w,y,w);

	y=70;
	count_x=0;
	Add_KeyBoard_key("Q",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("S",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("D",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("F",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("G",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("H",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("J",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("K",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("L",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("M",offset_x+(count_x++)*w,y,w);
	count_x++;
	Add_KeyBoard_key("4",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("5",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("6",offset_x+(count_x++)*w,y,w);

	y=115;
	count_x=0;

	Add_KeyBoard_key("W",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("X",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("C",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("V",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("B",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("N",offset_x+(count_x++)*w,y,w);

	Add_KeyBoard_key(" ",offset_x+(count_x++)*w,y,w*2);
	count_x++;
	
	Add_KeyBoard_key("BACK",offset_x+(count_x++)*w,y,w*2);	
 	count_x+=2;
	
	Add_KeyBoard_key("1",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("2",offset_x+(count_x++)*w,y,w);
	Add_KeyBoard_key("3",offset_x+(count_x++)*w,y,w);
	
	y=160;
	count_x=11;
	Add_KeyBoard_key("0",offset_x+(count_x++)*w,y,w);
	
	Add_KeyBoard_key("OK",offset_x+(count_x++)*w,y,w*2);
	

}

static void init_sdlgui(char * skin_layout,int fullscreen)
{
	SDL_Surface * screen;
// 	atexit (SDL_Quit);
	SDL_Init (SDL_INIT_VIDEO);
	int videoFlags;
	const SDL_VideoInfo *videoInfo;
	videoInfo = SDL_GetVideoInfo( );

	if ( !videoInfo )
	{
	    fprintf( stderr, "Video query failed: %s\n",
		     SDL_GetError( ) );
	}

	/* the flags to pass to SDL_SetVideoMode */
	videoFlags  = SDL_OPENGL;          /* Enable OpenGL in SDL */
	videoFlags |= SDL_GL_DOUBLEBUFFER; /* Enable double buffering */
	videoFlags |= SDL_HWPALETTE;       /* Store the palette in hardware */
	videoFlags |= SDL_RESIZABLE;       /* Enable window resizing */
	
	/* This checks to see if surfaces can be stored in memory */
	if ( videoInfo->hw_available )
		videoFlags |= SDL_HWSURFACE;
	else
		videoFlags |= SDL_SWSURFACE;
	
	/* This checks if hardware blits can be done */
	if ( videoInfo->blit_hw )
		videoFlags |= SDL_HWACCEL;
	
	/* Sets up OpenGL double buffering */
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	SDL_WM_SetCaption("NavIt - The OpenSource vector based navigation engine", NULL);

	/* get a SDL surface */
	screen = SDL_SetVideoMode( XRES, YRES, 32,
					videoFlags );

	if (screen == NULL) {
		fprintf (stderr, "Can't set SDL: %s\n", SDL_GetError ());
		exit (1);
	}
	if(fullscreen){
		SDL_WM_ToggleFullScreen(screen);
	}
	SDL_ShowCursor (SDL_ENABLE);
	SDL_EnableUNICODE (1);
	SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	init_GL();
	
	try
	{
		renderer = new CEGUI::OpenGLRenderer(0,XRES,YRES);
		new CEGUI::System(renderer);

		using namespace CEGUI;

		SDL_ShowCursor(SDL_ENABLE);
		SDL_EnableUNICODE(1);
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
		
		CEGUI::DefaultResourceProvider* rp = static_cast<CEGUI::DefaultResourceProvider*>
		(System::getSingleton().getResourceProvider());
		

		// FIXME This should maybe move to navit.xml
		static char *datafiles_path[]={
			"./gui/sdl/datafiles",
			"/usr/share/navit/datafiles",
			"/usr/local/share/navit/datafiles",
			NULL,
		};

		char **filename=datafiles_path;

		while (*filename) {	
			if (FILE * file = fopen(*filename, "r"))
			{
				fclose(file);
				break;
			}
			filename++;
		}

		if(*filename==NULL){
			// FIXME Elaborate the possible solutions
			printf("Can't find the datafiles directory for CEGUI files. Navit will probably crash :)\n");
		} else {
			printf("Loading SDL datafiles from %s\n",*filename);
		}

		rp->setResourceGroupDirectory("schemes", g_strdup_printf("%s/schemes/",*filename));
		rp->setResourceGroupDirectory("imagesets", g_strdup_printf("%s/imagesets/",*filename));
		rp->setResourceGroupDirectory("fonts", g_strdup_printf("%s/fonts/",*filename));
		rp->setResourceGroupDirectory("layouts", g_strdup_printf("%s/layouts/",*filename));
		rp->setResourceGroupDirectory("looknfeels", g_strdup_printf("%s/looknfeel/",*filename));
		rp->setResourceGroupDirectory("lua_scripts", g_strdup_printf("%s/lua_scripts/",*filename));


		CEGUI::Imageset::setDefaultResourceGroup("imagesets");
		CEGUI::Font::setDefaultResourceGroup("fonts");
		CEGUI::Scheme::setDefaultResourceGroup("schemes");
		CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
		CEGUI::WindowManager::setDefaultResourceGroup("layouts");
		CEGUI::ScriptModule::setDefaultResourceGroup("lua_scripts");

		char buffer [50];
		sprintf (buffer, "%s.scheme", skin_layout);
		dbg(1,"Loading scheme : %s\n",buffer);

		CEGUI::SchemeManager::getSingleton().loadScheme(buffer);

		CEGUI::FontManager::getSingleton().createFont("DejaVuSans-10.font");
		CEGUI::FontManager::getSingleton().createFont("DejaVuSans-14.font");

		CEGUI::System::getSingleton().setDefaultFont("DejaVuSans-10");

		CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

		dbg(1,"Loading layout : %s\n",buffer);

		sprintf (buffer, "%s.layout", skin_layout);

		myRoot = CEGUI::WindowManager::getSingleton().loadWindowLayout(buffer);

 		CEGUI::System::getSingleton().setGUISheet(myRoot);

		try {

		CEGUI::WindowManager::getSingleton().getWindow("OSD/Quit")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ButtonQuit));
// 		CEGUI::WindowManager::getSingleton().getWindow("OSD/Quit")->setText(_("Quit"));

		CEGUI::WindowManager::getSingleton().getWindow("ZoomInButton")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ZoomIn));
// 		CEGUI::WindowManager::getSingleton().getWindow("ZoomInButton")->setText(_("ZoomIn"));

		CEGUI::WindowManager::getSingleton().getWindow("ZoomOutButton")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ZoomOut));
// 		CEGUI::WindowManager::getSingleton().getWindow("ZoomOutButton")->setText(_("ZoomOut"));

		CEGUI::WindowManager::getSingleton().getWindow("DestinationWindow/CountryEditbox")->subscribeEvent(Window::EventKeyUp, Event::Subscriber(DestinationEntryChange));
 		CEGUI::WindowManager::getSingleton().getWindow("DestinationWindow/CountryEditbox")->subscribeEvent(Window::EventMouseButtonDown, Event::Subscriber(handleMouseEnters));
		CEGUI::WindowManager::getSingleton().getWindow("DestinationWindow/TownEditbox")->subscribeEvent(Window::EventKeyUp, Event::Subscriber(DestinationEntryChange));
 		CEGUI::WindowManager::getSingleton().getWindow("DestinationWindow/TownEditbox")->subscribeEvent(Window::EventMouseButtonDown, Event::Subscriber(handleMouseEnters));
		CEGUI::WindowManager::getSingleton().getWindow("DestinationWindow/StreetEditbox")->subscribeEvent(Window::EventKeyUp, Event::Subscriber(DestinationEntryChange));
 		CEGUI::WindowManager::getSingleton().getWindow("DestinationWindow/StreetEditbox")->subscribeEvent(Window::EventMouseButtonDown, Event::Subscriber(handleMouseEnters));

		CEGUI::WindowManager::getSingleton().getWindow("DestinationButton")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(DialogWindowSwitch));

		CEGUI::WindowManager::getSingleton().getWindow("OSD/ViewMode")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ToggleView));

		CEGUI::WindowManager::getSingleton().getWindow("DestinationWindow/GO")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ButtonGo));
		CEGUI::WindowManager::getSingleton().getWindow("DestinationWindow/KB")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ShowKeyboard));

		CEGUI::WindowManager::getSingleton().getWindow("DestinationWindow/Listbox")->subscribeEvent(MultiColumnList::EventSelectionChanged, Event::Subscriber(ItemSelect));


		// Translation for StaticTexts (labels)
		CEGUI::WindowManager::getSingleton().getWindow("DestinationWindow/Country")->setText(_("Country"));
		CEGUI::WindowManager::getSingleton().getWindow("DestinationWindow/Town")->setText(_("City"));
		CEGUI::WindowManager::getSingleton().getWindow("DestinationWindow/Street")->setText(_("Street"));

 		MultiColumnList* mcl = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("DestinationWindow/Listbox"));

		mcl->setSelectionMode(MultiColumnList::RowSingle) ;
		mcl->addColumn("Value", 0, cegui_absdim(200.0));
		mcl->addColumn("ID", 1, cegui_absdim(70.0));
		mcl->addColumn("Assoc", 2, cegui_absdim(70.0));
		mcl->addColumn("x", 3, cegui_absdim(70.0));
		mcl->addColumn("y", 4, cegui_absdim(70.0));

 		MultiColumnList* mcl2 = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("Roadbook"));

		mcl2->setSelectionMode(MultiColumnList::RowSingle) ;
		mcl2->addColumn("Instructions", 0, cegui_absdim(700.0));

 		BuildKeyboard();

		CEGUI::WindowManager::getSingleton().getWindow("OSD/Scrollbar1")->subscribeEvent(Scrollbar::EventScrollPositionChanged, Event::Subscriber(MoveCamera));

		CEGUI::WindowManager::getSingleton().getWindow("OSD/RoadbookButton")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(RoadBookSwitch));
		CEGUI::WindowManager::getSingleton().getWindow("OSD/RoadbookButton")->setText(_("RoadBook"));

		CEGUI::WindowManager::getSingleton().getWindow("OSD/nGhostButton")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(Switch_to_nGhost));
		// this one is maybe not needed anymore
		CEGUI::WindowManager::getSingleton().getWindow("OSD/RoadbookButton2")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(RoadBookSwitch));

		}
		catch (CEGUI::Exception& e)
		{
			fprintf(stderr,"CEGUI Exception occured: \n%s\n", e.getMessage().c_str());
			printf("Missing control!...\n");
		}

	}
	catch (CEGUI::Exception& e)
	{
		fprintf(stderr,"CEGUI Exception occured: \n%s\n", e.getMessage().c_str());
		printf("quiting...\n");
		exit(1);
	}
	
}

static void vehicle_callback_handler( struct navit *nav, struct vehicle *v){
	char buffer [50];
 	double  speed=*vehicle_speed_get(v);
	sprintf (buffer, "%02.02f km/h", speed);
  	CEGUI::WindowManager::getSingleton().getWindow("OSD/SpeedoMeter")->setText(buffer);

 	double  height=*vehicle_height_get(v);
	sprintf (buffer, "%.0f m", height);
 	CEGUI::WindowManager::getSingleton().getWindow("OSD/Altimeter")->setText(buffer);

	int sats=*vehicle_sats_get(v);
	int sats_used=*vehicle_sats_used_get(v);
// 	printf(" sats : %i, used %i: \n",sats,sats_used);
	// Sat image hardcoded for now. may break the TaharezSkin
	// setProperty("Image", CEGUI::PropertyHelper::imageToString( yourImageSet->getImage( "yourImageName" ) ) );

	try {
		if(sats_used>1){
			CEGUI::WindowManager::getSingleton().getWindow("SateliteStrenghBar1")->setProperty("Image","set:Mineque-Black image:SateliteStrenghBarOn");
		} else {
			CEGUI::WindowManager::getSingleton().getWindow("SateliteStrenghBar1")->setProperty("Image","set:Mineque-Black image:SateliteStrenghBarOff");
		}
	
		if(sats_used>3){
			CEGUI::WindowManager::getSingleton().getWindow("SateliteStrenghBar2")->setProperty("Image","set:Mineque-Black image:SateliteStrenghBarOn");
		} else {
			CEGUI::WindowManager::getSingleton().getWindow("SateliteStrenghBar2")->setProperty("Image","set:Mineque-Black image:SateliteStrenghBarOff");
		}
	
		if(sats_used>5){
			CEGUI::WindowManager::getSingleton().getWindow("SateliteStrenghBar3")->setProperty("Image","set:Mineque-Black image:SateliteStrenghBarOn");
		} else {
			CEGUI::WindowManager::getSingleton().getWindow("SateliteStrenghBar3")->setProperty("Image","set:Mineque-Black image:SateliteStrenghBarOff");
		}
	
		if(sats_used>7){
			CEGUI::WindowManager::getSingleton().getWindow("SateliteStrenghBar4")->setProperty("Image","set:Mineque-Black image:SateliteStrenghBarOn");
		} else {
			CEGUI::WindowManager::getSingleton().getWindow("SateliteStrenghBar4")->setProperty("Image","set:Mineque-Black image:SateliteStrenghBarOff");
		}
	
		if(sats_used>8){
			CEGUI::WindowManager::getSingleton().getWindow("SateliteStrenghBar5")->setProperty("Image","set:Mineque-Black image:SateliteStrenghBarOn");
		} else {
			CEGUI::WindowManager::getSingleton().getWindow("SateliteStrenghBar5")->setProperty("Image","set:Mineque-Black image:SateliteStrenghBarOff");
		}
	}
	catch (CEGUI::Exception& e)
	{
		dbg(1,"Warning : you skin doesn't have the satellitebars. You should use Mineque's skin.");
	}

}

static struct gui_priv *
gui_sdl_new(struct navit *nav, struct gui_methods *meth, struct attr **attrs) 
{
	dbg(1,"Begin SDL init\n");
	struct gui_priv *this_;
	sdl_gui_navit=nav;
	
	if(sdl_gui_navit){	
		dbg(1,"VALID navit instance in gui\n");
	} else {
		dbg(1,"Invalid navit instance in gui\n");
	}
	if(nav){	
		dbg(1,"VALID source navit instance in gui\n");
	} else {
		dbg(1,"Invalid source navit instance in gui\n");
	}
	
	*meth=gui_sdl_methods;

	this_=g_new0(struct gui_priv, 1);
	int fullscreen=0;

	struct attr *fullscreen_setting=attr_search(attrs, NULL, attr_fullscreen);
	if(fullscreen_setting){
		fullscreen=1;
		printf("fullscreen\n");
	} else {
		fullscreen=0;
		printf("Normal screen\n");
	}

	struct attr *skin_setting=attr_search(attrs, NULL, attr_skin);
	if(skin_setting){
		init_sdlgui(skin_setting->u.str,fullscreen);
	} else {
		g_warning("Warning, no skin set for <sdl> in navit.xml. Using default one");
		init_sdlgui("TaharezLook",fullscreen);
	}

	dbg(1,"End SDL init\n");

	//gui_sdl_window.cpp:710: error: invalid conversion from 'void (*)(vehicle*)' to 'void (*)()'
	struct callback *cb=callback_new_0(callback_cast(vehicle_callback_handler));

	navit_add_vehicle_cb(nav,cb);
	this_->nav=nav;
	
	return this_;
}

void
plugin_init(void)
{
	dbg(1,"registering sdl plugin\n");
	plugin_register_gui_type("sdl", gui_sdl_new);
}

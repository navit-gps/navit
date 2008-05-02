#include "glib.h"
#include <stdio.h>
#include <libintl.h>

//  FIXME temporary fix for enum
#include "projection.h"

#include "item.h"
#include "navit.h"
#include "vehicle.h"	
#include "profile.h"
#include "transform.h"
#include "gui.h"
#include "coord.h"
#include "config.h"
#include "plugin.h"
#include "callback.h"
#include "point.h"
#include "graphics.h"
#include "gui_sdl.h"
#include "navigation.h"
#include "debug.h"
#include "attr.h"
#include "track.h"
#include "menu.h"
#include "map.h"


#include "CEGUI.h"

// FIXME This is for 3d fonts. Needs QuesoGLC. Could probably (and should) be moved to graphics instead
// since fonts here are handled by CEGUI
#include "GL/glc.h"

#include "sdl_events.h"
#include "cegui_keyboard.h"
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

// Temp fix for pluginless mode
// #define MODULE "gui_sdl"
GLuint * DLid;

#define _(STRING)    gettext(STRING)

char  media_window_title[255], media_cmd[255];

struct bookmark{
	char * name;
	struct callback *cb;
	struct bookmark *next;
} *bookmarks;

struct former_dest{
	char * name;
	struct callback *cb;
	struct former_dest *next;
} *former_dests;

static int
gui_sdl_set_graphics(struct gui_priv *this_, struct graphics *gra)
{
	dbg(1,"setting up the graphics\n");

	DLid=(GLuint *)graphics_get_data(gra, "opengl_displaylist");
	if (!DLid) 
		return 1;
	return 0;
}

static void
sdl_update_roadbook(struct navigation *nav)
{

	using namespace CEGUI;

	struct map *map;
	struct map_rect *mr;

	if (! nav)
		return;
	map=navigation_get_map(nav);
	if (! map)
		return;
	mr=map_rect_new(map, NULL);
	if (! mr)
		return;

	// First, ensure the navigation tip is visible. quick workaround for when resuming a destination
	WindowManager::getSingleton().getWindow("Navit/Routing/Tips")->show();

	// update the 'Navigation Tip' on the main window
	try {
		struct attr attr;
		item_attr_get(map_rect_get_item(mr), attr_navigation_speech, &attr);
		map_rect_destroy(mr);
		mr=map_rect_new(map, NULL);
		WindowManager::getSingleton().getWindow("Navit/Routing/Tips")->setText((CEGUI::utf8*)(attr.u.str));
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

		while ((item=map_rect_get_item(mr))) {
	 		mcl->addRow();
			item_attr_get(item, attr_navigation_short, &attr);
			ListboxTextItem* itemListbox = new ListboxTextItem(attr.u.str);
			itemListbox->setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush");
			mcl->setItem(itemListbox, 0, mcl->getRowCount()-1);
		}
		map_rect_destroy(mr);
	}
	catch (CEGUI::Exception& e)
	{
		dbg(0,"CEGUI Exception occured: \n%s\n", e.getMessage().c_str());
		dbg(0,"Missing control!\n");
	}

}

static void show_road_name(){
	struct tracking *tracking;
	struct attr road_name_attr;
	tracking=navit_get_tracking(sdl_gui_navit);

	using namespace CEGUI;


	if (tracking && tracking_get_current_attr(tracking, attr_label, &road_name_attr) ) {
		WindowManager::getSingleton().getWindow("Navit/Routing/CurrentRoadName")->setText((CEGUI::utf8*)(road_name_attr.u.str));
	}

}

static gboolean gui_timeout_cb(gpointer data)
{
	return TRUE;
}

static int gui_run_main_loop(struct gui_priv *this_)
{
	GSource *timeout;
	using namespace CEGUI;
	dbg(0,"Entering main loop\n");

	bool must_quit = false;

	// get "run-time" in seconds
	double last_time_pulse = static_cast<double>(SDL_GetTicks());

	int frames=0;
	char fps [12];

	struct map_selection sel,sel2;

	memset(&sel, 0, sizeof(sel));
	memset(&sel2, 0, sizeof(sel2));
	sel.u.c_rect.rl.x=800;
	sel.u.c_rect.rl.y=600;
#if 0
	sel.next=&sel2;
	sel2.u.c_rect.rl.x=-200;
	sel2.u.c_rect.rl.y=0;
	sel2.u.c_rect.lu.x=1000;
	sel2.u.c_rect.lu.y=-800;
	for (int i=0 ; i < layer_end ; i++) 
		sel2.order[i]=-4;
#endif
	
	transform_set_screen_selection(navit_get_trans(this_->nav), &sel);
	navit_draw(this_->nav);

	bool enable_timer=0;

	struct navigation *navig;
	navig=navit_get_navigation(sdl_gui_navit);

	navigation_register_callback(navig,
		attr_navigation_long,
		callback_new_0((void (*)())sdl_update_roadbook)
	);

	timeout = g_timeout_source_new(100);
	g_source_set_callback(timeout, gui_timeout_cb, NULL, NULL);
	g_source_attach(timeout, NULL);
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
// 		if (!g_main_context_iteration (NULL, FALSE))
 //			sleep(1);
 		g_main_context_iteration (NULL, TRUE);
 		//	sleep(1);
 		if(enable_timer)
 			profile(0,"main context");

		show_road_name();

		if (DLid && *DLid)
			glCallList(*DLid);
		else
			navit_draw_displaylist(sdl_gui_navit);

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
	g_source_destroy(timeout);

}

static struct menu_priv *
add_menu(struct menu_priv *menu, struct menu_methods *meth, char *name, enum menu_type type, struct callback *cb);

static struct menu_methods menu_methods = {
	add_menu,
};

struct menu_priv {
	char *path;	
// 	GtkAction *action;
	struct gui_priv *gui;
	enum menu_type type;
	struct callback *cb;
	struct menu_priv *child;
	struct menu_priv *sibling;
	gulong handler_id;
	guint merge_id;
};

#define MENU_BOOKMARK 2
#define MENU_FORMER_DEST 3

static struct menu_priv *
add_menu(struct menu_priv *menu, struct menu_methods *meth, char *name, enum menu_type type, struct callback *cb)
{
	using namespace CEGUI;
	*meth=menu_methods;
	dbg(0,"callback : %s\n",name);

	if(menu==(struct menu_priv *)(MENU_BOOKMARK)){
		dbg(0,"Item is a bookmark\n");
		MultiColumnList* mcl = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("Bookmarks/Listbox"));

		ListboxTextItem* itemListbox = new ListboxTextItem((CEGUI::utf8*)(name));
		itemListbox->setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush");
		mcl->addRow(itemListbox,0);

		struct bookmark *newB = g_new0(struct bookmark, 1);
		newB->name=g_strdup(name);
		newB->cb=cb;
		if (newB) {
			newB->next = bookmarks;
			bookmarks = newB;
		}

	}

	if(menu==(struct menu_priv *)(MENU_FORMER_DEST)){
		dbg(0,"Item is a former destination\n");
		MultiColumnList* mcl = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("FormerDests/Listbox"));

		ListboxTextItem* itemListbox = new ListboxTextItem((CEGUI::utf8*)(name));
		itemListbox->setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush");
		mcl->addRow(itemListbox,0);

		struct former_dest *newB = g_new0(struct former_dest, 1);
		newB->name=g_strdup(name);
		newB->cb=cb;
		if (newB) {
			newB->next = former_dests;
			former_dests = newB;
		}

	}

	if(!strcmp(name,"Bookmarks")){
		dbg(0,"Menu is the bookmark menu!\n");
		return (struct menu_priv *)MENU_BOOKMARK;
	} else if(!strcmp(name,"Former Destinations")){
		dbg(0,"Menu is the Former Destinations menu!\n");
		return (struct menu_priv *)MENU_FORMER_DEST;
	} else {
		return (struct menu_priv *)1;
	}
}

bool BookmarkGo(const char * name)
{
	dbg(0,"searching for bookmark %s\n",name);
	bookmark * bookmark_search=bookmarks;
	while ( bookmark_search ){
		dbg(0,"-> %s\n",bookmark_search->name);
		if(!strcmp(bookmark_search->name,name)){
			dbg(0,"Got it :)\n");
 			 callback_call_0(bookmark_search->cb);
		}
		bookmark_search=bookmark_search->next;
	}

}

bool FormerDestGo(const char * name)
{
	dbg(0,"searching for former_dest %s\n",name);
	former_dest * former_dest_search=former_dests;
	while ( former_dest_search ){
		dbg(0,"-> %s\n",former_dest_search->name);
		if(!strcmp(former_dest_search->name,name)){
			dbg(0,"Got it :)\n");
 			callback_call_0(former_dest_search->cb);
		}
		former_dest_search=former_dest_search->next;
	}

}

static struct menu_priv *
gui_sdl_menubar_new(struct gui_priv *this_, struct menu_methods *meth)
{
	*meth=menu_methods;
	return (struct menu_priv *) 1; //gui_gtk_ui_new(this_, meth, "/ui/MenuBar", nav, 0);
}

static struct menu_priv *
gui_sdl_popup_new(struct gui_priv *this_, struct menu_methods *meth)
{
	return NULL; //gui_gtk_ui_new(this_, meth, "/ui/PopUp", nav, 1);
}

struct gui_methods gui_sdl_methods = {
	gui_sdl_menubar_new,
	gui_sdl_popup_new,
	gui_sdl_set_graphics,
	gui_run_main_loop,
};


int init_GL() {

	// Blue sky
 	glClearColor(0.3,0.7,1.0,0);

	if(VIEW_MODE==VM_2D){
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
	
		glOrtho( 0, XRES, YRES, 0, -1, 1 );
	
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		CEGUI::WindowManager::getSingleton().getWindow("OSD/ViewMode")->setText("2D");
	} else {

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
		CEGUI::WindowManager::getSingleton().getWindow("OSD/ViewMode")->setText("3D");
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
	init_GL();
}

bool MoveCamera(const CEGUI::EventArgs& event){
	
	CEGUI::Scrollbar * sb = static_cast<const CEGUI::Scrollbar *>(CEGUI::WindowManager::getSingleton().getWindow("OSD/Scrollbar1"));
	eyeZ=-sb->getScrollPosition();
	if (eyeZ>-100){
		eyeZ=-100;
	}
}



static void init_sdlgui(char * skin_layout,int fullscreen,int tilt, char *image_codec_name)
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

// 	init_GL();
	
	try
	{
		using namespace CEGUI;
		if (image_codec_name) {
			dbg(0, "Using image codec: %s from config\n", image_codec_name);
		} else {
#if defined (HAVE_LIBCEGUISILLYIMAGECODEC)
			image_codec_name = "SILLYImageCodec";
#elif defined(HAVE_LIBCEGUIDEVILIMAGECODEC)
			image_codec_name = "DevILImageCodec";
#elif defined (HAVE_LIBCEGUITGAIMAGECODEC)
			image_codec_name = "TGAImageCodec";
#else
			fprintf (stderr, "No default image codec available. Try setting image_codec in your config\n");
			exit (1);
#endif
			dbg(0, "Using default image codec: %s\n", image_codec_name);
		}
		CEGUI::OpenGLRenderer::setDefaultImageCodecName(image_codec_name);

		CEGUI::System::setDefaultXMLParserName(CEGUI::String("TinyXMLParser"));
		dbg(0, "Using %s as the default CEGUI XML Parser\n", CEGUI::System::getDefaultXMLParserName().c_str());
		renderer = new CEGUI::OpenGLRenderer(0,XRES,YRES);
		new CEGUI::System(renderer);
		
		SDL_ShowCursor(SDL_ENABLE);
		SDL_EnableUNICODE(1);
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
		
		CEGUI::DefaultResourceProvider* rp = static_cast<CEGUI::DefaultResourceProvider*>
		(System::getSingleton().getResourceProvider());
		

		char *filename;

		filename=g_strdup_printf("%s/share/navit/datafiles", getenv("NAVIT_PREFIX"));

		if (FILE * file = fopen(filename, "r")){
			fclose(file);
			dbg(0,"Ressources can be loaded from %s\n",filename);
		} else {
			filename=g_strdup_printf("./gui/sdl/datafiles");
			dbg(0,"Failling back to %s",filename);
		}

		dbg(0,"Loading SDL datafiles from %s\n",filename);

		rp->setResourceGroupDirectory("schemes", g_strdup_printf("%s/schemes/",filename));
		rp->setResourceGroupDirectory("imagesets", g_strdup_printf("%s/imagesets/",filename));
		rp->setResourceGroupDirectory("fonts", g_strdup_printf("%s/fonts/",filename));
		rp->setResourceGroupDirectory("layouts", g_strdup_printf("%s/layouts/",filename));
		rp->setResourceGroupDirectory("looknfeels", g_strdup_printf("%s/looknfeel/",filename));
		rp->setResourceGroupDirectory("lua_scripts", g_strdup_printf("%s/lua_scripts/",filename));
		g_free(filename);


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
		CEGUI::FontManager::getSingleton().createFont("DejaVuSans-12.font");
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

		CEGUI::WindowManager::getSingleton().getWindow("AdressSearch/CountryEditbox")->subscribeEvent(Window::EventKeyUp, Event::Subscriber(DestinationEntryChange));
 		CEGUI::WindowManager::getSingleton().getWindow("AdressSearch/CountryEditbox")->subscribeEvent(Window::EventMouseButtonDown, Event::Subscriber(handleMouseEnters));
		CEGUI::WindowManager::getSingleton().getWindow("AdressSearch/TownEditbox")->subscribeEvent(Window::EventKeyUp, Event::Subscriber(DestinationEntryChange));
 		CEGUI::WindowManager::getSingleton().getWindow("AdressSearch/TownEditbox")->subscribeEvent(Window::EventMouseButtonDown, Event::Subscriber(handleMouseEnters));
		CEGUI::WindowManager::getSingleton().getWindow("AdressSearch/StreetEditbox")->subscribeEvent(Window::EventKeyUp, Event::Subscriber(DestinationEntryChange));
 		CEGUI::WindowManager::getSingleton().getWindow("AdressSearch/StreetEditbox")->subscribeEvent(Window::EventMouseButtonDown, Event::Subscriber(handleMouseEnters));

		CEGUI::WindowManager::getSingleton().getWindow("DestinationButton")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(DestinationWindowSwitch));
		CEGUI::WindowManager::getSingleton().getWindow("DestinationWindow/Address")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(AddressSearchSwitch));
		CEGUI::WindowManager::getSingleton().getWindow("DestinationWindow/Bookmark")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(BookmarkSelectionSwitch));
		CEGUI::WindowManager::getSingleton().getWindow("DestinationWindow/FormerDest")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(FormerDestSelectionSwitch));


		CEGUI::WindowManager::getSingleton().getWindow("OSD/ViewMode")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ToggleView));

		CEGUI::WindowManager::getSingleton().getWindow("AdressSearch/GO")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ButtonGo));
		CEGUI::WindowManager::getSingleton().getWindow("AdressSearch/KB")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ShowKeyboard));

		CEGUI::WindowManager::getSingleton().getWindow("AdressSearch/Listbox")->subscribeEvent(MultiColumnList::EventSelectionChanged, Event::Subscriber(ItemSelect));
		CEGUI::WindowManager::getSingleton().getWindow("Bookmarks/Listbox")->subscribeEvent(MultiColumnList::EventSelectionChanged, Event::Subscriber(BookmarkSelect));
		CEGUI::WindowManager::getSingleton().getWindow("FormerDests/Listbox")->subscribeEvent(MultiColumnList::EventSelectionChanged, Event::Subscriber(FormerDestSelect));


		// Translation for StaticTexts (labels)
		CEGUI::WindowManager::getSingleton().getWindow("AdressSearch/Country")->setText(_("Country"));
		CEGUI::WindowManager::getSingleton().getWindow("AdressSearch/Town")->setText(_("City"));
		CEGUI::WindowManager::getSingleton().getWindow("AdressSearch/Street")->setText(_("Street"));


 		MultiColumnList* mcl = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("AdressSearch/Listbox"));

		mcl->setSelectionMode(MultiColumnList::RowSingle) ;
		mcl->addColumn("Value", 0, cegui_absdim(200.0));
		mcl->addColumn("ID", 1, cegui_absdim(70.0));
		mcl->addColumn("Assoc", 2, cegui_absdim(70.0));
		mcl->addColumn("x", 3, cegui_absdim(70.0));
		mcl->addColumn("y", 4, cegui_absdim(70.0));

 		MultiColumnList* mcl2 = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("Roadbook"));

		mcl2->setSelectionMode(MultiColumnList::RowSingle) ;
		mcl2->addColumn("Instructions", 0, cegui_absdim(700.0));

 		MultiColumnList* mcl3 = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("Bookmarks/Listbox"));

		mcl3->setSelectionMode(MultiColumnList::RowSingle) ;
		mcl3->addColumn("Name", 0, cegui_absdim(700.0));

 		MultiColumnList* mcl4 = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("FormerDests/Listbox"));

		mcl4->setSelectionMode(MultiColumnList::RowSingle) ;
		mcl4->addColumn("Name", 0, cegui_absdim(700.0));

 		BuildKeyboard();

		CEGUI::WindowManager::getSingleton().getWindow("OSD/Scrollbar1")->subscribeEvent(Scrollbar::EventScrollPositionChanged, Event::Subscriber(MoveCamera));

		// FIXME : char (conf) -> int (init) -> char (property) = bad
		char buffer[4];
		sprintf (buffer,"%i",tilt);
		CEGUI::WindowManager::getSingleton().getWindow("OSD/Scrollbar1")->setProperty("ScrollPosition",buffer);
		eyeZ=-tilt;

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
	init_GL();
	// Force centering view on cursor
// 	navit_toggle_cursor(gui->nav);
	// Force refresh on gps update
// 	navit_toggle_tracking(gui->nav);
	
}

static void vehicle_callback_handler( struct navit *nav, struct vehicle *v){
	char buffer [50];
	struct attr attr;
	int sats=0, sats_used=0;

	if (vehicle_get_attr(v, attr_position_speed, &attr))
		sprintf (buffer, "%02.02f km/h", *attr.u.numd);
	else
		strcpy (buffer, "N/A");
  	CEGUI::WindowManager::getSingleton().getWindow("OSD/SpeedoMeter")->setText(buffer);

	if (vehicle_get_attr(v, attr_position_height, &attr))
		sprintf (buffer, "%.f m", *attr.u.numd);
	else
		strcpy (buffer, "N/A");
 	CEGUI::WindowManager::getSingleton().getWindow("OSD/Altimeter")->setText(buffer);

	if (vehicle_get_attr(v, attr_position_sats, &attr))
		sats=attr.u.num;
	if (vehicle_get_attr(v, attr_position_sats_used, &attr))
		sats_used=attr.u.num;
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
		dbg(1,"Warning : you skin doesn't have the satellitebars. You should use Mineque's skin.\n");
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
	//FIXME currently, we only check if fullscreen is declared, but not its value
	if(fullscreen_setting){
		fullscreen=1;
		printf("fullscreen\n");
	} else {
		fullscreen=0;
		printf("Normal screen\n");
	}

	int tilt=400;
	struct attr *tilt_setting=attr_search(attrs, NULL, attr_tilt);
	if(tilt_setting){
		if(sscanf(tilt_setting->u.str,"%i",&tilt)){
			dbg(0,"tilt set to %i\n",tilt);
		} else {
			dbg(0,"title was not recognized : %s\n",tilt_setting->u.str);
		}
	} else {
		dbg(0,"tilt is not set\n");
	}
	
	struct attr *view_mode_setting=attr_search(attrs, NULL, attr_view_mode);
	if(view_mode_setting){
		if(!strcmp(view_mode_setting->u.str,"2D")){
			dbg(0,"View mode is 2D\n");
			VIEW_MODE=VM_2D;
		} else {
			dbg(0,"view mode is something else : %s\n",view_mode_setting->u.str);
		}
		
	} else {
		dbg(0,"view_mode is not set\n");
	}

	struct attr *media_cmd_setting=attr_search(attrs, NULL, attr_media_cmd);
	if(media_cmd_setting){
		dbg(0,"setting media_cmd to %s\n",media_cmd_setting->u.str);
		strcpy(media_cmd,media_cmd_setting->u.str);
	} else {
//		strcpy(media_cmd_setting->u.str,media_window_title);
	}

	struct attr *media_window_title_setting=attr_search(attrs, NULL, attr_media_window_title);
	if(media_window_title_setting){
		strcpy(media_window_title,media_window_title_setting->u.str);
	} else {
//		strcpy(media_cmd_setting->u.str,media_window_title);
	}

	struct attr *image_codec_setting=attr_search(attrs, NULL, attr_image_codec);
	char *image_codec_name=NULL;
	if (image_codec_setting)
		image_codec_name=image_codec_setting->u.str;
	struct attr *skin_setting=attr_search(attrs, NULL, attr_skin);
	if(skin_setting){
		init_sdlgui(skin_setting->u.str,fullscreen,tilt, image_codec_name);
	} else {
		g_warning("Warning, no skin set for <sdl> in navit.xml. Using default one");
		init_sdlgui("TaharezLook",fullscreen,tilt, image_codec_name);
	}
	

	dbg(1,"End SDL init\n");

	//gui_sdl_window.cpp:710: error: invalid conversion from 'void (*)(vehicle*)' to 'void (*)()'
	
	/* add callback for position updates */
	struct callback *cb=callback_new_attr_0(callback_cast(vehicle_callback_handler), attr_position_coord_geo);

	navit_add_callback(nav,cb);
	this_->nav=nav;
	
	return this_;
}

void
plugin_init(void)
{
	dbg(1,"registering sdl plugin\n");
	plugin_register_gui_type("sdl", gui_sdl_new);
}

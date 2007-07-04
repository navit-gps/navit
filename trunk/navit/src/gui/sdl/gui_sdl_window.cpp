#include "glib.h"
#include <stdio.h>
// #include <gtk/gtk.h>

//  FIXME temporary fix for enum
#include "projection.h"


#include "navit.h"
#include "profile.h"
#include "transform.h"
#include "gui.h"
#include "coord.h"
#include "plugin.h"
#include "graphics.h"
#include "gui_sdl.h"

#include "navigation.h"

#include "CEGUI.h"


// This is for 3d fonts
#include "GL/glc.h"


#include "sdl_events.h"

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

#include <CEGUI/RendererModules/OpenGLGUIRenderer/openglrenderer.h>
#include "CEGUIDefaultResourceProvider.h"
CEGUI::OpenGLRenderer* renderer;

#undef profile
#define profile(x,y)

CEGUI::Window* myRoot;

#define MODULE "gui_sdl"
GLuint * DLid;

struct navit *sdl_gui_navit;

static int
gui_sdl_set_graphics(struct gui_priv *this_, struct graphics *gra)
{
	printf("setting up the graphics\n");

	DLid=(GLuint *)graphics_get_data(gra, "opengl_displaylist");
	if (!DLid) 
		return 1;
	return 0;
}


void drawCursor() {
	printf("Pushing a cursor from GUI\n");
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
sdl_update_roadbook(struct navigation *nav, void *data)
{
	using namespace CEGUI;
	extern Window* myRoot;

	if(! WindowManager::getSingleton().getWindow("OSD/RoadbookButton")->isVisible()){
		WindowManager::getSingleton().getWindow("OSD/RoadbookButton")->show();
	}

	MultiColumnList* mcl = static_cast<MultiColumnList*>(myRoot->getChild("Navit/RoadBook")->getChild("Roadbook"));
	mcl->resetList();


	struct navigation_list *list;
	char *str;
	
	list=navigation_list_new(nav);
	while ((str=navigation_list_get(list, navigation_mode_short))) {
	
// 		printf("SDL : %s\n", str);

		
		mcl->addRow();
		/*
		char from [256];
		char to [256];
	
		sprintf(from,"%s %s",param[0].value,param[1].value);
		ListboxTextItem* itemListbox = new ListboxTextItem(from);
		sprintf(to,"%s %s",param[2].value,param[3].value);

		itemListbox = new ListboxTextItem(to);
		itemListbox->setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush");
		mcl->setItem(itemListbox, 1, mcl->getRowCount()-1);
	
		itemListbox = new ListboxTextItem(param[9].value);
		itemListbox->setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush");
		mcl->setItem(itemListbox, 2, mcl->getRowCount()-1);
	
		itemListbox = new ListboxTextItem(param[11].value);
		itemListbox->setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush");
		mcl->setItem(itemListbox, 3, mcl->getRowCount()-1);
	
		itemListbox = new ListboxTextItem(param[10].value);
		itemListbox->setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush");
		mcl->setItem(itemListbox, 4, mcl->getRowCount()-1);

		*/
		ListboxTextItem* itemListbox = new ListboxTextItem(str);
		itemListbox->setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush");
		mcl->setItem(itemListbox, 0, mcl->getRowCount()-1);

	}
	navigation_list_destroy(list);
}

static int gui_run_main_loop(struct gui_priv *this_)
{
	printf("Entering main loop\n");

	bool must_quit = false;
	
	// get "run-time" in seconds
	double last_time_pulse = static_cast<double>(SDL_GetTicks());

	int frames=0;
	char fps [12];

	struct transformation *t;


	t=navit_get_trans(this_->nav);
	transform_set_size(t, 800, 600);	
	navit_draw(this_->nav);
/*
        glNewList(DLid,GL_COMPILE);
                int x=400;
                int y=400;
                float cursor_size=15.0f;
                glColor4f(1.0f,0.0f,0.0f,0.75f);
                glEnable(GL_BLEND);
                glBegin(GL_TRIANGLES);
                        glVertex3f( x, y-cursor_size, 0.0f);
                        glVertex3f(x-cursor_size,y+cursor_size, 0.0f);
                        glVertex3f( x+cursor_size,y+cursor_size, 0.0f);
                glEnd();
                glDisable(GL_BLEND);
        glEndList();
*/
	GLuint cursorDL;
	cursorDL=glGenLists(1);
	glNewList(cursorDL,GL_COMPILE);
		drawCursor();
	glEndList();

	bool enable_timer=0;

	struct navigation *navig;
	navig=navit_get_navigation(sdl_gui_navit);
	if(navig){
		printf("navig valid, registering callback\n");
		navigation_register_callback(navig, navigation_mode_long, sdl_update_roadbook, sdl_gui_navit);
	} else {
		printf("navig unvalid\n");
	}


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
  		
/*
 		// FIXME This is to draw a ground. This is ugly and need to be fixed. Without it, we see the color of sky under the roads.
		glColor4f(0.0f,0.7f,0.35f,1.0f);
		glBegin(GL_POLYGON);
			glVertex3f( -800,-600*3, 0.0f);
			glVertex3f( -800,600*2, 0.0f);
			glVertex3f( 1600,600*2, 0.0f);	
			glVertex3f( 1600,-600*3, 0.0f);	
		glEnd();
*/

 		if(enable_timer)
 			profile(0,"graphics_redraw");
 		g_main_context_iteration (NULL, FALSE);
// 		profile_timer("main context");

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
			sprintf(fps,"%i",frames); // /(SDL_GetTicks()/1000));
			frames=0;
			last_time_pulse = SDL_GetTicks();
		}

		myRoot->getChild("OSD/SpeedoMeter")->setText(fps);

		/*
		glcRenderStyle(GLC_TEXTURE);
		glColor3f(1, 0, 0);
		glRotatef(180,1,0,0);
		glScalef(64, 64, 0);
		glcRenderString(fps);
		*/

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

	printf("init_GL()\n");
//  	glClearColor(1.0,0.9,0.7,0);

	// Blue sky
 	glClearColor(0.3,0.7,1.0,0);

	if(VIEW_MODE==VM_2D){
		printf("Switching to 2D view\n");
// 		myRoot->getChild("OSD/ViewMode")->setText("2D");
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
	
		glOrtho( 0, XRES, YRES, 0, -1, 1 );
	
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	} else {
		printf("Switching to 3D view\n");
// 		myRoot->getChild("OSD/ViewMode")->setText("3D");

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
 		myRoot->getChild("OSD/ViewMode")->setText("2D");
	} else {
 		myRoot->getChild("OSD/ViewMode")->setText("3D");
	}
	init_GL();
}

bool MoveCamera(const CEGUI::EventArgs& event){
	
	CEGUI::Scrollbar * sb = static_cast<const CEGUI::Scrollbar *>(myRoot->getChild("OSD/Scrollbar1"));
// 	printf("moving : %f\n",sb->getScrollPosition());
	eyeZ=-sb->getScrollPosition();
	if (eyeZ>-100){
		eyeZ=-100;
	}
}

bool ShowKeyboard(const CEGUI::EventArgs& event){
	myRoot->getChild("Navit/Keyboard")->getChild("Navit/Keyboard/Input")->setText("");
	myRoot->getChild("Navit/Keyboard")->show();
}

void Add_KeyBoard_key(char * key,int x,int y,int w){
	
	using namespace CEGUI;
	char button_name [5];
	sprintf(button_name,"%s",key);
	FrameWindow* wnd = (FrameWindow*)WindowManager::getSingleton().createWindow("TaharezLook/Button", button_name);
	myRoot->getChild("Navit/Keyboard")->addChildWindow(wnd);
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

static void init_sdlgui(void)
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

	SDL_ShowCursor (SDL_ENABLE);
	SDL_EnableUNICODE (1);
	SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	init_GL();
	
// 	sdl_audio_init();

	try
	{
		renderer = new CEGUI::OpenGLRenderer(0,XRES,YRES);
		new CEGUI::System(renderer);

		using namespace CEGUI;

		SDL_ShowCursor(SDL_ENABLE);
		SDL_EnableUNICODE(1);
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
		
		CEGUI::DefaultResourceProvider* rp = static_cast<CEGUI::DefaultResourceProvider*>
		(CEGUI::System::getSingleton().getResourceProvider());
		
		rp->setResourceGroupDirectory("schemes", "../datafiles/schemes/");
		rp->setResourceGroupDirectory("imagesets", "../datafiles/imagesets/");
		rp->setResourceGroupDirectory("fonts", "../datafiles/fonts/");
		rp->setResourceGroupDirectory("layouts", "../datafiles/layouts/");
		rp->setResourceGroupDirectory("looknfeels", "../datafiles/looknfeel/");
		rp->setResourceGroupDirectory("lua_scripts", "../datafiles/lua_scripts/");


		CEGUI::Imageset::setDefaultResourceGroup("imagesets");
		CEGUI::Font::setDefaultResourceGroup("fonts");
		CEGUI::Scheme::setDefaultResourceGroup("schemes");
		CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
		CEGUI::WindowManager::setDefaultResourceGroup("layouts");
		CEGUI::ScriptModule::setDefaultResourceGroup("lua_scripts");

		CEGUI::SchemeManager::getSingleton().loadScheme("TaharezLook.scheme");

		CEGUI::FontManager::getSingleton().createFont("DejaVuSans-10.font");
		CEGUI::FontManager::getSingleton().createFont("DejaVuSans-14.font");

		CEGUI::System::getSingleton().setDefaultFont("DejaVuSans-10");

		CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

 		myRoot = CEGUI::WindowManager::getSingleton().loadWindowLayout("navit.layout");

 		CEGUI::System::getSingleton().setGUISheet(myRoot);

	
		myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/CountryEditbox")->subscribeEvent(Window::EventKeyUp, Event::Subscriber(DestinationEntryChange));
 		myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/CountryEditbox")->subscribeEvent(Window::EventMouseButtonDown, Event::Subscriber(handleMouseEnters));
		myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/TownEditbox")->subscribeEvent(Window::EventKeyUp, Event::Subscriber(DestinationEntryChange));
 		myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/TownEditbox")->subscribeEvent(Window::EventMouseButtonDown, Event::Subscriber(handleMouseEnters));
		myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/StreetEditbox")->subscribeEvent(Window::EventKeyUp, Event::Subscriber(DestinationEntryChange));
 		myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/StreetEditbox")->subscribeEvent(Window::EventMouseButtonDown, Event::Subscriber(handleMouseEnters));

		myRoot->getChild("OSD/DestinationButton")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(DialogWindowSwitch));

		myRoot->getChild("OSD/RoadbookButton")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(RoadBookSwitch));
		myRoot->getChild("Navit/RoadBook")->getChild("OSD/RoadbookButton2")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(RoadBookSwitch));
		myRoot->getChild("OSD/ZoomIn")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ZoomIn));
		myRoot->getChild("OSD/ZoomOut")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ZoomOut));
		myRoot->getChild("OSD/Quit")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ButtonQuit));
		myRoot->getChild("OSD/ViewMode")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ToggleView));

		myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/GO")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ButtonGo));
		myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/KB")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ShowKeyboard));

		myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/Listbox")->subscribeEvent(MultiColumnList::EventSelectionChanged, Event::Subscriber(ItemSelect));

		myRoot->getChild("OSD/Scrollbar1")->subscribeEvent(Scrollbar::EventScrollPositionChanged, Event::Subscriber(MoveCamera));

		
 		MultiColumnList* mcl = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("DestinationWindow/Listbox"));

		mcl->setSelectionMode(MultiColumnList::RowSingle) ;
		mcl->addColumn("Value", 0, cegui_absdim(200.0));
		mcl->addColumn("ID", 1, cegui_absdim(70.0));
		mcl->addColumn("Assoc", 2, cegui_absdim(70.0));
		mcl->addColumn("x", 3, cegui_absdim(70.0));
		mcl->addColumn("y", 4, cegui_absdim(70.0));

 		MultiColumnList* mcl2 = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("Roadbook"));

		mcl2->setSelectionMode(MultiColumnList::RowSingle) ;
		mcl2->addColumn("From", 0, cegui_absdim(200.0));
		mcl2->addColumn("To", 1, cegui_absdim(200.0));
		mcl2->addColumn("Dist", 2, cegui_absdim(80.0));
		mcl2->addColumn("ETA", 3, cegui_absdim(80.0));
		mcl2->addColumn("Instruction",4, cegui_absdim(300.0));

 		BuildKeyboard();
		
	}
	catch (CEGUI::Exception& e)
	{
		fprintf(stderr,"CEGUI Exception occured: \n%s\n", e.getMessage().c_str());
		printf("quiting...\n");
		exit(1);
	}

	char * fontname="/usr/share/fonts/corefonts/verdana.ttf";

// 	printf("Gui initialized. Building fonts\n");


	int ctx = 0;
	int font = 0;
	ctx = glcGenContext();
	glcContext(ctx);
	font = glcNewFontFromFamily(glcGenFontID(), "Arial");
	glcFont(font);
// 	glcFontFace(font, "Italic");

// 	printf("Fonts built. Ready to rock!\n");

	
}

static struct gui_priv *
gui_sdl_new(struct navit *nav, struct gui_methods *meth, int w, int h) 
{
	printf("Begin SDL init\n");
	struct gui_priv *this_;
	sdl_gui_navit=nav;
	
	if(sdl_gui_navit){	
		printf("*** VALID navit instance in gui\n");
	} else {
		printf("*** Invalid navit instance in gui\n");
	}
	if(nav){	
		printf("*** VALID source navit instance in gui\n");
	} else {
		printf("*** Invalid source navit instance in gui\n");
	}
	
	*meth=gui_sdl_methods;

	this_=g_new0(struct gui_priv, 1);
	init_sdlgui();
	printf("End SDL init\n");

	/*
 	this_->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
 	this_->vbox = gtk_vbox_new(FALSE, 0);
	gtk_window_set_default_size(GTK_WINDOW(this_->win), w, h);
	gtk_window_set_title(GTK_WINDOW(this_->win), "Navit");
	gtk_widget_realize(this_->win);
	gtk_container_add(GTK_CONTAINER(this_->win), this_->vbox);
	gtk_widget_show_all(this_->win);
	*/
	this_->nav=nav;
	

	return this_;
}

void
plugin_init(void)
{
	printf("registering sdl plugin\n");
	plugin_register_gui_type("sdl", gui_sdl_new);
}

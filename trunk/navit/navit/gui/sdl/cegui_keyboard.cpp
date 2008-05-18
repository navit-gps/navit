#include "CEGUI.h"
#include "sdl_events.h"


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



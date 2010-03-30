
import Qt 4.6

Rectangle {
    id: page    

    function pageOpen(command) {
        if ( command=="menu") {
		gui.returnSource="";
		gui.setPage("main.qml");
	}
	if (command=="fullscreen") {
		if (gui.getAttr("fullscreen") == "1") {
			gui.setAttr("fullscreen",0)
		} else {
			gui.setAttr("fullscreen",1)
		}
		gui.backToMap();
	}
	if (command=="quit") {
		navit.quit();
	}
	
    }
    
    Component.onCompleted: pageOpen(gui.commandFunction);    
    
	Text { id: myText; anchors.centerIn: parent; text: "Hi, i'm Navit!"; color: "Black"; font.pointSize: gui.height/32 }    
}

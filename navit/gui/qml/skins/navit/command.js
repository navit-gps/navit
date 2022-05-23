import Qt 4.7

Rectangle {
    id: page

    function pageOpen(command) {
        if ( command=="menu") {
		gui.returnSource="";
		Loader {source: "main.qml"};
	}
	if (command=="quit") {
		navit.quit();
	}

    }

    Component.onCompleted: pageOpen(gui.commandFunction);

	Text { id: myText; anchors.centerIn: parent; text: "Hi, i'm Navit!"; color: "Black"; font.pointSize: gui.height/32 }
}

import Qt 4.6

Rectangle {

    function onStartup() {    
       	if ( gui.returnSource.split('/').length > 2  ) {
		btnBack.opacity=1;
	}
	if ( gui.returnSource == "/main.qml" ) {
		btnQuit.opacity=1;
	}
    }

   Component.onCompleted: onStartup();

    ButtonIcon {
        id: btnMap; icon: "gui_map.svg"; text:"Map"; onClicked: gui.backToMap();
        anchors.left: parent.left; anchors.leftMargin: 3
        anchors.bottom: parent.bottom; anchors.bottomMargin: 3
    }

    ButtonIcon {
        id: btnBack; icon: "gui_arrow_left.svg"; text: "Back"; onClicked: gui.backToPrevPage();
        anchors.right: parent.right; anchors.leftMargin: 3
        anchors.bottom: parent.bottom; anchors.bottomMargin: 3
	opacity: 0;
    }

    ButtonIcon {
        id: btnQuit; icon: "gui_quit.svg"; text: "Quit"; onClicked: navit.quit()
        anchors.right: parent.right; anchors.leftMargin: 3
        anchors.bottom: parent.bottom; anchors.bottomMargin: 3
	opacity: 0;
    }

}

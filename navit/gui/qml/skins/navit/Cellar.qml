import Qt 4.6

Rectangle {

    function onStartup() {        
       	if ( gui.returnSource.split('/').length > 2  ) {
		btnBack.opacity=1;
	}
	if ( gui.returnSource != "/point.qml" ) {
		btnPoint.opacity=1
	}
	if ( gui.returnSource != "/main.qml" ) {
		btnMenu.opacity=1;
	}
    }

   Component.onCompleted: onStartup();

    ButtonIcon {
        id: btnMap; icon: "gui_map.svg"; text:"Map"; onClicked: gui.backToMap();
        anchors.left: parent.left; anchors.leftMargin: 3
        anchors.bottom: parent.bottom; anchors.bottomMargin: 3
    }

    ButtonIcon {
        id: btnPoint; icon: "gui_arrow_up.svg"; text: "Point"; onClicked: { gui.returnSource=""; gui.setPage("point.qml"); }
        anchors.left: btnMap.left; anchors.leftMargin: parent.width/3;
        anchors.bottom: parent.bottom; anchors.bottomMargin: 3
  	opacity: 0;
    }

    ButtonIcon {
        id: btnMenu; icon: "gui_menu.svg"; text: "Menu"; onClicked: { gui.returnSource=""; gui.setPage("main.qml"); }
        anchors.right: btnBack.right; anchors.rightMargin: parent.width/3;
        anchors.bottom: parent.bottom; anchors.bottomMargin: 3
	opacity: 0;
    }

    ButtonIcon {
        id: btnBack; icon: "gui_arrow_left.svg"; text: "Back"; onClicked: gui.backToPrevPage();
        anchors.right: parent.right; anchors.leftMargin: 3
        anchors.bottom: parent.bottom; anchors.bottomMargin: 3
	opacity: 0;
    }

}

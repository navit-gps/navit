import Qt 4.6

Rectangle {

    function hideButtons() {    
       	if ( gui.returnSource.split('/').length < 4  ) {
		btnBack.opacity=0;
	}
	if ( gui.returnSource == "/main.qml" ) {
		btnHome.opacity=0;
		btnBack.opacity=0;
	}
    }

   Component.onCompleted: hideButtons();

    ButtonIcon {
        id: btnMap; icon: "gui_map.svg"; text:"Map"; onClicked: gui.backToMap();
        anchors.left: parent.left; anchors.leftMargin: 3
        anchors.bottom: parent.bottom; anchors.bottomMargin: 3
    }

    ButtonIcon {
        id: btnHome; icon: "gui_home.svg"; text: "Home"; onClicked: { gui.returnSource=""; gui.setPage("main.qml"); }
        anchors.horizontalCenter: parent.horizontalCenter; anchors.leftMargin: 3
        anchors.bottom: parent.bottom; anchors.bottomMargin: 3
    }

    ButtonIcon {
        id: btnBack; icon: "gui_arrow_left.svg"; text: "Back"; onClicked: gui.backToPrevPage();
        anchors.right: parent.right; anchors.leftMargin: 3
        anchors.bottom: parent.bottom; anchors.bottomMargin: 3
    }

}

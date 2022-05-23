import Qt 4.7
import "pagenavigation.js" as Navit

Rectangle {

    function onStartup(currentPage) {
	btnBack.opacity=0;
	btnQuit.opacity=0;
       	if ( gui.lengthPage() > 1  ) {
		btnBack.opacity=1;
	}
	if ( gui.lengthPage() == 1 && currentPage == "PageMain.qml" ) {
		btnQuit.opacity=1;
	}
    }


    ButtonIcon {
        id: btnMap; icon: "gui_map.svg"; text:"Map"; onClicked: gui.backToMap();
        anchors.left: parent.left; anchors.leftMargin: 3
        anchors.bottom: parent.bottom; anchors.bottomMargin: 3
    }

    ButtonIcon {
        id: btnBack; icon: "gui_arrow_left.svg"; text: "Back"; onClicked: Navit.back();
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

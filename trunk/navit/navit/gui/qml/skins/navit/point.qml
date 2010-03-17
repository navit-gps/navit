import Qt 4.6

Rectangle {
    id: page

    width: 800; height: 424
    color: "Black"
    opacity: 0

    function pageOpen() {
        page.opacity = 1;
    }
    
    Component.onCompleted: pageOpen();    
    
    opacity: Behavior {
        NumberAnimation { id: opacityAnimation; duration: 300; alwaysRunToEnd: true }
    }

    Text {
	id: infoTxt;
    	text: point.coordString; color: "White"; font.pointSize: 20;
	anchors.top:parent.top;anchors.horizontalCenter:parent.horizontalCenter
    }
    Text {
	id: nameTxt;
    	text: point.pointName; color: "White"; font.pointSize: 20;
	anchors.top:infoTxt.bottom;anchors.topMargin: 5;anchors.horizontalCenter:parent.horizontalCenter
    }

    Grid {
        columns: 3;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.bottom: parent.verticalCenter; anchors.bottomMargin: 48;
        spacing: 64
        ButtonIcon {
            id: btnView; text: "View on map"; icon: "gui_maps.svg"; onClicked: console.log("Implement me!");
        }
        ButtonIcon {
            id: btnRoadbook; text: "Set as position"; icon: "gui_active.svg"; onClicked: { navit.setPosition(); gui.backToMap() }
        }
        ButtonIcon {
            id: btnSettings; text: "Set as destination"; icon: "gui_active.svg"; onClicked: { navit.setDestination(); gui.backToMap() }
        }
    }

    Grid {
        columns: 3;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.top: parent.verticalCenter; anchors.topMargin: 48;
        spacing: 64
        ButtonIcon {
            id: btnQuit; text: "Nearest POIs"; icon: "attraction.svg"; onClicked: console.log("Implement me!");
        }
        ButtonIcon {
            id: btnStop; text: "Add as a Bookmark"; icon: "gui_bookmark.svg"; onClicked: console.log("Implement me!");
        }
        ButtonIcon {
            id: btnInfo; text: "Point information"; icon: "gui_menu.svg"; onClicked: console.log("Implement me!");
        }
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

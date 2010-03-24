import Qt 4.6

Rectangle {
    id: page

    width: gui.width; height: gui.height
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
    	text: point.coordString; color: "White"; font.pointSize: gui.height/20;
	anchors.top:parent.top;anchors.horizontalCenter:parent.horizontalCenter
    }
    Text {
	id: nameTxt;
    	text: point.pointName; color: "White"; font.pointSize: gui.height/20;
	anchors.top:infoTxt.bottom;anchors.topMargin: 5;anchors.horizontalCenter:parent.horizontalCenter
    }

    Grid {
        columns: 3;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.bottom: parent.verticalCenter; anchors.bottomMargin: gui.height/16;
        spacing: gui.width/12
        ButtonIcon {
            id: btnView; text: "View on map"; icon: "gui_maps.svg"; onClicked: console.log("Implement me!");
        }
        ButtonIcon {
            id: btnRoadbook; text: "Set as\nposition"; icon: "gui_active.svg"; onClicked: { navit.setPosition(); gui.backToMap() }
        }
        ButtonIcon {
            id: btnSettings; text: "Set as\ndestination"; icon: "gui_active.svg"; onClicked: { navit.setDestination(); gui.backToMap() }
        }
    }

    Grid {
        columns: 3;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.top: parent.verticalCenter; anchors.topMargin: gui.height/16;
        spacing: gui.width/12
        ButtonIcon {
            id: btnQuit; text: "Nearest\nPOIs"; icon: "attraction.svg"; onClicked: console.log("Implement me!");
        }
        ButtonIcon {
            id: btnBookmark; text: "Add as\na Bookmark"; icon: "gui_bookmark.svg"; onClicked: gui.setPage("PageBookmarksAdd.qml")
        }
        ButtonIcon {
            id: btnInfo; text: "Point\ninformation"; icon: "gui_menu.svg"; onClicked: console.log("Implement me!");
        }
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

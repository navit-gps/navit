import Qt 4.7
import "pagenavigation.js" as Navit

Rectangle {
    id: page

    width: gui.width; height: gui.height
    color: "Black"
    opacity: 0

    function pageOpen() {
	if (point.pointType!="Bookmark") {
	    btnAddBookmark.opacity=1;
	}
	if (point.pointType!="MapPoint") {
	    btnView.opacity=1;
	}
	if (point.pointType!="Position") {
	    btnPosition.opacity=1;
	}
	if (point.pointType!="Destination") {
	    btnDestination.opacity=1;
	}
        page.opacity = 1;
    }

    Component.onCompleted: pageOpen();

    Behavior on opacity {
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

        ButtonIcon {
            id: btnView; text: "View on map"; icon: "gui_maps.svg"; onClicked: { navit.setCenter(); gui.backToMap(); }
	    opacity: 0
	    anchors.top: nameTxt.bottom;anchors.topMargin:gui.height/32
	    anchors.left:page.left;anchors.leftMargin: gui.height/6
        }
        ButtonIcon {
            id: btnPosition; text: "Set as\nposition"; icon: "gui_active.svg"; onClicked: { navit.setPosition(); gui.backToMap() }
	    opacity: 0
	    anchors.top: nameTxt.bottom;anchors.topMargin:gui.height/32
	    anchors.left:btnView.right;anchors.leftMargin: gui.height/6
        }
        ButtonIcon {
            id: btnDestination; text: "Set as\ndestination"; icon: "gui_active.svg"; onClicked: { route.addDestination(); gui.backToMap() }
	    opacity: 0
	    anchors.top: nameTxt.bottom;anchors.topMargin:gui.height/32
	    anchors.left:btnPosition.right;anchors.leftMargin: gui.height/6
        }
        ButtonIcon {
            id: btnAddBookmark; text: "Add as\na bookmark"; icon: "gui_active.svg"; onClicked: Navit.load("PageBookmarksAdd.qml");
	    opacity: 0
	    anchors.top: nameTxt.bottom;anchors.topMargin:gui.height/32
	    anchors.left:btnDestination.right;anchors.leftMargin: gui.height/6
        }
    Grid {
        columns: 4;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
	anchors.top: btnDestination.bottom;anchors.topMargin:gui.height/32
        spacing: gui.height/6
        ButtonIcon {
            id: btnBookmarks; text: "Bookmarks"; icon: "gui_bookmark.svg"; onClicked: {bookmarks.moveRoot(); Navit.load("PageBookmarks.qml"); }
        }
        ButtonIcon {
            id: btnPOI; text: "Nearest\nPOIs"; icon: "attraction.svg"; onClicked: Navit.load("PagePoi.qml");
        }
        ButtonIcon {
            id: btnSearch; text: "Address\nSearch"; icon: "gui_town.svg"; onClicked: Navit.load("PageSearch.qml");
        }
        ButtonIcon {
            id: btnInfo; text: "Point\ninformation"; icon: "gui_menu.svg"; onClicked: Navit.load("PagePointInfo.qml");
        }
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

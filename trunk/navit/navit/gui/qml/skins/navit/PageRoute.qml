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

    Grid {
        columns: 3;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.bottom: parent.verticalCenter; anchors.bottomMargin: gui.height/16;
        spacing: gui.width/12
        ButtonIcon {
            id: btnBookmarks; text: "Bookmarks"; icon: "gui_bookmark.svg"; onClicked: console.log("Implement me!");
        }
        ButtonIcon {
            id: btnDestinations; text: "Destinations"; icon: "gui_bookmark.svg"; onClicked: console.log("Implement me!");
        }
        ButtonIcon {
            id: btnSettings; text: "Locations"; icon: "gui_bookmark.svg"; onClicked: console.log("Implement me!");
        }
    }

    Grid {
        columns: 2;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.top: parent.verticalCenter; anchors.topMargin: gui.height/16;
        spacing: gui.width/12
        ButtonIcon {
            id: btnAbout; text: "Search"; icon: "gui_town.svg"; onClicked: gui.setPage("PageSearch.qml");
        }
        ButtonIcon {
            id: btnQuit; text: "POIs"; icon: "attraction.svg"; onClicked: console.log("Implement me!");
        }
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

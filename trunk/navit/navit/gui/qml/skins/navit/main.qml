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
        columns: 2; rows: 2
        anchors.horizontalCenter: parent.horizontalCenter;
	anchors.top: parent.top;anchors.topMargin:gui.height/10
        spacing: gui.height/6

	ButtonIcon {
            id: btnDestination; text: "Drive to\npoint on map"; icon: "gui_active.svg"; onClicked: { navit.setDestination(); gui.backToMap() }
        }
        ButtonIcon {
            id: btnNavigate; text: "Navigate\nto . . ."; icon: "cursor.svg";  onClicked: gui.setPage("PageRoute.qml")
        }
        ButtonIcon {
            id: btnRoute; text: "Route\ninformation"; icon: "nav_destination_wh.svg"; onClicked: gui.setPage("PageNavigation.qml")
        }
        ButtonIcon {
            id: btnSettings; text: "Settings"; icon: "gui_settings.svg"; onClicked: gui.setPage("PageSettings.qml")
        }
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

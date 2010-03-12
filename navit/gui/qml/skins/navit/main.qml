import Qt 4.6

Rectangle {
    id: page

    width: 800; height: 424
    color: "Black"
    opacity: 0

    function pageOpen() {
    	gui.returnSource="NoReturnTicket";
        page.opacity = 1;
    }
    
    Component.onCompleted: pageOpen();    
    
    opacity: Behavior {
        NumberAnimation { id: opacityAnimation; duration: 300; alwaysRunToEnd: true }
    }


    Grid {
        columns: 3;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.bottom: parent.verticalCenter; anchors.bottomMargin: 48;
        spacing: 64
        ButtonIcon {
            id: btnRoute; text: "Route"; icon: "cursor.svg";  onClicked: { gui.returnSource="main.qml";gui.setPage("PageRoute.qml") }
        }
        ButtonIcon {
            id: btnNavigation; text: "Navigation"; icon: "nav_destination_wh.svg"; onClicked: { gui.returnSource="main.qml";gui.setPage("PageNavigation.qml") }
        }
        ButtonIcon {
            id: btnSettings; text: "Settings"; icon: "gui_settings.svg"; onClicked: { gui.returnSource="main.qml";gui.setPage("PageSettings.qml") }
        }
    }

    Grid {
        columns: 2;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.top: parent.verticalCenter; anchors.topMargin: 48;
        spacing: 64
        ButtonIcon {
            id: btnAbout; text: "About"; icon: "gui_about.svg"; onClicked: { gui.returnSource="main.qml";gui.setPage("PageAbout.qml") }
        }
        ButtonIcon {
            id: btnQuit; text: "Quit"; icon: "gui_quit.svg"; onClicked: navit.quit();
        }
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

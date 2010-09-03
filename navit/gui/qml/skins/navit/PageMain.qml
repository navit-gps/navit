import Qt 4.7
import "pagenavigation.js" as Navit

Rectangle {
    id: page

    width: main.width; height: main.height
    color: "Black"

    Grid {
        columns: 2; rows: 2
        anchors.horizontalCenter: parent.horizontalCenter;
	anchors.top: page.top;anchors.topMargin:page.height/10
        spacing: parent.height/6

	ButtonIcon {
            id: btnDestination; text: "Drive to\npoint on map"; icon: "gui_active.svg"; onClicked: { route.addDestination(); gui.backToMap() }
        }
        ButtonIcon {
            id: btnNavigate; text: "Navigate\nto . . ."; icon: "cursor.svg";  onClicked: Navit.load("PageNavigate.qml");
        }
        ButtonIcon {
            id: btnRoute; text: "Route\ninformation"; icon: "nav_destination_wh.svg"; onClicked: Navit.load("PageRoute.qml");
        }
        ButtonIcon {
            id: btnSettings; text: "Settings"; icon: "gui_settings.svg"; onClicked: Navit.load("PageSettings.qml");
        }
    }
}

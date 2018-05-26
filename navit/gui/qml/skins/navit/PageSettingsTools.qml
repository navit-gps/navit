import Qt 4.7
import "pagenavigation.js" as Navit

Rectangle {
    id: page

    width: gui.width; height: gui.height
    border.width: 1
    color: "Black"
    opacity: 0

    function pageOpen() {
        page.opacity = 1;
    }

    Component.onCompleted: pageOpen();

    Behavior on opacity {
        NumberAnimation { id: opacityAnimation; duration: 300; alwaysRunToEnd: true }
    }

    Grid {
        columns: 1;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.verticalCenter: parent.verticalCenter;
        ButtonIcon {
            id: btnDisplay; text: "Locale"; icon: "gui_actions.svg"; onClicked: Navit.load("PageSettingsLocale.qml");
        }
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

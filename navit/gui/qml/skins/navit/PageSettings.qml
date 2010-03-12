import Qt 4.6

Rectangle {
    id: page

    width: 800; height: 424
    border.width: 1
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
        anchors.bottom: parent.verticalCenter; anchors.bottomMargin: 48;
        spacing: 64
        ButtonIcon {
            id: btnDisplay; text: "Display"; icon: "gui_display.svg"; onClicked: console.log("Implement me!");
        }
        ButtonIcon {
            id: btnMap; text: "Map"; icon: "gui_maps.svg"; onClicked: console.log("Implement me!");
        }
        ButtonIcon {
            id: btnVehicle; text: "Vehicle"; icon: "gui_vehicle.svg"; onClicked: console.log("Implement me!");
        }
    }
    Grid {
        columns: 2;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.top: parent.verticalCenter; anchors.topMargin: 48;
        spacing: 64
        ButtonIcon {
            id: btnRules; text: "Rules"; icon: "gui_rules.svg"; onClicked: { gui.returnSource="PageSettings.qml";gui.setPage("PageSettingsRules.qml") }
        }
        ButtonIcon {
            id: btnTools; text: "Tools"; icon: "gui_tools.svg"; onClicked: { gui.returnSource="PageSettings.qml";gui.setPage("PageSettingsTools.qml") }
        }
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

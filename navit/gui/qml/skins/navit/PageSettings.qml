import Qt 4.6

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
        columns: 3;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.bottom: parent.verticalCenter; anchors.bottomMargin: gui.height/16;
        spacing: gui.width/12
        ButtonIcon {
            id: btnDisplay; text: "Display"; icon: "gui_display.svg"; onClicked: gui.setPage("PageSettingsDisplay.qml")
        }
        ButtonIcon {
            id: btnMap; text: "Map"; icon: "gui_maps.svg"; onClicked: console.log("Implement me!");
        }
        ButtonIcon {
            id: btnVehicle; text: "Vehicle"; icon: "gui_vehicle.svg"; onClicked: gui.setPage("PageSettingsVehicle.qml")
        }
    }
    Grid {
        columns: 3;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.top: parent.verticalCenter; anchors.topMargin: gui.height/16;
        spacing: gui.width/12
        ButtonIcon {
            id: btnRules; text: "Rules"; icon: "gui_rules.svg"; onClicked: gui.setPage("PageSettingsRules.qml")
        }
        ButtonIcon {
            id: btnTools; text: "Tools"; icon: "gui_tools.svg"; onClicked: gui.setPage("PageSettingsTools.qml")
        }
        ButtonIcon {
            id: btnAbout; text: "About"; icon: "gui_about.svg"; onClicked: gui.setPage("PageAbout.qml")
        }
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

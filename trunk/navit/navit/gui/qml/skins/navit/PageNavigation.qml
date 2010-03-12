
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

    Grid {
        columns: 3;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.bottom: parent.verticalCenter; anchors.bottomMargin: 48;
        spacing: 64
        ButtonIcon {
            id: btnView; text: "View on map"; icon: "gui_maps.svg"; onClicked: console.log("Implement me!");
        }
        ButtonIcon {
            id: btnRoadbook; text: "Roadbook"; icon: "gui_log.svg"; onClicked: console.log("Implement me!");
        }
        ButtonIcon {
            id: btnSettings; text: "Height profile"; icon: "peak.svg"; onClicked: console.log("Implement me!");
        }
    }

    Grid {
        columns: 2;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.top: parent.verticalCenter; anchors.topMargin: 48;
        spacing: 64
        ButtonIcon {
            id: btnQuit; text: "POIs"; icon: "attraction.svg"; onClicked: console.log("Implement me!");
        }
        ButtonIcon {
            id: btnStop; text: "Stop"; icon: "gui_stop.svg"; onClicked: console.log("Implement me!");
        }
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

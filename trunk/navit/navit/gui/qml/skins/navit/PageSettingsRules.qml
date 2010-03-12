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
        columns: 1; rows: 3
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.verticalCenter: parent.verticalCenter;
        spacing: 64
        ToggleSwitch {
             on: navit.getAttr("tracking");  text: "Lock on road"
        }
        ToggleSwitch {
             on: navit.getAttr("orientation");  text: "Northing"
        }
	ToggleSwitch {
             on: navit.getAttr("follow_cursor");  text: "Map follows Vehicle"
        }
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

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
    
    opacity: Behavior {
        NumberAnimation { id: opacityAnimation; duration: 300; alwaysRunToEnd: true }
    }

    Grid {
        columns: 1; rows: 3
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.verticalCenter: parent.verticalCenter;
        spacing: gui.width/12
        ToggleSwitch {
             id: trackingSw; on: navit.getAttr("tracking");  text: "Lock on road"; onChanged: navit.setAttr("tracking",trackingSw.on)
        }
        ToggleSwitch {
             id: orientationSw; on: navit.getAttr("orientation");  text: "Northing"; onChanged: navit.setAttr("orientation",orientationSw.on)
        }
	ToggleSwitch {
             id: followrcursorSw; on: navit.getAttr("follow_cursor");  text: "Map follows Vehicle"; onChanged: navit.setAttr("follow_cursor",followrcursorSw.on)
        }
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

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
             id: trackingSw; stOn: navit.getAttr("tracking");  text: "Lock on road"; onChanged: navit.setAttr("tracking",trackingSw.stOn)
        }
        ToggleSwitch {
             id: orientationSw; stOn: navit.getAttr("orientation");  text: "Northing"; onChanged: navit.setAttr("orientation",orientationSw.stOn)
        }
	ToggleSwitch {
             id: followrcursorSw; stOn: navit.getAttr("follow_cursor");  text: "Map follows Vehicle"; onChanged: navit.setAttr("follow_cursor",followrcursorSw.stOn)
        }
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

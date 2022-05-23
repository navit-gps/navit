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

  VisualItemModel {
      	id: selectorsModel

        ToggleSwitch { id: trackingSw; stOn: navit.getAttr("tracking");  text: "Lock on road"; onChanged: navit.setAttr("tracking",trackingSw.stOn)  }
        ToggleSwitch { id: orientationSw; stOn: navit.getAttr("orientation");  text: "Northing"; onChanged: navit.setAttr("orientation",orientationSw.stOn) }
	ToggleSwitch { id: followcursorSw; stOn: navit.getAttr("follow_cursor");  text: "Map follows Vehicle"; onChanged: navit.setAttr("follow_cursor",followcursorSw.stOn) }
	ToggleSwitch { id: autozoomSw; stOn: navit.getAttr("autozoom_active");  text: "Auto zoom"; onChanged: navit.setAttr("autozoom_active",autozoomSw.stOn) }
  }

  ListView {
      	id: selectorsList; model: selectorsModel
        focus: true; clip: true; orientation: Qt.Vertical
	anchors.verticalCenter: parent.verticalCenter; anchors.horizontalCenter: parent.horizontalCenter;
	width: trackingSw.width*1.5; height: trackingSw.height*4
  }
    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

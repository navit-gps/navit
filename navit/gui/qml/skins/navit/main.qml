import Qt 4.7
import "pagenavigation.js" as Navit

Rectangle {
    id: main
    property variant returnPath:[ "test" ]

    width: gui.width; height: gui.height
    color: "Black"

    function pageOpen() {
    	Navit.load("PageMain.qml");
    }

    Component.onCompleted: pageOpen();

    Behavior on opacity {
        NumberAnimation { id: opacityAnimation; duration: 300; alwaysRunToEnd: true }
    }


    Loader {
    	id: pageLoader;
	width: gui.width;
	height: gui.height-cellar.height;
	anchors.horizontalCenter: parent.horizontalCenter;
	anchors.verticalCenter: parent.verticalCenter;
    }

    Cellar {id: cellar;anchors.bottom: main.bottom; anchors.horizontalCenter: main.horizontalCenter; width: main.width }
}

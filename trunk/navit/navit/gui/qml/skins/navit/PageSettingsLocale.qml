
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
        columns: 1;rows: 3
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.verticalCenter: parent.verticalCenter; 

	Text { id: lang; anchors.bottom: langname.top; text: gui.localeName; color: "White";font.pointSize: gui.height/24 }
	Text { id: langname; anchors.centerIn: parent; text: gui.langName; color: "White";font.pointSize: gui.height/24 }
	Text { id: ctryname; anchors.top: langname.bottom; text: gui.ctryName; color: "White";font.pointSize: gui.height/24 }
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

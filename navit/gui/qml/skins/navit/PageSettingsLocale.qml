
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
        columns: 1;rows: 3
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.verticalCenter: parent.verticalCenter; 

	Text { id: lang; anchors.bottom: langname.top; text: "Hi, i'm Navit!"; color: "White" }
	Text { id: langname; anchors.centerIn: parent; text: "Hi, i'm Navit!"; color: "White" }
	Text { id: ctryname; anchors.top: langname.bottom; text: "Hi, i'm Navit!"; color: "White" }
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

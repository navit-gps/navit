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

    Text { id: myText; anchors.centerIn: parent; text: "Hi, i'm Navit!"; color: "White"; font.pointSize: gui.height/32 }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

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

    Text { id: navitText; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: urlText.top; text: "Navit"; color: "White"; font.pointSize: gui.height/32 }
    Text { id: urlText; anchors.horizontalCenter: parent.horizontalCenter; anchors.verticalCenter: parent.verticalCenter; text: "http://www.navit-project.org/"; color: "White"; font.pointSize: gui.height/32 }
    Text { id: authorsText; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: urlText.bottom; text: "By: Martin Schaller, Michael Farmbauer, Alexander Atanasov, Pierre Grandin"; color: "White"; font.pointSize: gui.height/32 }
    Text { id: contributorsText; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: authorsText.bottom; text: "and all the Navit Team members and contributors"; color: "White"; font.pointSize: gui.height/32 }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

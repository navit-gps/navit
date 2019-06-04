import Qt 4.7

Rectangle {
    id: container

    signal clicked
    property string text: "Button"
    property string icon: "Icon.png"

    color: "black"; smooth: true; opacity: 0.75
    width: imgItem.width+20; height: txtItem.height + 6 + imgItem.height;
    transformOrigin: Rectangle.Center;

    MouseArea { id: mr; anchors.fill: parent; onClicked: container.clicked() }

   Image {
	id: imgItem; source: gui.iconPath+container.icon; anchors.top: container.top; anchors.horizontalCenter: container.horizontalCenter;
	width: gui.height/4; height: gui.height/4
   }

    Text {
        id: txtItem; text: container.text; anchors.top: imgItem.bottom; anchors.horizontalCenter: container.horizontalCenter;
	color: "White"; font.pointSize: gui.height/32; horizontalAlignment: Qt.AlignHCenter
    }

    states: [
        State {
            name: "Pressed"; when: mr.pressed == true
            PropertyChanges { target: container; scale: 2.0 }
            PropertyChanges { target: container; opacity: 1 }

        }
    ]

    transitions: Transition {
        NumberAnimation { properties: "scale"; easing.type: "OutExpo"; duration: 200 }
        NumberAnimation { properties: "opacity"; easing.type: "InQuad"; duration: 200 }
    }

}

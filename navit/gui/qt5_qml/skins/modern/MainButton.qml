import QtQuick 2.2
import QtQuick.Layouts 1.0

Rectangle {
    id: container

    signal clicked
    property string text: "Button"
    property string icon: "icons/appbar.home.variant.svg"

    smooth: true;
    width: 200;
    height: 80
    color: "#35322f"
    radius: 2
    border.width: 1

    MouseArea { id: mr; anchors.fill: parent; onClicked: container.clicked() }

    Text {
        id: txtItem;
        color: "#ffffff"
        text: container.text
        font.pointSize: 18
        anchors.left: rectangle.right
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        z: 1
    }

    Rectangle {
        id: rectangle
        width: container.height
        height: container.height
        color: "#35322f"
        radius: 2
        border.width: 1
        anchors.verticalCenter: parent.verticalCenter

        Image {
            id: imgItem;
            width: parent.width;
            height: parent.height;
            source: container.icon
            smooth: true
            scale: 1
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
//            anchors.leftMargin: 8
            sourceSize.width: parent.width
            sourceSize.height: parent.height
        }
    }

    states: [
        State {
            name: "Pressed"; when: mr.pressed == true
            PropertyChanges {
                target: txtItem;
                opacity: 0.6;
                color: "#84ca43";
            }
        }
    ]

    transitions: Transition {
        NumberAnimation { properties: "scale"; easing.type: "OutExpo"; duration: 200 }
    }

}


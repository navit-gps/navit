import QtQuick.Layouts 1.0
import QtQuick 2.2

Rectangle {
    id: rectangle
    visible: true
    height: 300
    color: "#000000"

    Rectangle {
        id: rectangle1
        x: 0
        y: 200
        height: 300
        color: "#1e1b18"
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0

        Loader {
            id: menucontent
            width: parent.width
            height: parent.height
            Component.onCompleted: menucontent.source = "MainMenu.qml"
        }

    }

    MainButton {
        id: mainButton3
        width: 260
        height: 80
        radius: 1
        text: "Map"
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        Layout.fillHeight: true
        Layout.fillWidth: true
        icon: "icons/appbar.map.svg"
        onClicked: {
            mainMenu.state = ''
            mainMenu.source = ""
        }

    }

}

import QtQuick.Layouts 1.0
import QtQuick 2.2

Item {
    Rectangle {
        id: menuArea
        color: "#1e1b18"
        width: parent.width
        y: topBar.height
        height: parent.height - ( topBar.height + bottomBar.height )

        Loader {
            id: menucontent
            width: parent.width
            height: parent.height
            Component.onCompleted: {
                console.log("submenu : " + mainMenu.submenu)
                menucontent.source = mainMenu.submenu
            }
        }
    }

    Rectangle {
        id: bottomBar
        width: parent.width
        height: 64
        color: "#1e1b18"
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0

        MainButton {
            id: mainButton3
            x: 380
            y: 220
            width: 260
            height: 56
            radius: 1
            text: "Map"
            anchors.right: parent.right
            anchors.rightMargin: 4
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 4
            Layout.fillHeight: true
            Layout.fillWidth: true
            icon: "icons/appbar.map.svg"
            onClicked: {
                 container.hideMainMenu()
            }

        }
    }

    Rectangle {
        id: topBar
        width: parent.width
        height: 80
        color: "#1e1b18"
        anchors.top: parent.top
    }

}

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

ApplicationWindow {
    id: applicationWindow
    visible: true
    width: 800
    height: 480
    title: qsTr("Navit Stub")

    MouseArea {
        id: mouseArea
        z: -1
        anchors.fill: parent
        onClicked: {
            mainMenu.source = "skins/modern/menu.qml"
            mainMenu.state = 'visible'
            console.log("showing menu")
        }

        Image {
            id: image
            anchors.left: parent.left
            anchors.rightMargin: 0
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            source: "map.png"
        }

    }

    Loader {
        id: mainMenu
        width: parent.width
        height: parent.height
        x: parent.width
        opacity: 0

        states: [
            State {
                name: "visible"
                PropertyChanges {
                    target: mainMenu
                    x: 0
                    opacity: 1
                }
            }
        ]
        transitions: [
            Transition {
                NumberAnimation {
                    properties: "x,y,opacity";duration: 300
                }
            }
        ]
    }

}

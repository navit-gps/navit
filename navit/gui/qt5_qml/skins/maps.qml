import QtQuick 2.0

ListView {
    model: maps
    delegate: Rectangle {
        height: 64
        color: "#ff0000"
        radius: 2
        border.width: 1

        Image {
            id: image1
            height: parent.height - 4;
            source : active ? "icons/appbar.layer.svg" : "icons/appbar.layer.delete.svg"
            opacity: active ? 1 : 0.4
        }

        Text {
            text: name
            color: "#ffffff"
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: image1.right
            anchors.leftMargin: 8
        }

        MouseArea{
            anchors.fill: parent
            hoverEnabled: true
            onEntered: {
                // backend.list_maps(1)
            }
        }
    }

    Component.onCompleted: backend.list_maps(0)
}

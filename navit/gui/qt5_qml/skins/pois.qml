import QtQuick 2.0

Item {
    ListView {
        model: pois
        anchors.fill: parent
        id: listView
        delegate: Rectangle {
            height: 64
            color: "#ff0000"
            radius: 2
            border.width: 1

            Image {
                id: image1
                height: parent.height - 4;
                source : model.modelData.active ? "icons/appbar.layer.svg" : "icons/appbar.layer.delete.svg"
                opacity: model.modelData.active ? 1 : 0.4
            }

            Text {
                width: 128
                id: distanceText
                text: distance
                color: "#ffffff"
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: image1.right
                anchors.leftMargin: 8
            }

            Text {
                text: name
                color: "#ffffff"
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: distanceText.right
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

        Component.onCompleted: backend.get_pois()
    }

    Rectangle {
        height: 64
        width: height

        x: parent.width - width
        y: parent.height - height * 2

        color: "#35322f"

        Image {
            anchors.centerIn: parent
            source: "icons/appbar.chevron.up.svg"
        }

        MouseArea {
            anchors.fill: parent
            id: listUp

            SmoothedAnimation {
                target: listView
                property: "contentY"
                running: listUp.pressed
                velocity: 1000
                to: 0
            }
            onReleased: {
                if (!listView.atYBeginning)
                    listView.flick(0, 1000)
            }
        }
    }



    Rectangle {
        height: 64
        width: height

        x: parent.width - width
        y: parent.height - height

        color: "#35322f"

        Image {
            anchors.centerIn: parent
            source: "icons/appbar.chevron.down.svg"
        }

        MouseArea {
            anchors.fill: parent
            id: listDown

            SmoothedAnimation {
                target: listView
                property: "contentY"
                running: listDown.pressed
                to: listView.contentHeight - listView.height
                velocity: 1000
            }
            onReleased: {
                if (!listView.atYEnd)
                    listView.flick(0, -1000)
            }
        }
    }



}

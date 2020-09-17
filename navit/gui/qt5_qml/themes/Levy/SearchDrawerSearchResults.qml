import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

import "icons.js" as Icons

Item {
    id: __root
    signal itemClicked (var index)
    property alias results : listView.model
    property int boxRadius : 10
    clip: true

    Rectangle {
        id: rectangle
        color: "#ffffff"
        anchors.fill: parent
        radius: boxRadius
    }

    ListView {
        id: listView
        clip: true
        anchors.fill: parent
        anchors.topMargin: parent.height * 0.05
        anchors.bottomMargin: parent.height * 0.05
        anchors.leftMargin: parent.width * 0.05
        anchors.rightMargin: parent.width * 0.05
        delegate: Item {
            id: element
            height: 80
            width: parent.width

            Text {
                id: element1
                text: name
                font.pointSize: 12
                anchors.left: distanceText.right
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                id: element2
                text: address
                wrapMode: Text.NoWrap
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.top: element1.bottom
                anchors.left: distanceText.right
                font.bold: false
            }

            Text {
                id: distanceText
                width: height * 2
                height: parent.height * 0.8
                text: (distance/1000).toFixed(1)
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.left: parent.left
                font.pixelSize: height * 0.8
                anchors.top: parent.top
                anchors.topMargin: 0
            }

            Text {
                id: awayText
                text: qsTr("kilometers away")
                anchors.horizontalCenter: distanceText.horizontalCenter
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                anchors.top: distanceText.bottom
                anchors.bottom: parent.bottom
                font.pixelSize: 12
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                onClicked: {
                    __root.itemClicked(index)
                }

                Rectangle {
                    color: "#33cb0b0b"
                    anchors.fill: parent
                }
            }

        }
    }


}





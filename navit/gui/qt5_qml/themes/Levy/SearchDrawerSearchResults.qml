import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

import "icons.js" as Icons

Item {
    id: __root
    signal resultClicked()
    property alias model: listView.model

    ListView {
        id: listView
        anchors.fill: parent
        delegate: Item {
            id: element
            x: 5
            height: 80
            MouseArea {
                anchors.fill: parent
                onClicked: __root.resultClicked()
            }

            Text {
                id: element1
                text: name
                font.pointSize: 12
                anchors.verticalCenterOffset: -height/2
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
                font.bold: true
            }

            Text {
                id: distanceText
                text: distance
                font.pixelSize: parent.height * 0.8
                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.top: parent.top
                anchors.topMargin: 0
                anchors.bottom: parent.bottom
            }

        }
    }

}

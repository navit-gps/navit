import QtQuick 2.9
import QtQuick.Layouts 1.3

Item {
    id: __root

    signal startButtonClicked()
    signal routeDetailsClicked()
    signal cancelButtonClicked()

    property alias timeLeft: timeLeft.text
    property alias distanceLeft: distanceLeft.text
    property alias arrivalTime: arrivingTime.text

    Rectangle{
        id: buttonsBackground
        height: parent.height
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        border.width: 1
        radius: height / 2
        visible: true

        Item {
            id: buttonsWrapper
            anchors.rightMargin: buttonsBackground.radius
            anchors.leftMargin: buttonsBackground.radius
            anchors.bottomMargin: 1
            anchors.topMargin: 1
            anchors.fill: parent


            Item {
                id: journeyLeftWrapper
                width: parent.width / 5
                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.top: parent.top
                anchors.topMargin: 0
                anchors.bottom: parent.bottom
                visible: true
                Layout.fillHeight: true
                Layout.fillWidth: true

                Item {
                    id: leftWrapper
                    width: timeLeft.width + distanceLeft.width
                    anchors.verticalCenter: parent.verticalCenter
                    clip: false
                    height: timeLeft.height + distanceLeft.height
                    anchors.horizontalCenter: parent.horizontalCenter


                    Text {
                        id: timeLeft
                        text: qsTr("15 mins")
                        font.pixelSize: __root.height / 4
                        anchors.verticalCenterOffset: -height / 2
                        anchors.verticalCenter: parent.verticalCenter
                    }



                    Text {
                        id: distanceLeft
                        text: qsTr("5 km")
                        font.pixelSize: __root.height / 5
                        anchors.verticalCenterOffset: height / 2
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: timeLeft.left
                    }
                }
            }

            RowLayout {
                id: rowLayout
                anchors.left: journeyLeftWrapper.right
                anchors.right: journeyRightWrapper.left
                anchors.bottom: parent.bottom
                anchors.top: parent.top
                spacing: 0

                Item {
                    id: cancelButtonWrapper
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    Rectangle {
                        color: "#b70d0d"
                        anchors.fill: parent
                    }

                    Rectangle {
                        width: 1
                        color: "#000000"
                        anchors.left: parent.left
                        anchors.leftMargin: 0
                        anchors.top: parent.top
                        anchors.topMargin: 0
                        anchors.bottom: parent.bottom
                    }


                    Text {
                        color: "#ffffff"
                        text: qsTr("Cancel")
                        anchors.leftMargin: parent.width*0.05
                        anchors.rightMargin: parent.width*0.05
                        font.pixelSize: __root.height / 4
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: __root.cancelButtonClicked()
                    }
                }

                Item {
                    id: startButtonWrapper
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    Rectangle {
                        color: "#126828"
                        anchors.fill: parent
                    }

                    Rectangle {
                        width: 1
                        color: "#000000"
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.topMargin: 0
                        anchors.bottom: parent.bottom
                    }


                    Text {
                        color: "#ffffff"
                        text: qsTr("Start")
                        font.pixelSize: __root.height / 4
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        verticalAlignment: Text.AlignVCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                        horizontalAlignment: Text.AlignHCenter
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: __root.startButtonClicked()
                    }
                }
            }

            Item {
                id: journeyRightWrapper
                x: 810
                width: parent.width / 5
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 0
                anchors.bottom: parent.bottom
                Layout.fillHeight: true
                Layout.fillWidth: true

                Item {
                    id: rightWrapper
                    height: arrivingTimeLabel.height + arrivingTime.height
                    anchors.right: parent.right
                    anchors.left: parent.left
                    anchors.leftMargin: 0
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        id: arrivingTimeLabel
                        text: qsTr("Arriving:")
                        font.pixelSize: __root.height / 5
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                    }

                    Text {
                        id: arrivingTime
                        text: qsTr("16:29")
                        font.pixelSize: __root.height / 3
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        verticalAlignment: Text.AlignVCenter
                    }

                }
            }
        }
    }

}







/*##^## Designer {
    D{i:0;autoSize:true;height:120;width:1200}
}
 ##^##*/

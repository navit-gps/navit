import QtQuick 2.9
import QtQuick.Layouts 1.3

Item {
    id: __root
    signal searchButtonClicked()
    signal menuButtonClicked()

    Rectangle {
        id: leftButton
        width: height
        height: parent.height
        color: "#ffffff"
        radius: height/2
        anchors.verticalCenter: parent.verticalCenter
        clip: true
        border.width: 1
        anchors.left: parent.left
        anchors.leftMargin: 0

        Behavior on width {
            NumberAnimation { easing.type: Easing.InOutQuad; }
        }

        Rectangle {
            id: leftDivider
            width: 1
            color: "#000000"
            anchors.verticalCenter: parent.verticalCenter
            visible: false
            anchors.leftMargin: parent.height + 1
            anchors.left: parent.left
            Behavior on height {
                NumberAnimation { easing.type: Easing.InOutQuad; }
            }
        }


        Item {
            id: leftWrapper
            anchors.verticalCenter: parent.verticalCenter
            height: timeLeft.height + distanceLeft.height
            clip: true
            anchors.leftMargin: parent.height * 0.1
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: leftDivider.right


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

        Image {
            id: image
            width: height
            height: parent.height * 0.5
            anchors.leftMargin: (parent.height - width) / 2
            anchors.left: parent.left
            anchors.rightMargin: (parent.height - width) / 2
            anchors.verticalCenter: parent.verticalCenter
            fillMode: Image.PreserveAspectFit
            source: "assets/ionicons/md-search.svg"
            sourceSize.width: width
            sourceSize.height: height
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            onClicked: {
                __root.searchButtonClicked()
            }
        }
    }

    Rectangle {
        id: rightButton
        width: height
        height: parent.height
        color: "#ffffff"
        radius: height/2
        anchors.verticalCenter: parent.verticalCenter
        clip: true
        border.width: 1
        anchors.right: parent.right

        Behavior on width {
            NumberAnimation { easing.type: Easing.InOutQuad; }
        }

        Rectangle {
            id: rightDivider
            width: 1
            height: 0
            color: "#000000"
            anchors.verticalCenter: parent.verticalCenter
            visible: false
            anchors.rightMargin: parent.height
            anchors.right: parent.right
            Behavior on height {
                NumberAnimation { easing.type: Easing.InOutQuad; }
            }
        }

        Item {
            id: rightWrapper
            height: arrivingTimeLabel.height + arrivingTime.height
            clip: true
            anchors.right: rightDivider.left
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

        Image {
            id: image1
            width: height
            height: parent.height * 0.5
            anchors.rightMargin: (parent.height - width) / 2
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            fillMode: Image.PreserveAspectFit
            source: "qrc:/themes/Levy/assets/ionicons/md-menu.svg"
            sourceSize.width: width
            sourceSize.height: height
        }

        MouseArea {
            id: mouseArea1
            anchors.fill: parent
            onClicked: {
                __root.menuButtonClicked()
            }
        }
    }

    states: [
        State {
            name: "navigationState"

            PropertyChanges {
                target: leftButton
                width: parent.height > parent.width/5 ? parent.width* 0.45 : parent.height * 2.5
                height: parent.height > parent.width/5 ? parent.width/6 : parent.height
            }

            PropertyChanges {
                target: rightButton
                width: parent.height > parent.width/5 ? parent.width* 0.45 : parent.height * 2.5
                height: parent.height > parent.width/5 ? parent.width/6 : parent.height
            }

            PropertyChanges {
                target: rightDivider
                height: parent.height
                visible: true
            }

            PropertyChanges {
                target: leftDivider
                height: parent.height
                anchors.leftMargin: parent.height
                visible: true
            }

            PropertyChanges {
                target: rightWrapper
                visible: true
            }

            PropertyChanges {
                target: leftWrapper
                visible: true
            }

        },
        State {
            name: "destinationState"
            PropertyChanges {
                target: leftButton
                visible: false
            }

            PropertyChanges {
                target: rightButton
                visible: false
            }
        }
    ]

}







/*##^## Designer {
    D{i:0;autoSize:true;height:100;width:1000}D{i:2;anchors_height:200}D{i:4;anchors_height:200}
D{i:3;anchors_height:200;anchors_x:20}D{i:6;anchors_x:24;anchors_y:37}D{i:7;anchors_height:100;anchors_width:100}
D{i:9;anchors_height:200}D{i:14;anchors_height:100;anchors_width:100}D{i:8;anchors_height:200}
}
 ##^##*/

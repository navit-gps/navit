import QtQuick 2.9
import QtGraphicalEffects 1.0

Item {
    id: __root

    property int speed: 0
    property string speedUnit: "km/h"
    property int speedLimit: 0

    onSpeedChanged: {
        if(speed > speedLimit && speedLimit > 0){
            __root.state = "over_limit"
        } else {
            __root.state = ""
        }
    }
    onSpeedLimitChanged: {
        if(speed > speedLimit && speedLimit > 0){
            __root.state = "over_limit"
        } else {
            __root.state = ""
        }
    }

    Rectangle {
        id: rectangle
        width: height
        color: "#ffffff"
        radius: height
        border.width: 1
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter

        Item {
            id: element2
            anchors.fill: parent

            Text {
                id: speedText
                text: speed
                anchors.verticalCenterOffset: -height * 0.1
                font.pixelSize: parent.height * 0.4
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                id: speedUnitText
                text: speedUnit
                font.pixelSize: parent.height * 0.1
                anchors.verticalCenterOffset: height*2
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        Rectangle {
            id: rectangle1
            width: height
            height: parent.height /2
            color: "#ffffff"
            radius: height/2
            visible: speedLimit > 0
            border.width: height * 0.1
            border.color: "#000000"
            anchors.topMargin: -height/2
            anchors.rightMargin: -width/2
            anchors.right: parent.right
            anchors.top: parent.top

            Text {
                id: speedLimitText
                text: speedLimit
                font.pixelSize: parent.height * 0.4
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
    states: [
        State {
            name: "over_limit"

            PropertyChanges {
                target: speedText
                color: "#ee0808"
            }

            PropertyChanges {
                target: rectangle1
                anchors.rightMargin: -width * 0.4
                anchors.topMargin: -height * 0.4
                visible: true
                border.color: "#ee0808"
            }
        }
    ]

}





/*##^## Designer {
    D{i:0;autoSize:true;height:480;width:640}D{i:2;anchors_height:200;anchors_width:200}
D{i:1;anchors_height:200;anchors_y:267}
}
 ##^##*/

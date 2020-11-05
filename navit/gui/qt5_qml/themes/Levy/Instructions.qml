import QtQuick 2.9

Item {
    id: __root
    property string distance : "125 Meters"
    property string street: "High Street"
    property url icon: ""
    width:childrenRect.width
    Rectangle {
        id: rectangle
        color: "#ffffff"
        radius: parent.height/2
        anchors.bottom: parent.bottom
        anchors.top: parent.top
        border.color: "#000000"
        border.width: 1
        width:childrenRect.width + height

        Image {
            id: directionImage
            x: height / 2
            width: height
            height: parent.height * 0.8
            anchors.verticalCenter: parent.verticalCenter
            source: icon
            fillMode: Image.PreserveAspectFit
        }
        Item{
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: directionImage.right
            width: childrenRect.width
            anchors.leftMargin: height * 0.25

            Text {
                text: distance
                font.pixelSize: parent.height * 0.3
                anchors.verticalCenterOffset: -height/2
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: street
                font.pixelSize: parent.height * 0.2
                anchors.verticalCenterOffset: height/2
                anchors.verticalCenter: parent.verticalCenter
            }

        }
    }

}

















/*##^## Designer {
    D{i:0;height:150}
}
 ##^##*/

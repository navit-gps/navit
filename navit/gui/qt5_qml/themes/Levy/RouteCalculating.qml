import QtQuick 2.9

Item {
    Rectangle {
        id: rectangle
        color: "#ffffff"
        radius: parent.height/2
        border.width: 1
        anchors.fill: parent

        Text {
            id: calculatingText
            text: qsTr("Calculating Route...")
            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment: Text.AlignHCenter
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: 24
        }
    }

}

/*##^## Designer {
    D{i:0;autoSize:true;height:50;width:600}D{i:1;anchors_height:200;anchors_width:200}
}
 ##^##*/

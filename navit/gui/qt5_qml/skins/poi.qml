import QtQuick 2.0

Item {
    id: poiItem
    visible: true

    Text {
        x: 261
        y: 80
        color: "#ffffff"
        text: qsTr("Type")
        font.pixelSize: 24
    }

    Text {
        x: 261
        y: 162
        color: "#ffffff"
        text: qsTr("Distance")
        font.pixelSize: 24
    }

    Text {
        x: 8
        y: 8
        color: "#ffffff"
        text: backend.activePoi.name
        font.pixelSize: 32
    }

    Text {
        x: 404
        y: 80
        color: "#ffffff"
        text: backend.activePoi.type
        font.pixelSize: 24
    }

    Text {
        x: 484
        y: 162
        color: "#ffffff"
        text: backend.activePoi.distance
        font.pixelSize: 24
    }

    Image {
        x: 8
        y: 78
        height: parent.height /2 ;
        width: height
        source : backend.get_icon_path() + backend.activePoi.icon
        sourceSize.width: parent.width
        sourceSize.height: parent.height
    }


    MainButton {
        id: mainButton3
        x: 161
        y: 373
        width: 260
        height: 80
        radius: 1
        text: "Set as destination"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.right: parent.right
        anchors.rightMargin: 50
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 27
        icon: "icons/appbar.location.checkin.svg"
        onClicked: {
            backend.setActivePoiAsDestination()
        }

    }

}

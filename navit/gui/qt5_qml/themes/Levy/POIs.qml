import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3


Item {
    id: __root
    QtObject {
        id:navitPOIs
        property var poiTypeList : [
            {
                "name": "Petrol Station",
                "icon": "qrc:/themes/Levy/assets/osmic/transport/fuel-14.svg"
            },
            {
                "name": "Car Park",
                "icon": "qrc:/themes/Levy/assets/osmic/transport/parking-car-14.svg"
            },
            {
                "name": "Hotel",
                "icon": "qrc:/themes/Levy/assets/osmic/accommodation/hotel-14.svg"
            },
            {
                "name": "Restaurant",
                "icon": "qrc:/themes/Levy/assets/osmic/eat-drink/restaurant-14.svg"
            },
            {
                "name": "Bus",
                "icon": "qrc:/themes/Levy/assets/osmic/transport/bus-stop-14.svg"
            },
            {
                "name": "Airport",
                "icon": "qrc:/themes/Levy/assets/osmic/transport/airport-14.svg"
            },
            {
                "name": "Cafe",
                "icon": "qrc:/themes/Levy/assets/osmic/eat-drink/cafe-14.svg"
            },
            {
                "name": "Information",
                "icon": "qrc:/themes/Levy/assets/osmic/tourism/information-14.svg"
            },
            {
                "name": "Museum",
                "icon": "qrc:/themes/Levy/assets/osmic/tourism/museum-14.svg"
            }
        ]
    }

    ListView {
        id:pois
        height: parent.height
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 1
        anchors.leftMargin: 1
        spacing: 10
        clip: true
        orientation: ListView.Horizontal
        anchors.left: parent.left
        anchors.right: parent.right

        ScrollBar.horizontal: ScrollBar {
            id:poisScrollBar
            parent: pois.parent
            anchors.left: pois.left
            anchors.right: pois.right
            anchors.bottom: pois.bottom
            height: pois.height * 0.15
            active: true
            contentItem: Rectangle {
                radius: height / 2
                border.width: 1

            }
            background:Rectangle {
                height: 1
                color: "#000000"
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width * 0.98
            }
        }
        delegate: Rectangle {
            id: poiDelegate
            width: pois.height * 0.7
            height: width
            color: "#ffffff"
            radius: width / 2
            Layout.fillHeight: true
            Layout.fillWidth: true
            border.width: 1

            Image {
                id: image
                width: parent.width * 0.4
                height: parent.height * 0.4
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                fillMode: Image.PreserveAspectFit
                source: modelData.icon
                sourceSize.width: width
                sourceSize.height: height
            }
        }
        model:navitPOIs.poiTypeList
    }


}


/*##^## Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
 ##^##*/

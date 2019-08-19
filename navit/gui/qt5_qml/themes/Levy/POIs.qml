import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3


Item {
    id: __root
    signal poiClicked(var action)
    QtObject {
        id:navitPOIs
        property var poiTypeList : [
            {
                "name": "Petrol Station",
                "icon": "qrc:/themes/Levy/assets/osmic/transport/fuel-14.svg",
                "action" : "poi_fuel"
            },
            {
                "name": "Car Park",
                "icon": "qrc:/themes/Levy/assets/osmic/transport/parking-car-14.svg",
                "action" : "poi_car_parking"
            },
            {
                "name": "Hotel",
                "icon": "qrc:/themes/Levy/assets/osmic/accommodation/hotel-14.svg",
                "action" : "poi_hotel"
            },
            {
                "name": "Restaurant",
                "icon": "qrc:/themes/Levy/assets/osmic/eat-drink/restaurant-14.svg",
                "action" : "poi_restaurant"
            },
            {
                "name": "Others",
                "icon": "qrc:/themes/Levy/assets/ionicons/md-more.svg",
                "action" : "others"
            }
        ]
    }

    RowLayout {
        id:pois
        height: parent.height
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 1
        anchors.leftMargin: 1
        spacing: width * 0.02
        clip: true
        anchors.left: parent.left
        anchors.right: parent.right
        Repeater {
            model: navitPOIs.poiTypeList
            Item {
                id: element
                Layout.fillHeight: true
                Layout.fillWidth: true
                Rectangle {
                    id: poiDelegate
                    height: parent.width > parent.height ? parent.height : parent.width
                    width: parent.width > parent.height ? parent.height : parent.width
                    color: "#ffffff"
                    radius: width / 2
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
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
                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        __root.poiClicked(modelData.action)
                    }
                }
            }
        }
    }

//    ListView {
//        id:pois
//        height: parent.height
//        anchors.verticalCenter: parent.verticalCenter
//        anchors.rightMargin: 1
//        anchors.leftMargin: 1
//        spacing: 10
//        clip: true
//        orientation: ListView.Horizontal
//        anchors.left: parent.left
//        anchors.right: parent.right

//        ScrollBar.horizontal: ScrollBar {
//            id:poisScrollBar
//            parent: pois.parent
//            anchors.left: pois.left
//            anchors.right: pois.right
//            anchors.bottom: pois.bottom
//            height: pois.height * 0.15
//            active: true
//            contentItem: Rectangle {
//                radius: height / 2
//                border.width: 1

//            }
//            background:Rectangle {
//                height: 1
//                color: "#000000"
//                anchors.verticalCenter: parent.verticalCenter
//                anchors.horizontalCenter: parent.horizontalCenter
//                width: parent.width * 0.98
//            }
//        }
//        delegate: Rectangle {
//            id: poiDelegate
//            width: pois.height * 0.7
//            height: width
//            color: "#ffffff"
//            radius: width / 2
//            Layout.fillHeight: true
//            Layout.fillWidth: true
//            border.width: 1

//            Image {
//                id: image
//                width: parent.width * 0.4
//                height: parent.height * 0.4
//                anchors.horizontalCenter: parent.horizontalCenter
//                anchors.verticalCenter: parent.verticalCenter
//                fillMode: Image.PreserveAspectFit
//                source: modelData.icon
//                sourceSize.width: width
//                sourceSize.height: height
//            }
//        }
//        model:navitPOIs.poiTypeList
//    }


}










/*##^## Designer {
    D{i:0;autoSize:true;height:40;width:640}
}
 ##^##*/

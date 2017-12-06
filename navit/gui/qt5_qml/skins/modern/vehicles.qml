import QtQuick 2.0

ListView {
    id: list
    model: backend.vehicles
    delegate: Item {
        height: 64
        width: parent.width;
        Rectangle {
            color: "#1e1b18"
            height: parent.height;
            width: parent.width;
            border.width: 1

            Image {
                id: image1
                height: parent.height - 4;
                source : "icons/appbar.transit.car.svg"
                opacity: active ? 1 : 0.1
            }

            Text {
                text: name
                color: "#ffffff"
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: image1.right
                anchors.leftMargin: 8
            }

            MouseArea {
                id: mouse_area1
                z: 1
                hoverEnabled: false
                anchors.fill: parent

                onClicked:{
                    list.currentIndex = index
                    backend.setCurrentVehicle(index);
                    menucontent.source = "vehicle.qml"
                }
            }
        }
    }

    Component.onCompleted: backend.get_vehicles()
}

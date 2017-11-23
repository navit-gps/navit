import QtQuick 2.0
import QtQuick.Layouts 1.0


GridLayout {
    id: gridLayout
    height: 200
    columnSpacing: 4
    rowSpacing: 4
    rows: 3
    columns: 2

    MainButton {
        text: "Maps"
        icon: "icons/appbar.layer.svg"
        Layout.fillHeight: true
        Layout.fillWidth: true
        onClicked: {
            menucontent.source = "maps.qml"
        }
    }
    MainButton {
        text: "Vehicles"
        icon: "icons/appbar.transit.car.svg"
        Layout.fillHeight: true
        Layout.fillWidth: true
        onClicked: {
            menucontent.source = "vehicles.qml"
        }
    }
    MainButton {
        text: "Display"
        icon: "icons/appbar.fullscreen.box.svg"
        Layout.fillHeight: true
        Layout.fillWidth: true
    }

    MainButton {
        text: "Rules"
        icon: "icons/appbar.cogs.svg"
        Layout.fillHeight: true
        Layout.fillWidth: true
    }
}

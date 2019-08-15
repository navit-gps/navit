import QtQuick 2.0
import QtQuick.Layouts 1.0

GridLayout {
    id: gridLayout
    anchors.fill: parent
    columnSpacing: 4
    rowSpacing: 4
    rows: 4
    columns: 1

    MainButton {
        text: "Address lookup"
        Layout.fillHeight: true
        Layout.fillWidth: true
        icon: "icons/appbar.city.svg"
        onClicked: {
            menucontent.source = "destination_address.qml"
        }
    }

    MainButton {
        text: "Go Home"
        Layout.fillHeight: true
        Layout.fillWidth: true
    }

    MainButton {
        text: "Bookmarks"
        Layout.fillHeight: true
        Layout.fillWidth: true
        icon: "icons/appbar.location.svg"
        onClicked: {
            menucontent.source = "bookmarks.qml"
        }
    }

    MainButton {
        text: "Previous destinations"
        Layout.fillHeight: true
        Layout.fillWidth: true
        icon: "icons/appbar.timer.rewind.svg"
    }

}

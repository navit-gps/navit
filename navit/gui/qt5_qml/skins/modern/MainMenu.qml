import QtQuick 2.2
import QtQuick.Layouts 1.0

GridLayout {
    id: gridLayout
    anchors.fill: parent
    columnSpacing: 4
    rowSpacing: 4
    rows: 3
    columns: 2

    MainButton {
        text: "Where To?"
        Layout.fillHeight: true
        Layout.fillWidth: true
        icon: "icons/appbar.rocket.svg"
        onClicked: {
            menucontent.source = "destination.qml"
        }
    }

    MainButton {
        text: "My position"
        Layout.fillHeight: true
        Layout.fillWidth: true
        icon: "icons/appbar.location.checkin.svg"
    }

    MainButton {
        text: "Around Me"
        Layout.fillHeight: true
        Layout.fillWidth: true
        icon: "icons/appbar.information.circle.svg"
        onClicked: {
            menucontent.source = "pois.qml"
        }
    }

    MainButton {
        text: "Settings"
        Layout.fillHeight: true
        Layout.fillWidth: true
        icon: "icons/appbar.cog.svg"
        onClicked: {
            menucontent.source = "settings.qml"
        }
    }

    MainButton {
        text: "My route"
        Layout.fillHeight: true
        Layout.fillWidth: true
        icon: "icons/appbar.book.list.svg"
    }

    MainButton {
        text: "Quit"
        Layout.fillHeight: true
        Layout.fillWidth: true
        icon: "icons/appbar.power.svg"
        onClicked: Qt.quit()
    }

}

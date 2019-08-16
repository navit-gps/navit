import QtQuick 2.0
import QtQuick.Layouts 1.0

GridLayout {
    id: gridLayout
    anchors.fill: parent
    columnSpacing: 4
    rowSpacing: 4
    rows: 3
    columns: 1

    MainButton {
        id: townSearchButton
        height: 100
        text: backend.currentTown
        Layout.fillWidth: true
        onClicked: {
            backend.setSearchContext('town')
            menucontent.source = "search.qml"
        }
    }

    MainButton {
        id: streetSearchButton
        height: 100
        text: backend.currentStreet
        Layout.fillWidth: true
        onClicked: {
            backend.setSearchContext('street')
            menucontent.source = "search.qml"
        }
    }

    MainButton {
        id: countrySearchButton
        height: 100
        text: backend.currentCountry
        icon: backend.get_icon_path() + 'country_' + backend.currentCountryIso2 + ".svg"
        Layout.fillWidth: true
        onClicked: {
            backend.setSearchContext('country')
            menucontent.source = "search.qml"
        }
    }
}

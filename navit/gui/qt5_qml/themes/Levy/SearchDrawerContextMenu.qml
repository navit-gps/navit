import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3


Menu {
    id: __root
    signal itemClicked(var action)
    property alias setDestination : setDestination.visible
    property alias addStop : addStop.visible
    property alias setPosition : setPosition.visible
    property alias addBookmark : addBookmark.visible
    property alias pois : pois.visible
    property alias view : view.visible
    property alias showResults : showResults.visible
    MenuItem {
        id: setDestination
        text: "Set as destination"
        onClicked: __root.itemClicked("setDestination")
    }
    MenuItem {
        id: addStop
        text: "Add a stop"
        onClicked: __root.itemClicked("addStop")
    }
    MenuItem {
        id: setPosition
        text: "Set as position"
        onClicked: __root.itemClicked("setPosition")
    }
    MenuItem {
        id: addBookmark
        visible: true
        text: "Add as bookmark"
        onClicked: __root.itemClicked("addBookmark")
    }
    MenuItem {
        id: pois
        text: "POIs"
        onClicked: __root.itemClicked("pois")
    }
    MenuItem {
        id: view
        text: "View on Map"
        onClicked: __root.itemClicked("view")
    }
    MenuItem {
        id: showResults
        text: "Show results on Map"
        onClicked: __root.itemClicked("showResults")
    }
}

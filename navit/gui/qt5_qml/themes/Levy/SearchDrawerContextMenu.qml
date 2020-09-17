import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3


Menu {
    id: __root
    signal itemClicked(var action)
    property alias addBookmarkVisible : addBookmark.visible
    MenuItem {
        text: "Set as destination"
        onClicked: __root.itemClicked("setDestination")
    }
    MenuItem {
        text: "Add a stop"
        onClicked: __root.itemClicked("addStop")
    }
    MenuItem {
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
        text: "POIs"
        onClicked: __root.itemClicked("pois")
    }
}

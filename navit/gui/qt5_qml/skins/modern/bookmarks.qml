import QtQuick 2.0

Item {
    id: bookmarklist
    ListView {
        anchors.fill: parent
        id: listView
        model: backend.bookmarks
        delegate: Item {
            height: 64
            width: parent.width;
            Rectangle {
                color: "#1e1b18"
                height: parent.height;
                width: parent.width;
                border.width: 1

                Text {
                    text: name
                    color: "#ffffff"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: distanceText.right
                    anchors.leftMargin: 8
                }

                MouseArea {
                    id: mouse_area1
                    z: 1
                    hoverEnabled: false
                    anchors.fill: parent

                    onClicked:{
                        listView.currentIndex = index
			backend.setCurrentBookmark(index);
                        menucontent.source = "bookmark.qml"
                    }
                }
            }

        }

        Component.onCompleted: backend.get_bookmarks()
    }

    Rectangle {
        height: 64
        width: height

        x: parent.width - width
        y: parent.height - height * 2

        color: "#35322f"

        Image {
            anchors.centerIn: parent
            source: "icons/appbar.chevron.up.svg"
        }

        MouseArea {
            anchors.fill: parent
            id: listUp

            SmoothedAnimation {
                target: listView
                property: "contentY"
                running: listUp.pressed
                velocity: 1000
                to: 0
            }
            onReleased: {
                if (!listView.atYBeginning)
                    listView.flick(0, 1000)
            }
        }
    }

    Rectangle {
        height: 64
        width: height

        x: parent.width - width
        y: parent.height - height

        color: "#35322f"

        Image {
            anchors.centerIn: parent
            source: "icons/appbar.chevron.down.svg"
        }

        MouseArea {
            anchors.fill: parent
            id: listDown

            SmoothedAnimation {
                target: listView
                property: "contentY"
                running: listDown.pressed
                to: listView.contentHeight - listView.height
                velocity: 1000
            }
            onReleased: {
                if (!listView.atYEnd)
                    listView.flick(0, -1000)
            }
        }
    }
}

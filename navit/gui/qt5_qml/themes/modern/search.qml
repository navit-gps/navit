import QtQuick 2.0

Item {
    Rectangle {
        x : 16
        y : 64
        width: parent.width-32
        height: parent.height-64
        id: searchResults
	color: "#35322f"

        ListView {
            anchors.fill: parent
            id: listView
            model: backend.searchresults
            delegate: Loader { source: "search_result_item.qml"}

            Component.onCompleted: {
                backend.updateSearch("")
            }

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

    Rectangle {
        id: rectangle
        x: 16
        y: 16
        width: parent.width-32
        height: 50
        color: "#35322f"
        radius: 3
        border.width: 1

        Image {
            id: image
            x: 16
            y: 2
            width: 64
            height: 48
            fillMode: Image.PreserveAspectFit
            source: backend.get_country_icon("")
        }


        TextEdit {
            id: textEdit
            y: 8
            height: 20
            color: "#ffffff"
            text: qsTr("")
            focus: true
            anchors.rightMargin: 8
            anchors.verticalCenterOffset: 0
            anchors.leftMargin: 8
            horizontalAlignment: Text.AlignLeft
            anchors.verticalCenter: parent.verticalCenter
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.left: image.right
            font.pixelSize: 24
            onTextChanged: {
                backend.updateSearch(textEdit.text)
            }
        }
    }

}

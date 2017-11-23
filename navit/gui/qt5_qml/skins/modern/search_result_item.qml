import QtQuick 2.0


Item {
    height: 64
    width: searchResults.width;
    Rectangle {
        color: "#1e1b18"
        height: parent.height;
        width: parent.width;

        Image {
            id: image1
            height: 36;
            width: 64
            source : icon
            fillMode: Image.PreserveAspectFit
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
                listView.currentIndex = index
                backend.searchValidateResult(index)
                //                            backend.setActivePoi(index);
                //                            menucontent.source = "poi.qml"
            }
        }
    }
}


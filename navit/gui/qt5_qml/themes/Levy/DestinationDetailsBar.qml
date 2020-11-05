import QtQuick 2.9
import QtQuick.Layouts 1.3

Item {
    id: __root
    property alias address: addressText.text

    signal routeOverviewClicked ()
    signal poisClicked()
    signal addBookmarkClicked()
    Rectangle{
        id: buttonsBackground
        height: parent.height * 0.65
        anchors.right: parent.right
        anchors.left: parent.left
        border.width: 1
        radius: height / 2
        anchors.top: parent.top
        visible: true

        Text {
            id:addressText
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            Layout.fillHeight: true
            Layout.fillWidth: true
            font.pointSize: 18
        }
    }


    RowLayout {
        id: rowLayout
        height: parent.height * 0.25
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.leftMargin: 0
        spacing: 0
        Rectangle {
            color: "#ffffff"
            radius: height/2
            Layout.fillHeight: true
            Layout.fillWidth: true
            border.width: 1

            Text {
                text: qsTr("Route Overview")
                font.pointSize: 16
                fontSizeMode: Text.HorizontalFit
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
            }
            MouseArea {
                anchors.fill: parent
                onClicked: __root.routeOverviewClicked()
            }
        }

        Rectangle {
            id: rectangle1
            color: "#ffffff"
            radius: height/2
            Layout.fillHeight: true
            Layout.fillWidth: true
            border.width: 1
            Text {
                text: qsTr("POIs")
                anchors.verticalCenter: parent.verticalCenter
                font.pointSize: 16
                anchors.horizontalCenter: parent.horizontalCenter
                fontSizeMode: Text.HorizontalFit
            }
            MouseArea {
                anchors.fill: parent
                onClicked: __root.poisClicked()
            }
        }

        Rectangle {
            id: rectangle
            color: "#ffffff"
            radius: height/2
            Layout.fillHeight: true
            Layout.fillWidth: true
            border.width: 1

            Text {
                text: qsTr("Add as Bookmark")
                fontSizeMode: Text.HorizontalFit
                anchors.horizontalCenter: parent.horizontalCenter
                font.pointSize: 16
                anchors.verticalCenter: parent.verticalCenter
            }
            MouseArea {
                anchors.fill: parent
                onClicked: __root.addBookmarkClicked()
            }
        }
    }
}














/*##^## Designer {
    D{i:0;autoSize:true;height:240;width:1200}D{i:1;anchors_height:240}
}
 ##^##*/

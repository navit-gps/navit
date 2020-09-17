import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

import Navit 1.0
import Navit.Recents 1.0
import Navit.Favourites 1.0

Item {
    id: __root
    property int boxRadius : 10

    signal menuItemClicked(var index)
    TabBar {
        id: tabBar
        height: parent.height * 0.1
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        background: Rectangle {
            color: "#00ffffff"
            }

        TabButton {
            id: tabButton
            text: qsTr("Recent")
            font.pointSize: 12
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.bottom: parent.bottom
            background:  Rectangle {
                color: tabBar.currentIndex === 0 ? "#ffffff" : "#a4a4a4"
                implicitWidth: 100
                implicitHeight: 40
                radius: __root.boxRadius
                anchors.left: parent.left
                anchors.right: parent.right
                height: parent.height + radius
            }
            onCheckedChanged: {
                if(checked){
                    if(locationListLoader !== null) {
                        locationListLoader.sourceComponent = recentsList
                    }
                }
            }
        }

        TabButton {
            id: tabButton1
            text: qsTr("Favourites")
            font.pointSize: 12
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.bottom: parent.bottom
            background: Rectangle {
                color: tabBar.currentIndex === 1 ? "#ffffff" : "#a4a4a4"
                implicitWidth: 100
                implicitHeight: 40
                radius: __root.boxRadius
                anchors.left: parent.left
                anchors.right: parent.right
                height: parent.height + radius
            }
            onCheckedChanged: {
                if(checked){
                    if(typeof(locationListLoader.status) != "null") {
                        locationListLoader.sourceComponent = favouritesList
                    }

                }
            }
        }
    }

    Item {
        clip: true
        anchors.top: tabBar.bottom
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        Rectangle {
            color: "#ffffff"
            radius: __root.boxRadius
            anchors.fill: parent

            Rectangle {
                x: tabBar.currentIndex === 1 ? width : 0
                width: parent.width / 2
                color: "#ffffff"
                anchors.bottomMargin: parent.radius
                anchors.bottom: parent.bottom
                anchors.top: parent.top
                anchors.topMargin: 0
            }
        }
        Loader {
            id:locationListLoader
            anchors.fill: parent
            anchors.topMargin: parent.height * 0.05
            anchors.bottomMargin: parent.height * 0.05
            anchors.leftMargin: parent.width * 0.05
            anchors.rightMargin: parent.width * 0.05
            sourceComponent: recentsList
        }
    }

    Component {
        id:recentsList
        SearchDrawerRecentsList {
        }
    }

    Component {
        id:favouritesList
        SearchDrawerFavouritesList {
        }
    }
}



/*##^## Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
 ##^##*/

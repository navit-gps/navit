import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

import Navit 1.0
import Navit.Recents 1.0
ListView {
    id:__root
    clip: true
    model: NavitRecentsModel {
        id:navitRecentsModel
        navit: Navit
    }

    anchors.fill: parent
    delegate: Item {
        id: element
        height: 40
        width: __root.width

        Text {
            y: 0
            text: model.label
            anchors.left: parent.left
            font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent

            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: {
                if (mouse.button === Qt.LeftButton)
                    navitRecentsModel.setAsDestination(index)
                else if (mouse.button === Qt.RightButton)
                    contextMenu.popup()
            }
            onPressAndHold: {
                if (mouse.source === Qt.MouseEventNotSynthesized)
                    contextMenu.popup()
            }
            SearchDrawerContextMenu {
                id: contextMenu
                onItemClicked: {
                    switch(action){
                    case "addBookmark":
                        navitRecentsModel.addAsBookmark(index)
                        break;
                    case "pois":
                        break;
                    case "setPosition":
                        navitRecentsModel.setAsPosition(index)
                        break;
                    case "addStop":
                        navitRecentsModel.addStop(index, 0)
                        break;
                    case "setDestination":
                        navitRecentsModel.setAsDestination(index)
                        break;
                    }
                }
            }
        }
    }
}

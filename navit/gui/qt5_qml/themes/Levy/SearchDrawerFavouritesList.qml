import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

import Navit 1.0
import Navit.Favourites 1.0

ListView {
    id:__root

    clip: true

    Component.onCompleted: {
        console.log("Favourites loaded");
        //        navitFavouritesModel.showFavourites();
    }

    model: NavitFavouritesModel {
        id:navitFavouritesModel
        navit: Navit
    }

    anchors.fill: parent
    delegate: Item {
        id: element
        height: 40
        width: parent.width

        Text {
            y: 0
            text: model.label
            anchors.left: parent.left
            font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }

        MouseArea {
            id: mouseArea
            anchors.fill : parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: {
                if (mouse.button === Qt.LeftButton)
                    navitFavouritesModel.setAsDestination(index)
                else if (mouse.button === Qt.RightButton)
                    contextMenu.popup()
            }
            onPressAndHold: {
                if (mouse.source === Qt.MouseEventNotSynthesized)
                    contextMenu.popup()
            }
            SearchDrawerContextMenu {
                id: contextMenu
                addBookmark: false
                onItemClicked: {
                    switch(action){
                    case "addBookmark":
                        navitFavouritesModel.addAsBookmark(index)
                        break;
                    case "pois":
                        break;
                    case "setPosition":
                        navitFavouritesModel.setAsPosition(index)
                        break;
                    case "addStop":
                        navitFavouritesModel.addStop(index, 0)
                        break;
                    case "setDestination":
                        navitFavouritesModel.setAsDestination(index)
                        break;
                    }
                }

                MenuItem {
                    text: "Delete"
                }
            }
        }
    }
}

import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import Navit 1.0
import Navit.Layouts 1.0
import Navit.Layers 1.0
import Navit.Vehicles 1.0
import Navit.Maps 1.0

Item {
    id: __root
    clip: true
    signal closeMenu()
    signal routeOverview()
    signal cancelRoute()

    onStateChanged: {
        if(state == "searchOpen"){
            stackView.push(searchResultsComponents)
        } else {
            stackView.pop()
        }
    }
    onCloseMenu: {
        close()
    }

    function close(){
        while(menuStack.depth > 1){
            menuStack.pop();
        }
    }

    function menuItemClicked(name, action){
        switch(action){
        case "":
            break;
        case "cancelRoute":
            __root.cancelRoute()
            break;
        case "routeOverview":
            __root.routeOverview()
            break;
        case "routeDetails":
            break;
        case "heightProfile":
            break;
        case "mapRules":
            menuStack.push(menuListView, {"menuName": name, "model":mapRules})
            break;
        case "display":
            menuStack.push(menuListView, {"menuName": name, "model":menuDisplaySettingsModel})
            break;
        case "settings":
            menuStack.push(menuListView, {"menuName": name, "model":menuSettingsModel})
            break;
        case "layouts":
            menuStack.push(menuListView, {"menuName": name, "model":layoutsModel})
            break;
        case "setLayout":
            layoutsModel.setLayout(name);
            menuStack.pop();
            break;
        case "layers":
            menuStack.push(menuListView, {"menuName": name, "model":layersModel})
            break;
        case "toggleLayer":
            layersModel.toggleLayer(name);
            break;
        case "vehicles":
            vehiclesModel.update();
            menuStack.push(menuListView, {"menuName": name, "model":vehiclesModel});
            break;
        case "setVehicle":
            vehiclesModel.setVehicle(name);
            menuStack.pop();
            break;
        case "maps":
            mapsModel.update();
            menuStack.push(menuListView, {"menuName": name, "model":mapsModel});
            break;
        case "toggleMap":
            mapsModel.toggleMap(name);
        break;
        }
    }



    Component {
        id:menuListView
        Item {
            id: menuListViewComponent
            property alias model: listView.model
            property string menuName : ""
            Item {
                id: header
                height: parent.height * 0.15
                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.right: parent.right

                Image {
                    id: backButton
                    visible: menuListViewComponent.StackView.index
                    width: height
                    height: parent.height * 0.8
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    source: "qrc:/themes/Levy/assets/ionicons/md-arrow-back.svg"
                    mipmap: true

                    MouseArea {
                        anchors.fill: parent
                        onClicked: menuStack.pop()
                    }
                }

                Text {
                    text: qsTr(menuListViewComponent.menuName)
                    anchors.leftMargin: backButton.width / 2
                    anchors.left: backButton.right
                    anchors.rightMargin: closeButton.width / 2
                    anchors.right: closeButton.left
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 12
                }

                Image {
                    id: closeButton
                    width: height
                    height: parent.height * 0.8
                    anchors.right: parent.right
                    MouseArea {
                        anchors.fill: parent
                        onClicked: __root.closeMenu()
                    }
                    source: "qrc:/themes/Levy/assets/ionicons/md-close.svg"
                    anchors.verticalCenter: parent.verticalCenter
                    mipmap: true
                }
            }

            ListView {
                id: listView
                anchors.topMargin: 1
                anchors.top: header.bottom
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                model:menuModel
                delegate: Item {
                    id: item1
                    width: 80
                    height: 40


                    Text {
                        y: 0
                        color: "#ffffff"
                        text: name
                        anchors.leftMargin: itemImage.width / 2
                        anchors.left: itemImage.right
                        font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                    }


                    Image {
                        id: itemImage
                        width: height
                        height: parent.height * 0.6
                        anchors.left: parent.left
                        anchors.leftMargin: 0
                        anchors.verticalCenter: parent.verticalCenter
                        source: imageUrl
                        mipmap: true
                    }
                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        onClicked: __root.menuItemClicked(name, action)
                    }
                }
            }

        }

    }

    ListModel {
        id:menuModel
        ListElement {
            name: "Cancel Route"
            action: "cancelRoute"
            imageUrl: "qrc:/themes/Levy/assets/ionicons/md-close-circle-outline.svg"
        }

        ListElement {
            name: "Show Route"
            action: "routeOverview"
            imageUrl: ""
        }

        ListElement {
            name: "Route Details"
            action: "routeDetails"
            imageUrl: ""
        }

//        ListElement {
//            name: "Height Profile"
//            action: "heightProfile"
//            imageUrl: ""
//        }

        ListElement {
            name: "Map Rules"
            action: "mapRules"
            imageUrl: "qrc:/themes/Levy/assets/ionicons/md-map.svg"
        }

        ListElement {
            name: "Display Settings"
            action: "display"
            imageUrl: "qrc:/themes/Levy/assets/ionicons/md-desktop.svg"
        }
        ListElement {
            name: "Settings"
            action: "settings"
            imageUrl: "qrc:/themes/Levy/assets/ionicons/md-settings.svg"
        }
    }
    ListModel {
        id:mapRules
        ListElement {
            name: "Lock on Road"
            action: ""
            imageUrl: ""
        }
        ListElement {
            name: "Map follows Vehicle"
            action: ""
            imageUrl: ""
        }
        ListElement {
            name: "Plan with Waypoints"
            action: ""
            imageUrl: ""
        }
    }
    NavitLayouts {
        id:layoutsModel
        navit:Navit
    }

    NavitLayers {
        id:layersModel
        layout:layoutsModel
    }

    NavitVehicles {
        id:vehiclesModel
        navit:Navit
    }

    NavitMaps {
        id:mapsModel
        navit:Navit
    }

    ListModel {
        id:menuSettingsModel

        ListElement {
            name: "Maps"
            action: "maps"
            imageUrl: ""
        }

        ListElement {
            name: "Vehicle"
            action: "vehicles"
            imageUrl: ""
        }
    }

    ListModel {
        id:menuDisplaySettingsModel
        ListElement {
            name: "Layout"
            action: "layouts"
            imageUrl: ""
        }

        ListElement {
            name: "Fullscreen"
            action: ""
            imageUrl: ""
        }

        ListElement {
            name: "Layers"
            action: "layers"
            imageUrl: ""
        }
    }

    Rectangle {
        id: rectangle
        color: "#ffffff"
        radius: 20
        anchors.fill: parent
    }

    Rectangle {
        id: contentRect
        color: "#a4a4a4"
        radius: 15
        clip: true
        anchors.topMargin: parent.height * 0.02
        anchors.bottomMargin: parent.height * 0.02
        anchors.rightMargin: parent.height * 0.02
        anchors.leftMargin: parent.height * 0.02
        anchors.fill: parent
        Item {
            id: contentWrapper
            clip: true
            anchors.rightMargin: parent.radius
            anchors.leftMargin: parent.radius
            anchors.bottomMargin: parent.radius
            anchors.topMargin: parent.radius
            anchors.fill: parent
            StackView {
                id:menuStack
                anchors.fill: parent
                initialItem: menuListView
            }
        }
    }
}





/*##^## Designer {
    D{i:0;autoSize:true;height:480;width:640}D{i:37;anchors_height:200;anchors_width:200}
}
 ##^##*/

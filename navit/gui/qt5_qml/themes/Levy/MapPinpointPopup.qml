import QtQuick 2.9
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3

import QtGraphicalEffects 1.0

Popup {
    id:__root
    property string verticalPosition: "top"
    property string horizontalPosition: "left"
    property int xOffset: __root.horizontalPosition == "left"? __root.width * 0.1 : 0
    property int yOffset: __root.verticalPosition == "top"? __root.width * 0.1 : 0
    signal menuItemClicked (var action)
    background: Item {
        x: xOffset
        y: yOffset
        width: parent.width - __root.width * 0.1
        height: parent.height - __root.width * 0.1
        DropShadow {
            anchors.fill: parent
            source: backgroundRectangle
            color: "#80000000"
            samples: 1 + radius * 2
            radius: 60
            spread: 0.2
        }
        Rectangle {
            id: backgroundRectangle
            color: "#ffffff"
            radius: width  * 0.025
            border.width: 0
            anchors.fill: parent
            Rectangle {

                color: "#ffffff"
                border.width: 0
                width: parent.width/2
                height: parent.height/2
                x:__root.horizontalPosition == "left"? 0 : parent.width/2
                y:__root.verticalPosition == "top"? 0 : parent.height/2
                Image {
                    rotation:  {
                        var rotation = 0;

                        if(__root.horizontalPosition == "right")
                            rotation += 90

                        if(__root.verticalPosition == "bottom")
                            rotation += 90

                        if(__root.verticalPosition == "bottom" && __root.horizontalPosition == "left")
                            rotation += 180

                        return rotation
                    }

                    x: {
                        var x = - width / 4;

                        if(__root.horizontalPosition == "right")
                            x += parent.width - width / 2;
                        return x;
                    }

                    y: {
                        var y = - height / 4;

                        if(__root.verticalPosition == "bottom")
                            y += parent.height - height / 2;
                        return y;
                    }
                    width: parent.width * 0.4
                    height: width
                    fillMode: Image.PreserveAspectFit
                    source: "qrc:/themes/Levy/assets/corner-arrow.svg"
                }
            }
        }
    }

    ColumnLayout {
        id: menuItems
        x: __root.xOffset
        y: __root.yOffset
        width: parent.width - __root.width * 0.1
        height: parent.height - __root.width * 0.1
        Repeater {
            id: menuElementsRepeater
            model: ListModel {
                ListElement {
                    name: "Set as destination"
                    action: "setDestination"
                }
                ListElement {
                    name: "Visit before.."
                    action: "setWaypoint"
                }
                ListElement {
                    name: "Set as position"
                    action: "setPosition"
                }
                ListElement {
                    name: "Add as bookmark"
                    action: "bookmark"
                }
                ListElement {
                    name: "POIs"
                    action: "pois"
                }
            }
            Item {
                id: element
                Layout.fillHeight: true
                Layout.fillWidth: true

                Text {
                    id:nameText
                    text: name
                    fontSizeMode: Text.Fit
                    font.pixelSize: (menuItems.height / menuElementsRepeater.count) * 0.45
                    anchors.verticalCenter: parent.verticalCenter
                    font.bold: true
                }

                Rectangle {
                    visible: menuElementsRepeater.count !== index + 1
                    height: 1
                    color: "#000000"
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.leftMargin: 0
                    anchors.right: parent.right
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        __root.menuItemClicked(action)
                    }
                }
            }
        }
    }
}



/*##^## Designer {
    D{i:0;autoSize:true;height:720;width:1280}D{i:2;anchors_width:80}D{i:1;anchors_height:200;anchors_width:200}
}
 ##^##*/

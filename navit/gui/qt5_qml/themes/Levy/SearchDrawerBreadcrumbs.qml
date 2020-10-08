import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

Item {
    id:__root

    property string country: "Country"
    property string town: "Town"
    property string street: ""
    property string house: ""

    signal countryClicked()
    signal townClicked()
    signal streetClicked()
    signal houseClicked()
    clip: true

    Rectangle {
        id: leftBlob
        width: height
        color: "#ffffff"
        radius: height/2
        anchors.top: parent.top
        anchors.topMargin: 0
        anchors.bottom: parent.bottom
    }

    Rectangle {
        id: rightBlob
        width: height
        color: "#ffffff"
        radius: height/2
        anchors.top: parent.top
        anchors.topMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.rightMargin: -width/2
        anchors.right: breadcrumbs.right

        Rectangle {
            width: parent.width/2
            height: parent.height
            color: "#ffffff"
        }
    }

    RowLayout {
        id: breadcrumbs
        width: parent.width - parent.height
        anchors.leftMargin: leftBlob.width / 2
        anchors.left: leftBlob.left
        anchors.bottom: parent.bottom
        anchors.top: parent.top
        spacing: 0

        Item {
            id: countryCrumb
            visible: true
            Layout.fillHeight: true
            Layout.fillWidth: true
            Rectangle {
                width: height
                height: parent.height
                color: "#ffffff"
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                transformOrigin: Item.Center
                rotation: 45
            }

            Rectangle {
                anchors.fill: parent
                anchors.rightMargin: height/2

                Text {
                    id: element3
                    text: __root.country
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 12
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        __root.countryClicked();
                    }
                }
            }

        }

        Item {
            id: townCrumb
            visible: (__root.town)
            Layout.fillHeight: true
            Layout.fillWidth: true
            Rectangle {
                y: -height
                width: height
                height: parent.height
                color: "#ffffff"
                transformOrigin: Item.BottomLeft
                rotation: 45
            }

            Rectangle {
                width: height
                height: parent.height
                color: "#ffffff"
                transformOrigin: Item.BottomLeft
                rotation: 45
            }

            Rectangle {
                width: height
                height: parent.height
                color: "#ffffff"
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                transformOrigin: Item.Center
                rotation: 45
            }

            Rectangle {
                anchors.fill: parent
                color: "#ffffff"
                anchors.leftMargin: height/2
                anchors.rightMargin: height/2

                Text {
                    id: element4
                    text: __root.town
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 12
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        __root.townClicked()
                    }
                }

            }
        }

        Item {
            id: streetCrumb
            visible: (__root.street)
            Layout.fillHeight: true
            Layout.fillWidth: true
            Rectangle {
                y: -height
                width: height
                height: parent.height
                color: "#ffffff"
                transformOrigin: Item.BottomLeft
                rotation: 45
            }

            Rectangle {
                width: height
                height: parent.height
                color: "#ffffff"
                transformOrigin: Item.BottomLeft
                rotation: 45
            }

            Rectangle {
                anchors.fill: parent
                color: "#ffffff"
                anchors.leftMargin: height/2

                Text {
                    id: element5
                    text: __root.street
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 12
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        __root.streetClicked();
                    }
                }
            }


        }

        Item {
            id: houseCrumb
            visible: false
            Layout.fillHeight: true
            Layout.fillWidth: true
            Rectangle {
                anchors.fill: parent
                color: "#ffffff"

                Text {
                    id: element6
                    text: qsTr("House")
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 12
                }
            }
        }
    }


}






/*##^## Designer {
    D{i:0;autoSize:true;height:50;width:640}
}
 ##^##*/

import QtQuick 2.9
import QtQuick.Layouts 1.3

ColumnLayout {
    id: __root

    signal zoomModeClicked()
    signal dimensionClicked()
    signal zoomInClicked()
    signal zoomOutClicked()
    signal compassClicked()

    property int pitch: 0
    property int orientation: 0
    property bool autoZoom : true

    Item {
        id: modes
        Layout.fillHeight: true
        Layout.fillWidth: true

        Rectangle {
            id: zoomMode
            height: width
            color: "#ffffff"
            radius: width /2
            anchors.verticalCenterOffset: - height * 0.6
            anchors.verticalCenter: parent.verticalCenter
            border.color: "#a6a6a6"
            border.width: 1
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.right: parent.right

            Item {
                id: element4
                anchors.fill: parent
                Text {
                    id: element5
                    text: __root.autoZoom ? "M" : "A"
                    font.pixelSize: parent.height * 0.4
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                MouseArea {
                    id: mouseArea2
                    anchors.fill: parent
                    onClicked : __root.zoomModeClicked()
                }
            }
        }

        Rectangle {
            id: dimension
            height: width
            color: "#ffffff"
            radius: width /2
            anchors.verticalCenterOffset: width * 0.6
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 0
            border.color: "#a6a6a6"
            border.width: 1

            Item {
                id: element6
                Text {
                    id: element7
                    text: __root.pitch === 0 ? "3D" : "2D"
                    font.pixelSize: parent.height * 0.4
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                MouseArea {
                    id: mouseArea3
                    anchors.fill: parent
                    onClicked : __root.dimensionClicked()
                }
                anchors.fill: parent
            }
        }
    }

    Item {
        id: zoom
        Layout.fillHeight: true
        Layout.fillWidth: true

        Rectangle {
            id: rectangle
            height: width * 2
            color: "#ffffff"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            border.width: 1
            border.color: "#a6a6a6"

            Item {
                id: element
                height: parent.height/2
                anchors.top: parent.top
                anchors.topMargin: 0
                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.right: parent.right

                Text {
                    id: element2
                    text: qsTr("+")
                    font.pixelSize: parent.height * 0.4
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    onClicked : __root.zoomInClicked()
                }
            }

            Rectangle {
                id: rectangle1
                height: 1
                color: "#a6a6a6"
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.right: parent.right
            }


            Item {
                id: element1
                height: parent.height/2
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 0

                Text {
                    id: element3
                    text: qsTr("-")
                    font.pixelSize: parent.height * 0.4
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                MouseArea {
                    id: mouseArea1
                    anchors.fill: parent
                    onClicked : __root.zoomOutClicked()
                }
            }
        }
    }

    Item{
        id: compass
        Layout.fillHeight: true
        Layout.fillWidth: true
        Image {
            id: image
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            fillMode: Image.PreserveAspectFit
            source: "assets/compass-fave.png"
        }

        Image {
            id: compassNeedle
            height: parent.width
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            fillMode: Image.PreserveAspectFit
            source: "assets/compass-arrow.png"
            transform: Rotation{
                origin.x:compassNeedle.width / 2
                origin.y:compassNeedle.height / 2
                angle:__root.orientation
            }
        }
        MouseArea {
            anchors.fill: parent
            onClicked: __root.compassClicked()
        }
    }
    states: [
        State {
            name: "routeOverview"

            PropertyChanges {
                target: dimension
                visible: false
            }

            PropertyChanges {
                target: zoomMode
                visible: false
            }
        }
    ]
}

import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

import Navit 1.0
import Navit.POI 1.0
import Navit.Recents 1.0

Item {
    id: __root
    clip: true
    signal openSearch()
    signal closeSearch()
    signal closeDrawer()
    signal routeOverview()

    function open() {
        stackView.clear()
        stackView.push(mainComponent)
        state = ""
    }

    Component.onCompleted: {
        //        backend.setSearchContext("town")
    }

    onStateChanged: {
        if(state == "searchOpen"){
            stackView.push(searchResultsComponents)
        } else if(state == ""){
            stackView.pop()
        }
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
            id: element
            anchors.fill: parent
            anchors.topMargin: parent.radius
            anchors.rightMargin: parent.radius
            anchors.leftMargin: parent.radius
            anchors.bottomMargin: parent.radius


            Item {
                id: contentWrapper
                anchors.topMargin: contentRect.radius
                clip: true
                anchors.top: headerContainer.bottom
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.left: parent.left

                POIs {
                    id: pois
                    height: __root.width > __root.height ? parent.height * 0.25 : parent.width * 0.25
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.left: parent.left
                    anchors.leftMargin: 0
                    onPoiClicked: {
                        if(action === "others"){
                            return;
                        }

                        navitPoiModel.search(__root.lng, __root.lat, action)
                        stackView.push(searchResultsComponents,{model:navitPoiModel})
                        __root.state = "poiView"
                    }
                }

                StackView {
                    id: stackView
                    anchors.topMargin: parent.height * 0.05
                    anchors.top: pois.bottom
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right

                    pushEnter: Transition {
                        PropertyAnimation {
                            property: "opacity"
                            from: 0
                            to:1
                            duration: 200
                        }
                    }
                    pushExit: Transition {
                        PropertyAnimation {
                            property: "opacity"
                            from: 1
                            to:0
                            duration: 200
                        }
                    }
                    popEnter: Transition {
                        PropertyAnimation {
                            property: "opacity"
                            from: 0
                            to:1
                            duration: 200
                        }
                    }
                    popExit: Transition {
                        PropertyAnimation {
                            property: "opacity"
                            from: 1
                            to:0
                            duration: 200
                        }
                    }
                }
            }

            Item {
                id: headerContainer
                height: __root.width > __root.height ? parent.height * 0.1 : parent.width * 0.1
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right

                Item {
                    id: searchWrapper
                    height: parent.height
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    clip: true
                    width: parent.width

                    Rectangle {
                        id: backButton
                        width: height
                        height: 0
                        color: "#ffffff"
                        radius: height / 2
                        anchors.verticalCenter: searchBar.verticalCenter

                        Image {
                            id: image1
                            width: height
                            height: parent.height * 0.8
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.verticalCenter: parent.verticalCenter
                            sourceSize.width: width
                            source: "assets/ionicons/md-arrow-back.svg"
                            sourceSize.height: height
                        }

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            onClicked: {
                                __root.state = ""
                                __root.closeSearch()
                            }
                        }
                    }

                    Rectangle {
                        id: searchBar
                        height: parent.height
                        color: "#ffffff"
                        radius: parent.height/2
                        anchors.rightMargin: 0
                        anchors.leftMargin: 0
                        anchors.left: backButton.right
                        anchors.right: closeButton.left
                        anchors.top: parent.top
                        anchors.topMargin: 0

                        TextField {
                            id: search
                            anchors.right: searchIcon.left
                            anchors.rightMargin: 0
                            placeholderText: "Enter destination"
                            font.pointSize: 12
                            anchors.top: parent.top
                            anchors.topMargin: 0
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 0
                            anchors.leftMargin: parent.height/4
                            anchors.left: parent.left
                            background: Item { }
                            onPressed: {
                                __root.state = "searchOpen"
                                __root.openSearch()
                            }
                            onTextChanged: {
                                backend.updateSearch(text)
                            }
                        }

                        Image {
                            id: searchIcon
                            width: height
                            height: parent.height * 0.8
                            anchors.verticalCenterOffset: 1
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.rightMargin: height/4
                            anchors.right: parent.right
                            source: "assets/ionicons/md-search.svg"
                            sourceSize.width: width
                            sourceSize.height: height
                        }
                    }

                    Rectangle {
                        id: closeButton
                        width: height
                        height: 0
                        color: "#ffffff"
                        radius: height / 2
                        anchors.verticalCenter: searchBar.verticalCenter
                        anchors.right: parent.right

                        Image {
                            id: image2
                            width: height
                            height: parent.height * 0.8
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.verticalCenter: parent.verticalCenter
                            sourceSize.width: width
                            source: "assets/ionicons/md-close.svg"
                            sourceSize.height: height
                        }

                        MouseArea {
                            id: mouseArea1
                            anchors.fill: parent
                            onClicked: {
                                __root.state = ""
                                __root.closeDrawer()
                            }
                        }
                    }
                }

                Item {
                    id: poisHeader
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    clip: true
                    visible: true



                    Rectangle {
                        id: rectangle
                        color: "#ffffff"
                        radius: height / 2
                        anchors.fill: parent

                        Text {
                            id: element1
                            text: qsTr("Parking")
                            font.pixelSize: parent.height * 0.7
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }


                    Rectangle {
                        id: backButton1
                        width: height
                        color: "#ffffff"
                        radius: height / 2
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        Image {
                            id: image3
                            width: height
                            height: parent.height * 0.8
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.verticalCenter: parent.verticalCenter
                            source: "assets/ionicons/md-arrow-back.svg"
                            sourceSize.width: width
                            sourceSize.height: height
                        }

                        MouseArea {
                            id: mouseArea2
                            anchors.fill: parent
                            onClicked: {
                                __root.state = ""
                                __root.closeSearch()
                            }
                        }
                    }

                    Rectangle {
                        id: closeButton1
                        width: height
                        color: "#ffffff"
                        radius: height / 2
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        Image {
                            id: image4
                            width: height
                            height: parent.height * 0.8
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.verticalCenter: parent.verticalCenter
                            source: "assets/ionicons/md-close.svg"
                            sourceSize.width: width
                            sourceSize.height: height
                        }

                        MouseArea {
                            id: mouseArea3
                            anchors.fill: parent
                            onClicked: {
                                __root.state = ""
                                __root.closeDrawer()
                            }
                        }
                    }
                }
            }

        }
    }
    NavitPOIModel {
        id:navitPoiModel
        navit: Navit
    }
    NavitRecentsModel {
        id:navitRecentsModel
        navit: Navit
    }

    Component{
        id:mainComponent
        SearchDrawerMain {
            width: stackView.width
            height: stackView.height
            boxRadius : contentRect.radius / 2
        }
    }

    Component{
        id:searchResultsComponents
        SearchDrawerSearchResults {
            onResultClicked: __root.routeOverview()
        }
    }
    states: [
        State {
            name: "searchOpen"

            PropertyChanges {
                target: backButton
                height: parent.height
            }

            PropertyChanges {
                target: closeButton
                height: parent.height
            }

            PropertyChanges {
                target: searchBar
                anchors.rightMargin: parent.height / 2
                anchors.leftMargin: parent.height / 2
            }

            PropertyChanges {
                target: pois
                height: 0
                visible: false
            }
        },
        State {
            name: "poiView"

            PropertyChanges {
                target: pois
                height: 0
            }

            PropertyChanges {
                target: poisHeader
                height: headerContainer.height
                width: headerContainer.width
            }

            PropertyChanges {
                target: searchWrapper
                width: 0
                height: 0
            }

            PropertyChanges {
                target: backButton1
                height: parent.height
            }

            PropertyChanges {
                target: closeButton1
                height: parent.height
            }
        }
    ]
    transitions: [
        Transition {
            NumberAnimation { property: "height"; easing.type: Easing.InOutQuad; }
            NumberAnimation { property: "width"; easing.type: Easing.InOutQuad; }
            NumberAnimation { property: "anchors.rightMargin"; easing.type: Easing.InOutQuad; }
            NumberAnimation { property: "anchors.leftMargin"; easing.type: Easing.InOutQuad; }
        }
    ]

}








/*##^## Designer {
    D{i:0;autoSize:true;height:720;width:1000}D{i:2;anchors_height:200;anchors_width:200}
D{i:37;anchors_height:100;anchors_width:100;anchors_x:"-591";anchors_y:"-25"}D{i:38;anchors_height:200;anchors_width:200}
}
 ##^##*/

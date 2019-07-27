import QtQuick 2.9
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import com.navit.graphics_qt5 1.0

Item {
    id: __root

    property string prevState: ""
    height: parent.height


    QtObject {
        id:navitMapControls
        property int mapDimension : 3
        property bool mapOverview : false

        function showRouteOverview() {
//            //navitView.source = "qrc:/themes/Levy/assets/destination-view.png"
//            mapArrow.visible = false
        }

        function hideRouteOverview() {
//            mapArrow.visible = true
//            if(mapDimension === 2) {
//                //navitView.source = "qrc:/themes/Levy/assets/2d-view.png"
//                mapArrow.source = "qrc:/themes/Levy/assets/arrow2d.png"
//            } else {
//                navitView.source = "qrc:/themes/Levy/assets/3d-view.png"
//                mapArrow.source = "qrc:/themes/Levy/assets/arrow3d.png"
//            }
        }

        function toggleMapDimension () {
//            mapArrow.visible = true
//            if(mapDimension === 2) {
//                //navitView.source = "qrc:/themes/Levy/assets/3d-view.png"
//                mapArrow.source = "qrc:/themes/Levy/assets/arrow3d.png"
//                mapDimension = 3
//            } else {
//                //navitView.source = "qrc:/themes/Levy/assets/2d-view.png"
//                mapArrow.source = "qrc:/themes/Levy/assets/arrow2d.png"
//                mapDimension = 2
//            }
        }

    }

//    Image {
//        id: navitView
//        fillMode: Image.PreserveAspectCrop
//        anchors.fill: parent
//        source: "qrc:/themes/Levy/assets/3d-view.png"

//        Image {
//            id: mapArrow
//            width: parent.width * 0.1
//            height: parent.height * 0.2
//            anchors.horizontalCenter: parent.horizontalCenter
//            anchors.verticalCenter: parent.verticalCenter
//            source: "qrc:/themes/Levy/assets/arrow3d.png"
//            fillMode: Image.PreserveAspectFit
//        }
//    }

    QNavitQuick {
        id: navit1
        width: parent.width
        height: parent.height
        // focus: true
        opacity: 0;
        Component.onCompleted: {
            console.log(width + "x" + height)
            navit1.setGraphicContext(graphics_qt5_context);
            navit1.opacity = 1;
        }
        Behavior on opacity {
            NumberAnimation {
                id: opacityAnimation;duration: 1000;alwaysRunToEnd: true
            }
        }
    }

    MouseArea {
        id: mouseArea4
        enabled: true
        anchors.fill: __root
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        propagateComposedEvents : true
        onClicked: {
            switch (mouse.button) {
            case Qt.LeftButton :

                if(__root.state != "mapControlsVisible"){
                    __root.prevState = __root.state
                    __root.state = "mapControlsVisible"
                } else {
                    __root.state = __root.prevState
                }
                break;
            case Qt.RightButton :
                pinpointPopup.mouseY = mouse.y
                pinpointPopup.mouseX = mouse.x
                pinpointPopup.open()
                break;

            }
        }
        onPressAndHold: {
            pinpointPopup.open()
        }
    }

    MapControls {
        id: mapControls
        x: -width
        width: parent.width > parent.height ? parent.height * 0.1 : parent.width * 0.1
        spacing: height * 0.02
        anchors.topMargin: parent.height * 0.05
        anchors.top: parent.top
        anchors.bottom: mapNavigationBar.top
        mapDimension : navitMapControls.mapDimension == 2 ? 3 : 2
        onDimensionClicked: {
            navitMapControls.toggleMapDimension()
        }
        onZoomInClicked: {

        }
        onZoomOutClicked: {

        }
        onZoomModeClicked: {

        }
    }


    MapNavigationBar {
        id: mapNavigationBar
        width: parent.width * 0.95 //parent.width > parent.height ? parent.width * 0.9 : parent.width
        height: parent.width > parent.height ?  parent.height * 0.15 : parent.width * 0.2
        clip: true
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: parent.height * 0.05
        anchors.bottom: parent.bottom
        onSearchButtonClicked: {
            __root.state = "searchDrawerOpen"
            searchDrawer.state = ""
        }
        onMenuButtonClicked: {
            __root.prevState = __root.state
            __root.state = "menuDrawerOpen"
        }
    }


    DestinationBar {
        id: destinationBar
        height: parent.width > parent.height ?  parent.height * 0.15 : parent.width * 0.2
        visible: false
        clip: true
        anchors.bottomMargin: parent.height * 0.05
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter

        onCancelButtonClicked: {
            navitMapControls.hideRouteOverview()
            __root.state = __root.prevState
        }
        onRouteDetailsClicked: {
        }
        onStartButtonClicked: {
            navitMapControls.hideRouteOverview()
            __root.state = ""
            mapNavigationBar.state = "navigationState"
            __root.prevState = ""
        }
    }

    Rectangle {
        id: rectangle
        color: "#00000000"
        visible: false
        anchors.fill: parent

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            onClicked: {
                __root.state = ""
            }
        }
    }

    MapPinpointPopup {
        id: pinpointPopup
        property real mouseX : 0
        property real mouseY : 0
        height: parent.height > parent.width ? parent.width * 0.6 : parent.height * 0.4
        width: parent.height > parent.width ? parent.width * 0.5  : parent.height * 0.4
        //        x: Math.round((parent.width - width) / 2)
        //        y: Math.round((parent.height - height) / 2)
        x: {
            if(mouseX + width > parent.width){
                horizontalPosition = "right"
                return mouseX - width
            } else {
                horizontalPosition = "left"
                return mouseX
            }
        }
        y: {
            if(mouseY + height > parent.height){
                verticalPosition = "bottom"
                return mouseY - height
            } else {
                verticalPosition = "top"
                return mouseY
            }
        }
    }

    SearchDrawer {
        id: searchDrawer
        x: - width
        y: 0
        width: parent.width < parent.height ? parent.width * 0.9 : parent.width/2
        height: parent.height
        visible: false
        onOpenSearch: {
            __root.state = "searchDrawerSearchActive"
        }
        onCloseSearch: {
            __root.state = "searchDrawerOpen"
        }
        onCloseDrawer: {
            __root.state = ""
            __root.prevState = ""
        }
        onRouteOverview: {
            __root.prevState = __root.state
            __root.state = "routeOverview"
            navitMapControls.showRouteOverview()
        }
    }



    MenuDrawer {
        id: menuDrawer
        x: parent. width
        y: 0
        width: parent.width < parent.height ? parent.width * 0.9 : parent.width/2
        height: parent.height
        visible: false
        onCloseMenu : {
            __root.state = __root.prevState
            __root.prevState = ""
        }
        onRouteOverview : {
            __root.state = __root.prevState
            __root.prevState = ""
            navitMapControls.showRouteOverview()
        }
        onCancelRoute : {
            __root.state = ""
            mapNavigationBar.state = ""
        }
    }


    states: [
        State {
            name: "mapControlsVisible"

            PropertyChanges {
                target: mapControls
                x: (parent.width * 0.025) + (mapNavigationBar.height / 2) - (width / 2)
            }
        },
        State {
            name: "searchDrawerOpen"
            when: mapNavigationBar.searchButtonClicked
            PropertyChanges {
                target: searchDrawer
                x: 0
                visible: true
            }

            PropertyChanges {
                target: rectangle
                color: "#a6000000"
                visible: true
            }
        },
        State {
            name: "searchDrawerSearchActive"
            PropertyChanges {
                target: searchDrawer
                x: 0
                width: parent.width
                visible: true
            }

            PropertyChanges {
                target: rectangle
                color: "#a6000000"
                visible: true
            }
        },
        State {
            name: "routeOverview"
            //            PropertyChanges {
            //                target: searchDrawer
            //                x: 0
            //                y: -height
            //                width: parent.width
            //                visible: true
            //            }

            PropertyChanges {
                target: rectangle
                color: "#00000000"
                visible: false
            }

            PropertyChanges {
                target: destinationBar
                width: parent.width > parent.height ? parent.height : parent.width
                visible: true
            }

            PropertyChanges {
                target: mapNavigationBar
                width: 0
            }
            PropertyChanges {
                target: mapControls
                x: (parent.width * 0.025) + (mapNavigationBar.height / 2) - (width / 2)
                state: "routeOverview"
            }
        },
        State {
            name: "menuDrawerOpen"

            PropertyChanges {
                target: menuDrawer
                x: parent.width - width
                visible: true
            }

            PropertyChanges {
                target: rectangle
                color: "#a6000000"
                visible: true
            }
        }
    ]

    transitions: [
        Transition {
            from: ""
            to: "searchDrawerOpen"
            reversible: true
            PropertyAction {
                target:searchDrawer
                property: "visible"
                value: true
            }
            PropertyAction {
                target:rectangle
                property: "visible"
                value: true
            }
            ParallelAnimation{
                ColorAnimation {
                    target:rectangle
                }
                NumberAnimation {
                    property: "x";
                    easing.type: Easing.InOutQuad;
                }
            }
        },
        Transition {
            to: "menuDrawerOpen"
            reversible: true
            SequentialAnimation {
                PropertyAction {
                    property: "visible"
                    value: true
                }
                ColorAnimation { }
            }
            NumberAnimation { property: "x"; easing.type: Easing.InOutQuad; }
            PropertyAnimation { property: "visible"; }
        },

        Transition {
            from:"searchDrawerSearchActive"
            to: ""
            NumberAnimation { property: "x"; easing.type: Easing.InOutQuad; }
            SequentialAnimation {
                ColorAnimation { }
                PropertyAction {
                    property: "visible"
                    value: true
                }
            }
        },
        Transition {
            from: "searchDrawerOpen"
            to: "searchDrawerSearchActive"
            reversible: true
            NumberAnimation { property: "width"; easing.type: Easing.InOutQuad; }
        },

        Transition {
            from: ""
            to: "mapControlsVisible"
            reversible: true
            NumberAnimation { property: "x"; easing.type: Easing.InOutQuad; }
        },

        //        Transition {
        //            from: "searchDrawerOpen"
        //            to: "routeOverview"
        //            SequentialAnimation {
        //                PropertyAction {
        //                    target: destinationBar
        //                    property: "visible"
        //                    value: true
        //                }
        //                ParallelAnimation {
        //                    ColorAnimation { }
        //                    NumberAnimation {
        //                        property: "width";
        //                        easing.type: Easing.InOutQuad;
        //                    }
        //                    NumberAnimation {
        //                        property: "x";
        //                        easing.type: Easing.InOutQuad;
        //                    }
        //                }

        //                PropertyAction {
        //                    property: "visible"
        //                    value: false
        //                }
        //            }
        //        },

        Transition {
            to: "routeOverview"
            reversible: true
            SequentialAnimation {
                PropertyAction {
                    property: "visible"
                    value: true
                }
                ParallelAnimation {
                    ColorAnimation {
                    }
                    NumberAnimation {
                        property: "width";
                        easing.type: Easing.InOutQuad;
                    }
                    NumberAnimation {
                        property: "x";
                        easing.type: Easing.InOutQuad;
                    }
                }
                PropertyAction {
                    property: "visible"
                    value: false
                }
            }
        }
    ]
}


































































/*##^## Designer {
    D{i:0;height:720;width:1280}D{i:4;anchors_x:196}D{i:5;anchors_width:200;anchors_x:196}
D{i:6;anchors_height:100;anchors_width:100}D{i:7;anchors_height:100;anchors_width:100}
D{i:9;anchors_y:109}D{i:8;anchors_y:109}D{i:19;anchors_height:200;anchors_width:200}
D{i:20;anchors_height:200;anchors_width:200}
}
 ##^##*/

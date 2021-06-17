import QtQuick 2.9
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import Navit 1.0
import Navit.Graphics 2.0
import Navit.Route 1.0

Item {
    id: __root

    property string prevState: ""
    height: parent.height

    onStateChanged: {
        if(state != "mapControlsVisible") {
            mapControlsTimer.stop()
        }
        console.log(state)
    }

    QtObject {
        id:navigation

        property int previous_pitch : 0
        property int previous_zoom : 0
        function routeOverview() {
            previous_pitch = navit1.pitch;
            __root.state = "routeOverview"
            navit1.pitch = 0;
            navit1.followVehicle = false;
            navit1.zoomToRoute();
            navit1.zoomOut(2);
        }

        function startNavigation() {
            __root.state = ""
            __root.prevState = ""
            mapNavigationBar.setNavigationState()

            navit1.pitch = 45;
            navit1.orientation = -1
            navit1.followVehicle = true
            navit1.autoZoom = true
            navit1.centerOnPosition()
        }

        function cancelNavigation() {
            __root.state = __root.prevState
            __root.prevState = ""
            navit1.followVehicle = true;
            navit1.pitch = 0;
        }
    }

    NavitRoute {
        id:navitRoute
        navit: Navit
        onDestinationAdded: {
            navigation.routeOverview();
            destinationDetailsBar.address = destination;
        }
        onNavigationFinished: {
            console.log("onNavigationFinished");
            mapNavigationBar.setNormalState()
        }
        onStatusChanged: {
            console.log(status)
            NavitRoute.Navigating
            if(status === NavitRoute.Recalculating || status === NavitRoute.Navigating ){
                console.log("showing navigation state")
                mapNavigationBar.setNavigationState()
            }
        }
    }

    NavitMap {
        id: navit1
        anchors.leftMargin: 0
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: parent.width
        navit:Navit

        property bool hasMoved : false

        onLeftButtonClicked: {
            if(__root.state == ""){
                mapControls.showControls()
            }
        }
        onRightButtonClicked: {
            navit1.followVehicle = false
            pinpointPopup.mouseY = position.y
            pinpointPopup.mouseX = position.x
            pinpointPopup.address = navit1.getAddress(position.x, position.y)
            pinpointPopup.open()
        }

        onPositionChanged: {
            mapControls.restartTimer()
            hasMoved = true
        }
    }

    Text {
        id:zoomLevel
        text: "Zoom Level : " + navit1.zoomLevel
    }
    Text {
        id: pitch
        text: "Pitch : " + navit1.pitch
        anchors.top: zoomLevel.bottom
    }
    Text {
        id: orientation
        text: "Orientation : " + navit1.orientation
        anchors.top: pitch.bottom
        MouseArea {
            anchors.fill:parent
            onClicked: navit1.orientation = !navit1.orientation
        }
    }
    Text {
        id: followVehcile
        text: "Follow Vehicle : " + navit1.followVehicle
        anchors.top: orientation.bottom
        MouseArea {
            anchors.fill:parent
            onClicked: navit1.followVehicle = !navit1.followVehicle
        }
    }
    Text {
        id: tracking
        text: "Tracking : " + navit1.tracking
        anchors.top: followVehcile.bottom
        MouseArea {
            anchors.fill:parent
            onClicked: navit1.tracking = !navit1.tracking
        }
    }
    Text {
        id: autozoom
        text: "Auto Zoom : " + navit1.autoZoom
        anchors.top: tracking.bottom
        MouseArea {
            anchors.fill:parent
            onClicked: navit1.autoZoom = !navit1.autoZoom
        }
    }

    Timer {
        id:mapControlsTimer
        interval: 10000; running: false; repeat: false
        onTriggered: {
            __root.state = __root.prevState
        }
    }

    MapControls {
        id: mapControls
        x: -width
        width: parent.width > parent.height ? parent.height * 0.1 : parent.width * 0.1
        anchors.topMargin: parent.height * 0.05
        spacing: height * 0.02
        anchors.top: parent.top
        anchors.bottom: mapNavigationBar.top
        pitch: navit1.pitch
        orientation: navit1.orientation
        autoZoom: navit1.autoZoom
        onDimensionClicked: {
            restartTimer()
            if(navit1.pitch != 0) {
                navit1.pitch = 0
            } else {
                navit1.pitch = 45
            }
        }
        onZoomInClicked: {
            restartTimer()
            navit1.zoomIn(2)
        }
        onZoomOutClicked: {
            restartTimer()
            navit1.zoomOut(2)

        }
        onZoomModeClicked: {
            restartTimer()
            navit1.autoZoom = !navit1.autoZoom
        }
        onCompassClicked: {
            navit1.orientation = -1
        }

        function showControls () {
            if(__root.state != "mapControlsVisible"){
                __root.prevState = __root.state
                __root.state = "mapControlsVisible"
            }
            mapControlsTimer.restart()
        }
        function restartTimer() {
            mapControlsTimer.restart()
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
        timeLeft: navitRoute.timeLeft
        distanceLeft: navitRoute.distance
        arrivalTime: navitRoute.arrivalTime

        onSearchButtonClicked: {
            __root.state = "searchDrawerOpen"
        }
        onMenuButtonClicked: {
            __root.prevState = __root.state
            __root.state = "menuDrawerOpen"
        }
    }

    Rectangle {
        id: currentStreet
        width: currenStreetText.width * 1.3
        height: parent.height * 0.05
        color: "#ffffff"
        radius: height/2
        anchors.verticalCenter: destinationBar.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        border.width: 1

        Text {
            id: currenStreetText
            text: navitRoute.currentStreet
            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment: Text.AlignHCenter
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: 24
        }
    }

    Instructions {
        id: instructions
        height: parent.height * 0.15
        visible: mapNavigationBar.state=="navigationState"
        anchors.rightMargin: parent.width * 0.025
        anchors.right: parent.right
        transformOrigin: Item.Right
        anchors.topMargin: parent.height * 0.025
        anchors.top: parent.top
        icon: navitRoute.nextTurnIcon
        distance: navitRoute.nextTurnDistance
        street: navitRoute.nextTurn
    }

    DestinationBar {
        id: destinationBar
        height: parent.width > parent.height ?  parent.height * 0.15 : parent.width * 0.2
        visible: false
        clip: true
        anchors.bottomMargin: parent.height * 0.05
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter

        timeLeft: navitRoute.timeLeft
        distanceLeft: navitRoute.distance
        arrivalTime: navitRoute.arrivalTime
        onCancelButtonClicked: {
            navigation.cancelNavigation();
        }
        onRouteDetailsClicked: {
        }
        onStartButtonClicked: {
            navigation.startNavigation();
        }
    }

    DestinationDetailsBar {
        id: destinationDetailsBar
        height: parent.width > parent.height ?  parent.height * 0.15 : parent.width * 0.25
        anchors.topMargin: parent.height * 0.05
        clip: true
        anchors.top: routeCalculating.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: 0
        visible: false
        onPoisClicked: {
            __root.state = "routeOverviewPOIs";
            searchDrawer.searchPOIs("");
        }
    }

    Rectangle {
        id: routeCalculating
        width: calculatingText.width + height
        height: parent.height * 0.05
        anchors.topMargin: parent.height * 0.05
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        visible: (navitRoute.status === NavitRoute.Calculating || navitRoute.status === NavitRoute.Recalculating )

        color: "#ffffff"
        radius: height/2
        border.width: 1

        Text {
            id: calculatingText
            text: qsTr("Calculating Route...")
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 24
        }
    }

    Item {
        id: recenterButton
        width: parent.width > parent.height ? parent.height * 0.1 : parent.width * 0.1
        height: width
        x: navit1.followVehicle? parent.width : parent.width - ((parent.width * 0.025) + (mapNavigationBar.height / 2)) - (width / 2)
        anchors.verticalCenter: parent.verticalCenter
        Behavior on x {
            NumberAnimation { duration: 200 }
        }

        Rectangle {
            id: rectangle1
            color: "#ffffff"
            radius: width/2
            border.width: 1
            anchors.fill: parent
        }

        Image {
            id: image
            width: parent.width * 0.8
            height: width
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            source: "qrc:/themes/Levy/assets/ionicons/md-locate.svg"
            fillMode: Image.PreserveAspectFit
            mipmap: true
        }

        MouseArea {
            id: mouseArea1
            width: 71
            anchors.fill: parent
            onClicked: {
                navit1.followVehicle = true
                navit1.autoZoom = true
                navit1.orientation = -1
                navit1.centerOnPosition()
            }
        }
    }

    SpeedGauge {
        id: speedGauge
        x: parent.width - ((parent.width * 0.025) + (mapNavigationBar.height / 2)) - (width / 2)
        width: parent.width > parent.height ? parent.height * 0.1 : parent.width * 0.1
        height: parent.width > parent.height ?  parent.height * 0.15 : parent.width * 0.2
        anchors.verticalCenterOffset: parent.height/4 - height/2
        anchors.verticalCenter: parent.verticalCenter
        speed: navitRoute.speed
        speedLimit: navitRoute.speedLimit
        speedUnit: "km/h"
    }

    Rectangle {
        id: overlay
        color: "#00000000"
        visible: false
        anchors.fill: navit1

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            preventStealing: true
            onClicked: {
                menuDrawer.close()
                searchDrawer.close()
                __root.state = ""
            }
        }
    }

    MapPinpointPopup {
        id: pinpointPopup
        property real mouseX : 0
        property real mouseY : 0
        property var coordinates : {"lng": 0.00, "lat": 0.00}
        height: parent.height > parent.width ? parent.width * 0.6 : parent.height * 0.4
        width: parent.height > parent.width ? parent.width * 0.5  : parent.height * 0.4
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
        onMenuItemClicked: {
            switch (action) {
            case "setDestination":
                navitRoute.setDestination(pinpointPopup.address, mouseX, mouseY)
                break;
            case "addStop":
                navitRoute.addStop(pinpointPopup.address, mouseX, mouseY, 0)
                break;
            case "setPosition":
                navitRoute.setPosition(mouseX, mouseY)
                break;
            case "bookmark":
                navit1.addBookmark(pinpointPopup.address, mouseX, mouseY);
                break;
            case "pois":
                break;
            }
            pinpointPopup.close()
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
    }

    MenuDrawer {
        id: menuDrawer
        x: parent. width
        y: 0
        width: parent.width < parent.height ? parent.width * 0.9 : parent.width/2
        height: parent.height
        visible: false
        onCloseMenu : {
            navigation.cancelNavigation()
        }
        onRouteOverview : {
            navigation.routeOverview()
        }
        onCancelRoute : {
            __root.state = ""
            mapNavigationBar.setNormalState()
            navitRoute.cancelNavigation();
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
            PropertyChanges {
                target: searchDrawer
                x: 0
                visible: true
            }

            PropertyChanges {
                target: overlay
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
                target: overlay
                color: "#a6000000"
                visible: true
            }
        },
        State {
            name: "routeOverview"

            PropertyChanges {
                target: overlay
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

            PropertyChanges {
                target: destinationDetailsBar
                width: parent.width > parent.height ? parent.height : parent.width
                visible: true
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
                target: overlay
                color: "#a6000000"
                visible: true
            }

        },
        State {
            name: "routeOverviewPOIs"
            PropertyChanges {
                target: overlay
                color: "#00000000"
                visible: true
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

            PropertyChanges {
                target: destinationDetailsBar
                width: parent.width > parent.height ? parent.height : parent.width
                visible: true
            }

            PropertyChanges {
                target: searchDrawer
                x: 0
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
                target:overlay
                property: "visible"
                value: true
            }
            ParallelAnimation{
                ColorAnimation {
                    target:overlay
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


/*##^##
Designer {
    D{i:0;formeditorZoom:0.33000001311302185;height:720;width:1280}D{i:4}D{i:5}D{i:7}
D{i:6}D{i:9}D{i:8}D{i:11}D{i:10}D{i:12}D{i:14}D{i:15}D{i:16}D{i:18}D{i:17}D{i:19}
D{i:25}D{i:26}D{i:27}D{i:28}D{i:30}D{i:29}D{i:31}D{i:32}D{i:33}D{i:35}D{i:34}D{i:37}
D{i:38}D{i:39}D{i:36}D{i:41}D{i:40}
}
##^##*/

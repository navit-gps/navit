import Qt 4.7
import "pagenavigation.js" as Navit

Rectangle {
    id: page

    width: gui.width; height: gui.height
    color: "Black"
    opacity: 0

    function pageOpen() {
        page.opacity = 1;
	if (navit.getPosition().length>0) {
		btnPoi.opacity=0.8;
	}
	if (navit.getDestination().length>0 && navit.getPosition().length>0) {
		btnRouteView.opacity=0.8;
		btnRouteBook.opacity=0.8;
		btnRouteHeight.opacity=0.8;
	}
	route.getDestinations();
    }

    Component.onCompleted: pageOpen();

    Behavior on opacity {
        NumberAnimation { id: opacityAnimation; duration: 300; alwaysRunToEnd: true }
    }

    Text {
	id: statusTxt;
    	text: route.getAttr("route_status"); color: "White"; font.pointSize: gui.height/20;
	anchors.top:parent.top;anchors.left:parent.left;anchors.topMargin: gui.height/20;anchors.leftMargin: gui.height/20;
    }
    Text {
	id: timeLabel;
    	text: "Time to drive route: "; color: "White"; font.pointSize: gui.height/20;
	anchors.top:statusTxt.top;anchors.left:statusTxt.right;anchors.leftMargin: gui.height/20;
    }
    Text {
	id: timeTxt;
    	text: route.getAttr("destination_time"); color: "White"; font.pointSize: gui.height/20;
	anchors.top:timeLabel.top;anchors.left:timeLabel.right;anchors.leftMargin: gui.height/20;
    }
    Text {
	id: lengthLabel;
    	text: "Route length: "; color: "White"; font.pointSize: gui.height/20;
	anchors.top:timeTxt.top;anchors.left:timeTxt.right;anchors.leftMargin: gui.height/20;
    }
    Text {
	id: lengthTxt;
    	text: route.getAttr("destination_length"); color: "White"; font.pointSize: gui.height/20;
	anchors.top:lengthLabel.top;anchors.left:lengthLabel.right;anchors.leftMargin: gui.height/20;
    }

    Grid {
        columns: 3;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.bottom: parent.verticalCenter; anchors.bottomMargin: gui.height/16;
        spacing: gui.width/12
        ButtonIcon {
            id: btnRouteView; text: "View route"; icon: "gui_town.svg"; onClicked: { navit.command("zoom_to_route()"), gui.backToMap(); }
            opacity: 0;
        }
        ButtonIcon {
            id: btnRouteBook; text: "Roadbook"; icon: "gui_log.svg"; onClicked: console.log("Implement me!");
            opacity: 0;
        }
        ButtonIcon {
            id: btnRouteHeight; text: "Height profile"; icon: "peak.svg"; onClicked: console.log("Implement me!");
            opacity: 0;
        }
    }

    Grid {
        columns: 3;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.top: parent.verticalCenter; anchors.topMargin: gui.height/16;
        spacing: gui.width/12
        ButtonIcon {
            id: btnView; text: "View position\non map"; icon: "gui_maps.svg"; onClicked: { navit.getPosition();navit.setCenter();gui.backToMap(); }
        }
        ButtonIcon {
            id: btnPoi; text: "POIs near\nPosition"; icon: "attraction.svg"; onClicked: Navit.load("PagePoi.qml");
	    opacity: 0;
        }
        ButtonIcon {
            id: btnStop; text: "Stop"; icon: "gui_stop.svg"; onClicked: { navit.stopNavigation(); Navit.back(); }
	    opacity: 0;
        }
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

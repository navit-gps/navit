import Qt 4.6

Rectangle {
    id: page

    width: 800; height: 424
    border.width: 1
    color: "Black"
    opacity: 0

    function pageOpen() {
        page.opacity = 1;
    }
    
    Component.onCompleted: pageOpen();    
    
    opacity: Behavior {
        NumberAnimation { id: opacityAnimation; duration: 300; alwaysRunToEnd: true }
    }

	ListSelector { 
		id:vehicleList; text: "Current vehicle profile"; itemId: navit.getAttrList("vehicle"); onChanged: {navit.setObjectByName("vehicle",vehicleList.value) }
		anchors.top: parent.top; anchors.topMargin: 48
		anchors.horizontalCenter: parent.horizontalCenter;		
	}

	ButtonIcon {
            id: btnVehicle; text: "Vehicle options"; icon: "gui_vehicle.svg"; onClicked: { gui.returnSource="PageSettingsVehicle.qml"; gui.setPage("PageSettingsVehicleOptions.qml") }
	    anchors.top: vehicleList.bottom; anchors.topMargin: 48
	    anchors.right: page.horizontalCenter;
        }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

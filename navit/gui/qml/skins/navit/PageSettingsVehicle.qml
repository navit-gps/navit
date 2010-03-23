import Qt 4.6

Rectangle {
    id: page

    width: gui.width; height: gui.height
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
		anchors.top: parent.top; anchors.topMargin: gui.height/16; anchors.leftMargin: gui.width/32
		anchors.left: parent.left; width: page.width/2;height: page.height/2
	}
	ButtonIcon {
            id: btnVehicle; text: "Vehicle options"; icon: "gui_vehicle.svg"; onClicked: gui.setPage("PageSettingsVehicleOptions.qml")
	    anchors.verticalCenter: vehicleList.verticalCenter; anchors.leftMargin: gui.width/32
	    anchors.left: vehicleList.right;
        }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}

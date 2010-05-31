import Qt 4.7

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
    
    Behavior on opacity {
        NumberAnimation { id: opacityAnimation; duration: 300; alwaysRunToEnd: true }
    }

     Component {
         id: listDelegate
         Item {
             id: wrapper
             width: list.width; height: 20
             Column {
                 x: 5; y: 5
                 Text { id: txtItemName; text: itemName; color: "White" }
		 Text { id: txtItemDist; text: itemDistance; color: "White"; anchors.leftMargin: 5; anchors.left: txtItemName.right;anchors.top: txtItemName.top }
		 Text { id: txtItemDirect; text: itemDirection; color: "White"; anchors.leftMargin: 5; anchors.left: txtItemDist.right;anchors.top: txtItemDist.top }
             }
	     MouseArea {
	   		id:delegateMouse
			anchors.fill: parent
			onClicked: { list.currentIndex=itemId; listselector.value=itemValue; listselector.changed() }
	     }
         }
     }
CommonHighlight { id: listHighlight} 
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

import Qt 4.7
import "pagenavigation.js" as Navit

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

    XmlListModel {
	id: listModel
	xml: navit.getAttrList("vehicle");
	query: "/attributes/vehicle"
	XmlRole { name: "itemId"; query: "id/string()" }
	XmlRole { name: "itemName"; query: "name/string()" }
    }

     Component {
         id: listDelegate
         Item {
             id: wrapper
             width: list.width; height: txtItemName.height
             Column {
                 x: 5; y: 5
                 Text { id: txtItemName; text: itemName; color: "White"; font.pointSize: 42 }
             }
	     MouseArea {
	   		id:delegateMouse
			anchors.fill: parent
			onClicked: { list.currentIndex=itemId; listselector.value=itemName; listselector.changed() }
		}
	}
    }
	CommonHighlight { id: listHighlight}

	ListSelector {
		id:vehicleList; text: "Current vehicle profile"; itemId: navit.itemId; onChanged: {navit.setObjectByName("vehicle",vehicleList.value) }
		anchors.top: parent.top; anchors.topMargin: gui.height/16; anchors.leftMargin: gui.width/32
		anchors.left: parent.left; width: page.width/2;height: page.height/2
	}

	ButtonIcon {
            id: btnVehicle; text: navit.getAttr("vehicle"); icon: "gui_vehicle.svg"; onClicked: Navit.load("PageSettingsVehicleOptions.qml");
	    anchors.verticalCenter: vehicleList.verticalCenter; anchors.leftMargin: gui.width/32
	    anchors.left: vehicleList.right;
        }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}
